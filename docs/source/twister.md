# Testing with Twister

This repository overrides default Zephyr `west twister` command to allow running tests that requires PCIe device implementing `warp-pipe`.
It requires built [memory-mock](#memory-mock).

```
west twister --testsuite-root zephyr-tests/
```
