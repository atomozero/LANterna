#include "DeviceStore.h"

namespace lanterna {

Device& DeviceStore::Upsert(const Device& device) {
    auto it = fByIp.find(device.ip);
    if (it == fByIp.end()) {
        auto res = fByIp.emplace(device.ip, device);
        return res.first->second;
    }
    it->second = device;
    return it->second;
}

Device* DeviceStore::Find(const std::string& ip) {
    auto it = fByIp.find(ip);
    return it == fByIp.end() ? nullptr : &it->second;
}

std::vector<Device> DeviceStore::All() const {
    std::vector<Device> out;
    out.reserve(fByIp.size());
    for (const auto& kv : fByIp)
        out.push_back(kv.second);
    return out;
}

} // namespace lanterna
