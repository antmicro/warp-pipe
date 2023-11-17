# PCIe NVMe

This samples demonstrates the interaction with NVMe disks in Zephyr.

## Preparing the environment

Use the following commands to prepare the working environment:

<!-- name="pcie-nvme-prep" -->
```
west init --local --mf zephyr-samples/warp-pipe-zephyr.yml
west update
west zephyr-export
```

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
