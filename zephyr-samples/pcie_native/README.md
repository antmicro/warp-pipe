# PCIe Zephyr native sample

This sample interacts directly with memory-mock device using warp-pipe library.

## Preparing the environment

Please refer to [preparing the environment for Zephyr samples](../../README.md#preparing-the-environment-for-zephyr-samples) in main README.

## Building and running

This sample requires installed `warp-pipe` library.
Please refer to [installing warp-pipe](../../README.md#building-and-installing-warp-pipe) for more information.

This sample requires also custom QEMU with warp-pipe integration. Use following commands to build it:
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
<!-- name="pcie-native-build" -->
```
west build -p -d build.pcie_native -b native_sim zephyr-samples/pcie_native
```

This sample requires `memory-mock` that acts as a PCIe memory device.
Please refer to [building memory-mock](../../README.md#building-memory-mock) for more information.

Now you can launch both `memory-mock` and this sample:
<!-- name="pcie-native-run" -->
```
./build_memory_mock/memory_mock &
SERVER_PID=$!
build.pcie_native/zephyr/zephyr.exe &
ZEPHYR_PID=$!
```

You should see the following output on the standard output console:
```
*** Booting Zephyr OS build f89c5ddd1aaf ***
[00:00:00.000,000] <inf> pcie_native: Started app
[00:00:00.000,000] <inf> pcie_native: Started client
[00:00:00.000,000] <inf> pcie_native: Registering bar 0 at 0x100000 (size: 128)
[00:00:00.000,000] <inf> pcie_native: Registering bar 1 at 0x140000 (size: 1024)
Read data (len: 16): 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f
Read data (len: 16): 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
Read data (len: 16): 68 65 6c 6c 6f 00 00 00 00 00 00 00 00 00 00 00
[00:00:00.000,000] <inf> pcie_native: The next read should fail
[00:00:00.000,000] <err> pcie_native: Failed to read bar 2 at addr 0-0
[00:00:00.000,000] <inf> pcie_native: All checks has succeded.
```

After that, you can terminate all background processes (the sleep is needed if you use this snippet inside a script, to wait some time for output):
<!-- name="pcie-native-teardown" -->
```
sleep 10
kill -2 $ZEPHYR_PID
kill -2 $SERVER_PID
```
