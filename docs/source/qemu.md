# QEMU

The QEMU plugin for warp-pipe allows to create a PCIe device using QEMU command-line interface, like:
```sh
qemu-system-x86_64 -device warp-pipe,connect=127.0.0.1:2115
```

The above will launch a QEMU system emulation process with an extra PCIe device attached, backed by a remote Warp Pipe server.
