# PCIe Scan

This samples demonstrates the functionality of the ``pcie-pipe`` device from QEMU using warp-pipe.

## Preparing the environment

Use the following commands to prepare the working environment:

<!-- name="pcie-pipe-prep" -->
```
west init --local --mf zephyr-samples/warp-pipe-zephyr.yml
west update -o=--depth=1 -n
west zephyr-export
```

## Building and running

This sample requires installed `warp-pipe` library and `memory-mock` that acts as a end PCIe device.
Use the following command to build `memory-mock` and install `warp-pipe`:
<!-- name="pcie-pipe-lib-prep" -->
```
mkdir -p build_lib
cmake -S . -B build_lib
make install -j $(nproc) -C build_lib
mkdir -p build
cmake -S memory-mock -B build
make -j $(nproc) -C build
```

This sample requires  also custom QEMU with warp-pipe integration. Use following commands to build it:
<!-- name="pcie-qemu-build" -->
```
mkdir qemu/build
pushd qemu/build
../configure --enable-warp-pipe --target-list=x86_64-softmmu
make -j$(nproc)
export QEMU_BIN_PATH=`pwd`
popd
```

Use the following command to build this sample:
<!-- name="pcie-pipe-build" -->
```
west build -d build.pcie_pipe/ -b qemu_x86_64 zephyr-samples/pcie_pipe/
```

Now you can launch both `memory-mock` and this sample:
<!-- name="pcie-pipe-run" -->
```
./build/memory_mock &
SERVER_PID=$!
west build -t run -d build.pcie_pipe/ &
ZEPHYR_PID=$!
```

You should see the following output on the standard output console:
```
Data transfer finished: 0x10 0x11 0x12 0x13 0x14 0x15 0x16 0x17 0x18 0x19 0x1a 0x1b 0x1c 0x1d 0x1e 0x1f 0x20 0x21 0x22 0x23 0x24 0x25 0x26 0x27 0x28 0x29 0x2a 0x2b 0x2c 0x2d 0x2e 0x2f
```

After that, you can terminate all background processes:
<!-- name="pcie-pipe-teardown" -->
```
sleep 10
kill -INT $ZEPHYR_PID
kill -INT $SERVER_PID
```
