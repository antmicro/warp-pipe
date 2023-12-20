# PCIe IVSHMEM

This sample demonstrates the interaction with [Inter-VM Shared Memory device](https://www.qemu.org/docs/master/system/devices/ivshmem.html) over PCIe.

## Preparing the environment

Please refer to [preparing the environment for Zephyr samples](../../README.md#preparing-the-environment-for-zephyr-samples) in main README.

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
