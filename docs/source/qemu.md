# QEMU

Warp Pipe repository tracks a fork of QEMU that includes a `hw/pci/warp_pipe.c` device, an interface towards `warp-pipe`, loosely based on the `hw/misc/edu.c` educational PCI device.

The QEMU plugin for warp-pipe allows to create a PCIe device using QEMU command-line interface, like:
```sh
qemu-system-x86_64 -device warp-pipe,connect=127.0.0.1:2115
```

The above will launch a QEMU system emulation process with an extra PCIe device attached, backed by a remote Warp Pipe server.

## Building QEMU

If you installed warp-pipe to a non-standard location, you need to set the `PKG_CONFIG_PATH` variable:

```
export PKG_CONFIG_PATH=path/to/install-dir/lib/pkgconfig
```

The following snippet will allow you to build warp-pipe QEMU fork:

```
# ensure submodules are updated
git submodule update --init --recursive
mkdir qemu/build
cd qemu/build
../configure --enable-warp-pipe --target-list=x86_64-softmmu
make -j$(nproc)
export QEMU_BIN_PATH=`pwd`
cd -
```
