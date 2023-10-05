PCIe Communication Server
=========================

Building
--------

To build the PCIe Communication Server, use the following snippet:

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
journalctl -t pcied -f
```
