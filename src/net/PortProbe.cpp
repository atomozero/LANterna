#include "PortProbe.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <unordered_map>

namespace lanterna {

namespace {

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
        .count();
}

struct InFlight {
    uint32_t ip;
    uint16_t port;
    int64_t deadline;
};

bool SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

// Classifica un errno di connect (immediato o da SO_ERROR) in un ProbeResult.
ProbeResult ClassifyError(int err) {
    if (err == 0) return ProbeResult::Open;
    if (err == ECONNREFUSED) return ProbeResult::Refused;
    if (err == ETIMEDOUT) return ProbeResult::Timeout;
    return ProbeResult::Unreachable; // ENETUNREACH, EHOSTUNREACH, ecc.
}

} // namespace

std::vector<ProbeOutcome> RunProbes(
    const std::vector<ProbeTarget>& targets,
    const ProbeConfig& config,
    const std::function<void(const ProbeOutcome&)>& callback) {

    std::vector<ProbeOutcome> outcomes;
    outcomes.reserve(targets.size());

    auto emit = [&](uint32_t ip, uint16_t port, ProbeResult r) {
        ProbeOutcome o{ip, port, r};
        outcomes.push_back(o);
        if (callback) callback(o);
    };

    std::unordered_map<int, InFlight> inflight; // fd -> stato
    size_t next = 0;                            // prossimo target da avviare

    auto startOne = [&]() -> bool {
        while (next < targets.size()) {
            const ProbeTarget t = targets[next++];
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) {
                // Esaurito qualche risorsa (EMFILE/ENFILE): segnala e prosegui.
                emit(t.ip, t.port, ProbeResult::Unreachable);
                continue;
            }
            if (!SetNonBlocking(fd)) {
                close(fd);
                emit(t.ip, t.port, ProbeResult::Unreachable);
                continue;
            }
            struct sockaddr_in sa{};
            sa.sin_family = AF_INET;
            sa.sin_port = htons(t.port);
            sa.sin_addr.s_addr = htonl(t.ip);

            int rc = connect(fd, reinterpret_cast<struct sockaddr*>(&sa),
                             sizeof(sa));
            if (rc == 0) {
                // Connessione immediata (tipico su loopback).
                close(fd);
                emit(t.ip, t.port, ProbeResult::Open);
                continue;
            }
            if (errno == EINPROGRESS) {
                inflight[fd] = InFlight{t.ip, t.port, NowMs() + config.timeoutMs};
                return true;
            }
            // Errore immediato (es. ECONNREFUSED, ENETUNREACH).
            ProbeResult r = ClassifyError(errno);
            close(fd);
            emit(t.ip, t.port, r);
            // non era davvero "in volo": continua a riempire
        }
        return false;
    };

    // Riempi il primo lotto.
    while (static_cast<int>(inflight.size()) < config.maxInFlight) {
        if (!startOne()) break;
    }

    std::vector<struct pollfd> pfds;
    while (!inflight.empty()) {
        pfds.clear();
        pfds.reserve(inflight.size());
        for (const auto& kv : inflight) {
            struct pollfd p{};
            p.fd = kv.first;
            p.events = POLLOUT;
            pfds.push_back(p);
        }

        int rc = poll(pfds.data(), pfds.size(), 100 /* tick ms */);
        int64_t now = NowMs();

        if (rc > 0) {
            for (const auto& p : pfds) {
                if (p.revents == 0) continue;
                auto it = inflight.find(p.fd);
                if (it == inflight.end()) continue;

                int err = 0;
                socklen_t len = sizeof(err);
                if (getsockopt(p.fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0)
                    err = errno ? errno : ECONNABORTED;

                ProbeResult r = ClassifyError(err);
                emit(it->second.ip, it->second.port, r);
                close(p.fd);
                inflight.erase(it);
            }
        }

        // Scadenze: chiudi le probe oltre deadline.
        for (auto it = inflight.begin(); it != inflight.end();) {
            if (now >= it->second.deadline) {
                emit(it->second.ip, it->second.port, ProbeResult::Timeout);
                close(it->first);
                it = inflight.erase(it);
            } else {
                ++it;
            }
        }

        // Rifornisci il lotto.
        while (static_cast<int>(inflight.size()) < config.maxInFlight) {
            if (!startOne()) break;
        }
    }

    return outcomes;
}

} // namespace lanterna
