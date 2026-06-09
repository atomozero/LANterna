# LANterna - Verifiche Haiku e design L0

Documento di lavoro. Tutto cio' che riguarda API e librerie Haiku va trattato
come "da confermare sul sistema reale" finche' non passa i probe descritti qui.
Niente di questo e' stato eseguito su Haiku: sono conoscenze BeAPI/POSIX piu'
istruzioni concrete per accertarle on-device.

## A. Incognite da verificare per prime (gate di L0)

### A.1 Enumerazione interfacce e netmask

Due strade. La nativa e' preferibile, la POSIX e' il piano B.

Strada nativa (BeAPI, libbnetapi):
- Header attesi: `NetworkRoster.h`, `NetworkInterface.h`,
  `NetworkInterfaceAddress.h`, `NetworkAddress.h`.
- Iterazione: `BNetworkRoster::Default().GetNextInterface(uint32* cookie, BNetworkInterface&)`.
- Per interfaccia: `CountAddresses()` + `GetAddressAt(i, BNetworkInterfaceAddress&)`.
- Da ogni indirizzo: `Address()`, `Mask()`, `Broadcast()` -> `BNetworkAddress`.
- Filtro: `Flags()` per scartare loopback e interfacce down (IFF_UP, IFF_LOOPBACK).
- Link: `-lbnetapi -lnetwork`.

Strada POSIX (fallback):
- `getifaddrs()` / `freeifaddrs()` se presente, oppure
  `ioctl(sock, SIOCGIFCONF, ...)` con `struct ifconf`/`ifreq` e `SIOCGIFNETMASK`.

Come accertare concretamente:
1. In Terminal: `ifconfig` per vedere interfacce e maschere attese.
2. Header presenti:
   `ls /boot/system/develop/headers/os/net/`
   (cercare NetworkRoster.h, NetworkInterface.h)
   `grep -r getifaddrs /boot/system/develop/headers/posix/`
3. Libreria presente: `ls /boot/system/lib/libbnetapi.so`
4. Probe da ~20 righe che itera BNetworkRoster e stampa addr/mask/flags;
   compilare con `g++ probe.cpp -lbnetapi -lnetwork`.

Decisione: usare la strada nativa se il probe (4) compila e gira; tenere il
fallback POSIX dietro la stessa interfaccia `Subnet`.

### A.2 Reverse DNS

- POSIX: `getnameinfo(sockaddr*, len, host, NI_MAXHOST, NULL, 0, NI_NAMEREQD)`.
  Standard, Haiku ha `netdb`. Dipende dal resolver (`/etc/resolv.conf`) e dal
  fatto che il router pubblichi i PTR: in molte LAN domestiche i PTR non ci
  sono, quindi aspettarsi parecchi host senza nome. E' il motivo per cui mDNS
  in L1 conta davvero.
- Accertare: `grep getnameinfo /boot/system/develop/headers/posix/netdb.h`,
  poi un mini programma che risolve un IP noto. Sanity check con `host <ip>` o
  `nslookup` se presenti.

### A.3 Tante connect TCP non bloccanti in parallelo

- Modello: socket non bloccante (`fcntl O_NONBLOCK` o `ioctl FIONBIO`),
  `connect()` ritorna `EINPROGRESS`, si attende scrivibilita', poi
  `getsockopt(SO_ERROR)`: 0 = porta aperta.
- Multiplexing: usare `poll()`, non `select()`. select e' limitato da
  `FD_SETSIZE` (valore Haiku da verificare, storicamente basso). epoll/kqueue
  su Haiku NON sono affidabili: non contarci.
- Limiti reali da misurare:
  - `getrlimit(RLIMIT_NOFILE)` o `ulimit -n` in bash: max fd per team.
    Storicamente Haiku ha default bassi, quindi servira' comunque batching.
  - Comportamento sotto carico: cercare `EMFILE`, `ENOBUFS`.
- Accertare: programma di test che apre N connect non bloccanti verso un /24
  con un loop poll, misura tempo e conta errori. Annotare `ulimit -n`.

Strategia conseguente: concorrenza limitata (es. 128-256 socket in volo),
timeout di connect breve (300-500 ms), a lotti. Sidesteppa sia i limiti di fd
sia il rischio di floodare la rete.

### A.4 Onesta' su librerie/metodi non verificabili da qui

- libbnetapi: non posso confermare le firme esatte sulla tua build. Probe A.1.
- `getifaddrs`/`getnameinfo`: presenza da confermare con i grep sopra.
- mjansson/mdns (header-only C, pubblico dominio): dovrebbe compilare, ma la
  join multicast UDP (`IP_ADD_MEMBERSHIP`) su Haiku va verificata. Resta L1.
- Socket raw (ICMP/ARP): il supporto `SOCK_RAW` su Haiku e' storicamente
  incompleto. Non farci affidamento. Per questo ARP/ICMP sono L4 e sotto gate.
- MAC address: correzione importante al piano. Su pura connect TCP il MAC NON
  e' ottenibile. Il MAC on-link arriva da ARP (L4) o leggendo la cache ARP del
  sistema (`SIOCGARP`/route table), supporto Haiku incerto. Conseguenza: il
  vendor-da-MAC di L1 e' realisticamente subordinato a L4 o alla lettura della
  cache ARP. Da segnare come dipendenza, non come acquisito.

