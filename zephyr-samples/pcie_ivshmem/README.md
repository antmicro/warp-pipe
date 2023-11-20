# PCIe IVSHMEM

This sample demonstrates the interaction with [Inter-VM Shared Memory device](https://www.qemu.org/docs/master/system/devices/ivshmem.html) over PCIe.

## Preparing the environment

Use the following commands to prepare the working environment:

<!-- name="pcie-scan-prep" -->
```
west init --local --mf zephyr-samples/warp-pipe-zephyr.yml
west update -o=--depth=1 -n
west zephyr-export
```

## Building and running

Use the following command to build this sample:

<!-- name="pcie-ivshmem-build" -->
```
west build -p -d build.pcie_ivshmem -b qemu_cortex_a53 zephyr-samples/pcie_ivshmem
```

You should see the following output on the standard output console:
```
*** Booting Zephyr OS build zephyr-v3.5.0-1103-g183b2a0120a3 ***
Shared memory size: 0x100000
Reading and verifying data... Correct!
```
