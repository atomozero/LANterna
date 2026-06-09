# LANterna

Piccola utility nativa per Haiku OS che scopre i dispositivi sulla LAN (IP,
hostname, porte aperte) e, nelle fasi successive, vendor da MAC, mDNS e
integrazione di sistema Haiku (attributi BFS, query Tracker, notifiche).

Stato: progettazione. Nessun codice applicativo ancora. Il design di L0 e le
verifiche tecniche da fare su Haiku reale sono in
[`docs/01-verifiche-e-design-L0.md`](docs/01-verifiche-e-design-L0.md).

## Milestone

- L0 Scoperta base: sottorete + TCP connect probe + reverse DNS (solo socket
  POSIX/BSD standard).
- L1 Arricchimento: vendor da MAC (OUI IEEE), mDNS (mjansson/mdns), tipo device.
- L2 Anima Haiku: device come oggetti con attributi BFS, query Tracker,
  Replicant/Deskbar.
- L3 Sentinella: scansione periodica e notifica per device nuovi.
- L4 Socket raw (se verificati): ARP/ICMP e MAC affidabile.

## Vincoli

- Niente Qt, niente dipendenze pesanti. Solo mattoni compatibili MIT.
- Niente funzioni offensive, niente packet sniffer, niente cloud.

Licenza: MIT (vedi `LICENSE`).
