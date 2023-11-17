# PCIe Scan

This samples demonstrates the functionality of the ``pcie_scan`` function in Zephyr.

## Preparing the environment

Use the following commands to prepare the working environment:

<!-- name="pcie-scan-prep" -->
```
west init --local --mf zephyr-samples/warp-pipe-zephyr.yml
west update
west zephyr-export
```

## Building and running

Use the following command to build this sample:

<!-- name="pcie-scan-build" -->
```
west build -p -d build.pcie_scan -b qemu_cortex_a53 zephyr-samples/pcie_scan
```

You should see the following output on the standard output console:
```
*** Booting Zephyr OS build zephyr-v3.5.0 ***
Scanning PCIe endpoints...
[0:0.0] Found PCIe endpoint: vendor=0x1b36 device=0x8 class=0x6 subclass=0x0 progif=0x0 rev=0x0 bridge=0
[0:0.0]  BAR 0: type=mem size=32 addr=0x00000000
[0:0.0]  BAR 1: type=mem size=32 addr=0x00000000
[0:0.0]  BAR 2: type=mem size=32 addr=0x00000000
[0:0.0]  BAR 3: type=mem size=32 addr=0x00000000
[0:0.0]  BAR 4: type=mem size=32 addr=0x00000000
[0:0.0]  BAR 5: type=mem size=32 addr=0x00000000
[0:1.0] Found PCIe endpoint: vendor=0x1b36 device=0x5 class=0x0 subclass=0xff progif=0x0 rev=0x0 bridge=0
[0:1.0]  BAR 0: type=mem size=32 addr=0x10000000
[0:1.0]  BAR 1: type=io size=32 addr=0x00001000
[0:1.0]  BAR 2: type=mem size=64 addr=0x0000008000000000
[0:1.0]  BAR 4: type=mem size=32 addr=0x00000000
[0:1.0]  BAR 5: type=mem size=32 addr=0x00000000
```
