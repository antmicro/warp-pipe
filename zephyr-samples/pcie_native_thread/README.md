# PCIe Zephyr native (threaded) sample

This sample interacts directly with memory-mock device using warp-pipe library.
The purpose of the application is to interact with a memory device using multiple threads.

## Preparing the environment

Please refer to [preparing the environment for Zephyr samples](../../README.md#preparing-the-environment-for-zephyr-samples) in main README.

## Building and running

This sample requires installed `warp-pipe` library.
Please refer to [installing warp-pipe](../../README.md#building-and-installing-warp-pipe) for more information.

Use the following command to build this sample:
<!-- name="pcie-native-thread-build" -->
```
west build -p -d build.pcie_native_thread -b native_sim zephyr-samples/pcie_native_thread
```

This sample requires `memory-mock` that acts as a PCIe memory device.
Please refer to [building memory-mock](../../README.md#building-memory-mock) for more information.

Now you can launch both `memory-mock` and this sample:
<!-- name="pcie-native-thread-run" -->
```
./build_memory_mock/memory_mock &
SERVER_PID=$!
./build.pcie_native_thread/zephyr/zephyr.exe &
ZEPHYR_PID=$!
```

After that, you can terminate all background processes (the sleep is needed if you use this snippet inside a script, to wait some time for output):
<!-- name="pcie-native-thread-teardown" -->
```
sleep 30
kill -INT $ZEPHYR_PID
kill -INT $SERVER_PID
```
