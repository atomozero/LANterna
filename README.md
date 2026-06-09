# LANterna

Piccola utility nativa per Haiku OS che scopre i dispositivi sulla LAN (IP,
hostname, porte aperte) e, nelle fasi successive, vendor da MAC, mDNS e
integrazione di sistema Haiku (attributi BFS, query Tracker, notifiche).

Stato: progettazione. Nessun codice applicativo ancora. Il design di L0 e le
verifiche tecniche da fare su Haiku reale sono in
[`docs/01-verifiche-e-design-L0.md`](docs/01-verifiche-e-design-L0.md).

## Milestone

- L0 Scoperta base: sottorete + TCP connect probe + reverse DNS (solo socket
  POSIX/BSD standard). Core fatto e testato; UI nativa BeAPI scritta, da
  compilare e verificare su Haiku.
- L1 Arricchimento: vendor da MAC (OUI IEEE), tipo device (fatto). Il MAC
  degli host on-link si ricava dalla cache ARP di sistema dopo il connect,
  senza socket raw (backend Linux `/proc/net/arp`; backend Haiku via
  `/proc/net/arp` BSD compat).
- L2 Anima Haiku: persistenza su attributi BFS, finestra Riepilogo (pivot),
  topologia interattiva, replicant Desktop, BNotification, BAboutWindow,
  pannello impostazioni, multilingua (it/en/es/de/ja).
- L3 Discovery avanzato (in corso): mDNS/DNS-SD attivo (fatto), NetBIOS,
  SNMP, UPnP/SSDP.
- L4 Sentinella: scansione periodica (fatto) e notifica per device
  scomparsi (in corso).
- L5 Socket raw (se verificati): ARP/ICMP e MAC affidabile.

## Pipeline di arricchimento

Il core applica una pipeline di `Enricher` su ogni device scoperto. L'ordine
attuale e':

1. `ArpMacEnricher`     - MAC da cache ARP del sistema
2. `OuiVendorEnricher`  - vendor da prefisso OUI (`oui.txt` IEEE)
3. `ReverseDnsEnricher` - hostname da reverse DNS unicast
4. `MdnsEnricher`       - hostname `.local` e tipo da DNS-SD multicast
5. `NetBiosEnricher`    - nome NetBIOS e workgroup (UDP 137, NBSTAT)
6. `SnmpEnricher`       - sysDescr e sysName via SNMPv1 GET (UDP 161)
7. `SsdpEnricher`       - smart TV, Sonos, NAS, router IGD via SSDP/UPnP
                         multicast (M-SEARCH a 239.255.255.250:1900)
8. `TypeInferenceEnricher` - tipo device inferito dalle porte aperte

Gli enricher multicast (mDNS, SSDP) eseguono una "discovery" una sola
volta all'inizio della scansione e riempiono cache `ip -> dati` che gli
`Enrich()` per device consultano. Gli enricher unicast (NetBIOS, SNMP)
contattano direttamente ogni device durante l'arricchimento, con timeout
brevi (250-300ms) per non rallentare la scansione.

`MdnsEnricher` esegue una "discovery" multicast una volta sola all'inizio
della scansione (query a 224.0.0.251:5353 per `_services._dns-sd._udp` e
per ~10 service-type comuni: HTTP, IPP, SMB, AFP, AirPlay, Chromecast,
HomeKit, SSH, workstation). Le risposte popolano una cache `ip -> servizi`
che viene poi consultata per ogni device.

## Vincoli

- Niente Qt, niente dipendenze pesanti. Solo mattoni compatibili MIT.
- Niente funzioni offensive, niente packet sniffer, niente cloud.

## Build

Core portabile + CLI di test (Linux/POSIX, per sviluppare e verificare la
logica di scoperta):

    make
    ./lanterna-cli --list-interfaces
    ./lanterna-cli --oui data/oui-sample.txt

App nativa Haiku con UI BeAPI (da compilare SU Haiku, vedi note nel file):

    make -f Makefile.haiku

La UI (`src/ui/`) e' scritta in BeAPI ma non e' stata compilata fuori da
Haiku: header come BColumnListView e librerie come libbe vanno verificati
sul sistema reale. Il core resta indipendente e testabile su qualunque POSIX.

Licenza: MIT (vedi `LICENSE`).
