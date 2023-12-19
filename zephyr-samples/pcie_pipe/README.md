# PCIe PIPE

This samples demonstrates the functionality of the ``pcie-pipe`` device from QEMU using warp-pipe.

## Preparing the environment

Please refer to [preparing the environment for Zephyr samples](../../README.md#preparing-the-environment-for-zephyr-samples) in main README.

## Building and running

This sample requires installed `warp-pipe` library and `memory-mock` that acts as a end PCIe device.
Please refer to [building memory-mock](../../README.md#building-memory-mock) and [installing warp-pipe](../../README.md#building-and-installing-warp-pipe) for more information.

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
./build_memory_mock/memory_mock &
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
