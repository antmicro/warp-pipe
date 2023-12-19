Warp Pipe
=========

Copyright (c) 2023 [Antmicro](https://www.antmicro.com)  
Copyright (c) 2023 Meta

A TCP server for the Warp Pipe Library, which aims to facilitate
creating PCIe links in a distributed simulation environment. This includes:
* packetizing/depacketizing PCIe transactions into TLPs and DLLPs 
* transmitting PCIe transactions over sockets using a custom PCIe/IP TCP-based
  protocol
* creating PCIe links between instances of different simulators/emulators (e.g.
  [Renode](https://renode.io/) and QEMU)
* handling MSIs, packet routing and flow control

Building
--------

To build the Warp Pipe, use the following snippet:

<!-- name="server-build" -->
```
mkdir -p build
cmake -S . -B build
make -j $(nproc) -C build
```

To launch the server, use:
```
./build/server
```

Syslog
------

To view logs, use:

```
journalctl -f
```
