# PCIe DMA

This sample demonstrates the interaction using DMA over PCIe.

## Preparing the environment

Use the following commands to prepare the working environment:

<!-- name="pcie-dma-prep" -->
```
west init --local --mf zephyr-samples/warp-pipe-zephyr.yml
west update -o=--depth=1 -n
west zephyr-export
```

## Building and running

Use the following command to build this sample:

<!-- name="pcie-dma-build" -->
```
west build -p -d build.pcie_dma -b native_sim zephyr-samples/pcie_dma/
```

This sample requires also `memory-mock` that acts as a end PCIe device.
Use the following command to build `memory-mock`:
<!-- name="pcie-dma-memory-mock" -->
```
mkdir -p build
cmake -S memory-mock -B build
make -j $(nproc) -C build
```

To allow communication between zephyr and host, `zeth` interface is needed.
This interface can be setup using `net-setup.sh` from `net-tools` repository:
<!-- name="pcie-dma-net-setup" -->
```
../tools/net-tools/net-setup.sh &
# save PID for later
NET_TOOLS_PID=$!
```

Now you can launch both `memory-mock` and this sample:
<!-- name="pcie-dma-run" -->
```
./build/memory_mock &
SERVER_PID=$!
build.pcie_dma/zephyr/zephyr.exe &
ZEPHYR_PID=$!
```

You should see the following output on the standard output console:
```
Data transfer finished: 0x10 0x11 0x12 0x13 0x14 0x15 0x16 0x17 0x18 0x19 0x1a 0x1b 0x1c 0x1d 0x1e 0x1f 0x20 0x21 0x22 0x23 0x24 0x25 0x26 0x27 0x28 0x29 0x2a 0x2b 0x2c 0x2d 0x2e 0x2f
```

After that, you can terminate all background processes:
<!-- name="pcie-dma-teardown" -->
```
sleep 10
kill -2 $ZEPHYR_PID
kill -2 $SERVER_PID
kill -2 $NET_TOOLS_PID
```
