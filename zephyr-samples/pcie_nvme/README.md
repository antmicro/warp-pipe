# PCIe NVMe

This samples demonstrates the interaction with NVMe disks in Zephyr.

## Preparing the environment

Please refer to [preparing the environment for Zephyr samples](../../README.md#preparing-the-environment-for-zephyr-samples) in main README.

## Building and running

Use the following command to build this sample:

<!-- name="pcie-nvme-build" -->
```
west build -p -d build.pcie_nvme -b qemu_x86_64 zephyr-samples/pcie_nvme
```

You should see the following output on the standard output console:
```
*** Booting Zephyr OS build zephyr-v3.5.0 ***
Checking disk status... OK
Disk reports 2048 sectors
Disk reports sector size 512
Performing writes to a single disk sector...
Reading and verifying data... Correct!
```
