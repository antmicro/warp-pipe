# PCIe DMA

This sample demonstrates the interaction using DMA over PCIe.

## Preparing the environment

Use the following commands to prepare the working environment:

Please refer to [preparing the environment for Zephyr samples](../../README.md#preparing-the-environment-for-zephyr-samples) in main README.

## Building and running

Use the following command to build this sample:

<!-- name="pcie-dma-build" -->
```
west build -p -d build.pcie_dma -b native_sim zephyr-samples/pcie_dma/
```

This sample requires `memory-mock` that acts as a end PCIe device.
Please refer to [building memory-mock](../../README.md#building-memory-mock) for more information.

Now you can launch both `memory-mock` and this sample:
<!-- name="pcie-dma-run" -->
```
./build_memory_mock/memory_mock &
SERVER_PID=$!
build.pcie_dma/zephyr/zephyr.exe &
ZEPHYR_PID=$!
```

You should see the following output on the standard output console:
```
Data transfer finished: 0x10 0x11 0x12 0x13 0x14 0x15 0x16 0x17 0x18 0x19 0x1a 0x1b 0x1c 0x1d 0x1e 0x1f 0x20 0x21 0x22 0x23 0x24 0x25 0x26 0x27 0x28 0x29 0x2a 0x2b 0x2c 0x2d 0x2e 0x2f
```

After that, you can terminate all background processes (the sleep is needed if you use this snippet inside a script, to wait some time for output):
<!-- name="pcie-dma-teardown" -->
```
sleep 10
kill -INT $ZEPHYR_PID
kill -INT $SERVER_PID
```