## B. Design L0

Obiettivo L0: apri, premi Scansiona, in pochi secondi vedi IP + hostname +
porte aperte in una finestra nativa.

### B.1 Struttura del codice (disaccoppiata per innesto L1/L2)

```
src/
  net/
    Subnet.{h,cpp}      // interfaccia + maschera, enumerazione host del /N
    PortProbe.{h,cpp}   // connect non bloccante via poll, a lotti
    Resolver.{h,cpp}    // reverse DNS (getnameinfo)
  model/
    Device.{h,cpp}      // record device, gia' esteso per L1/L2
    DeviceStore.{h,cpp} // collezione, dedup per IP, notifica osservatori
    Enricher.h          // interfaccia per la pipeline di arricchimento
  scan/
    Scanner.{h,cpp}     // orchestratore, gira in thread proprio, posta BMessage
  ui/
    App.{h,cpp}
    MainWindow.{h,cpp}
    DeviceListView.{h,cpp}  // BColumnListView
  Makefile
```

### B.2 Modello device, esteso fin da subito (campi L1/L2 vuoti in L0)

```cpp
struct Device {
    BString ip;                  // L0
    BString hostname;            // L0 (reverse DNS, puo' restare vuoto)
    std::set<uint16> openPorts;  // L0
    // L1 (vuoti in L0):
    BString mac;                 // dipende da L4 / cache ARP
    BString vendor;              // da OUI, richiede mac
    BString mdnsName;
    BString deviceType;          // inferito da porte/servizi
    // L2:
    time_t firstSeen = 0;
    time_t lastSeen = 0;
    entry_ref bfsRef;            // file che materializza il device
};
```

### B.3 Pipeline di arricchimento (chiave per non riscrivere dopo)

```cpp
class Enricher {
public:
    virtual ~Enricher() {}
    virtual void Enrich(Device& d) = 0;  // chiamato per ogni device scoperto
};
```

- L0 registra solo: `ReverseDnsEnricher`.
- L1 aggiunge: `OuiVendorEnricher`, `MdnsEnricher`, `TypeInferenceEnricher`.
- L2 aggiunge: `BfsPersistEnricher` (scrive attributi e indici).

Lo `Scanner` esegue la pipeline registrata. Aggiungere L1/L2 = registrare altri
enricher, zero riscrittura di scoperta/UI.

### B.4 Parallelizzazione delle probe

1. `Subnet` costruisce la lista host: da network+1 a broadcast-1, escluso self.
2. Per ogni host, tentativo di connect verso il set di porte default.
3. Un solo loop `poll()` con insieme in-volo limitato (contatore tipo semaforo).
   Per ogni fd si tiene (ip, porta, deadline). Scrivibile + `SO_ERROR==0` =>
   aperta. Deadline scaduta => chiusa/filtrata, chiudi fd e rifornisci il lotto.
4. Tutto in un thread worker (`spawn_thread` BeAPI o `std::thread`). La UI riceve
   progresso e risultati via `BMessenger`/`BMessage` (`PostMessage` alla finestra).
   Mai bloccare il thread della finestra.

Set di porte default iniziale (piccolo, poi configurabile):
22, 80, 443, 139, 445, 548, 631, 5000, 5353, 8080, 9100, 53317.

### B.5 UI nativa

- `BApplication` + `BWindow` con `BColumnListView` (Interface Kit). Colonne: IP,
  Host, Vendor (vuota in L0), Porte. Bottone "Scansiona", barra di stato con
  avanzamento.
- Regola di threading BeAPI: il worker posta `BMessage` (es. `'dvfd'` device
  trovato, `'prog'`, `'done'`); la finestra aggiorna la lista nel proprio thread
  in `MessageReceived`. Pattern corretto BeAPI, niente accesso cross-thread alla
  view.

### B.6 Come si innestano L1 e L2 senza riscrivere

- L1: nuovi enricher + nuove colonne. Nota onesta': senza MAC (vedi A.4) il
  vendor resta spesso vuoto finche' non arriva L4/cache ARP. mDNS e inferenza da
  porte funzionano comunque e danno gia' nome amichevole e tipo device.
- L2: `BfsPersistEnricher` scrive attributi (`LANterna:ip`, `:mac`, `:vendor`,
  `:hostname`, `:first_seen`, `:last_seen`), crea gli indici con
  `fs_create_index`, definisce un tipo file e deposita un file per device in una
  cartella dedicata. Le query Tracker e `BQuery` ("visti oggi", "vendor X",
  "mai visti prima") lavorano sugli attributi indicizzati.

## C. Prossimi passi consigliati

1. Eseguire i probe A.1-A.3 su Haiku reale e annotare i risultati in un
   `docs/02-risultati-verifiche.md`.
2. Solo dopo, scaffold di `Subnet` + `PortProbe` + `Resolver` + UI minimale.
3. Confermare il buco di ecosistema in HaikuDepot (cerca: scanner, network,
   nmap, ping, discovery).
