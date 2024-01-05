Warp Pipe
=========

Copyright (c) 2023 [Antmicro](https://www.antmicro.com)  
Copyright (c) 2023 Meta

A TCP server for the Warp Pipe Library, which aims to facilitate
creating PCIe links in a distributed simulation environment. This includes:
* packetizing/depacketizing PCIe transactions into TLPs and DLLPs 
* transmitting PCIe transactions over sockets using a custom PCIe/IP TCP-based
  protocol
* creating PCIe links between instances of different simulators/emulators (e.g.
  [Renode](https://renode.io/) and QEMU)
* handling MSIs, packet routing and flow control

Building and installing warp-pipe
---------------------------------

To build Warp Pipe, use the following snippet:

<!-- name="warp-pipe-build" -->
```
mkdir -p build
cmake -S . -B build
make -j $(nproc) -C build
```

and install it using:

<!-- name="warp-pipe-install" -->
```
make install -C build
```

You can select PATH where `warp-pipe` will be installed using `CMAKE_INSTALL_PREFIX` during cmake configuration step:
```
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=`pwd`/install
```

You can also disable building tests by setting `-DENABLE_TESTS=OFF`.

Running tests
-------------

Tests are build by default while building `warp-pipe` library. To execute tests, run:

<!-- name="warp-pipe-tests" -->
```
./build/warppipe-tests
```

Generating coverage report
--------------------------

To generate tests coverage report, use the following snippet:

<!-- name="warp-pipe-coverage" -->
```
./scripts/coverage.sh
```

Generated coverage report can be found in the `coverage` folder.


Building memory-mock
--------------------

This repository contains a `memory-mock` application that uses `warp-pipe` library and acts as simple PCIe device that emulates memory.
To build it, use the following snippet:

<!-- name="memory-mock-build" -->
```
mkdir -p build_memory_mock
cmake -S memory-mock -B build_memory_mock
make -j $(nproc) -C build_memory_mock
```

If you installed `warp-pipe` to custom localization, you might also need to set `PKG_CONFIG_PATH` to the path containing `warp-pipe.pc` file:
```
PKG_CONFIG_PATH=`pwd`/install/lib/pkgconfig cmake -S memory-mock -B build_memory_mock
```

To launch the memory-mock, use:
```
./build_memory_mock/memory_mock
```

Preparing the environment for Zephyr samples
--------------------------------------------

Use the following commands to prepare the Zephyr working environment:

<!-- name="zephyr-env-prep" -->
```
west init --local --mf zephyr-samples/warp-pipe-zephyr.yml
west update -o=--depth=1 -n
west zephyr-export
```

This environment is required to build and run samples from `zephyr-samples` folder.
More information about building each of zephyr samples can be found in respective folders.


Running Twister tests
---------------------

This repository overrides default Zephyr `west twister` command to allow running tests that requires PCIe device implementing `warp-pipe`.
It requires built [memory-mock](README.md#building-memory-mock).

<!-- name="run-twister-tests" -->
```
west twister --testsuite-root zephyr-tests/
```


Syslog
------

To view logs, use:

```
journalctl -f
```
