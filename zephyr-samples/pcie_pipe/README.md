# PCIe PIPE

This samples demonstrates the functionality of the ``warp-pipe`` device from QEMU using warp-pipe.

## Preparing the environment

Please refer to [preparing the environment for Zephyr samples](../../README.md#preparing-the-environment-for-zephyr-samples) in main README.

## Building and running

This sample requires installed `warp-pipe` library.
Please refer to [installing warp-pipe](../../README.md#building-and-installing-warp-pipe) for more information.

This sample requires also custom QEMU with warp-pipe integration. Use following commands to build it:
<!-- name="pcie-qemu-build" -->
```
mkdir qemu/build
pushd qemu/build
../configure --enable-warp-pipe --target-list=x86_64-softmmu
make -j$(nproc)
export QEMU_BIN_PATH=`pwd`
popd
```

Use the following command to build this sample in both server and client mode:
<!-- name="pcie-pipe-build" -->
```
west build -d build.pcie_pipe_server/ -b qemu_x86_64 zephyr-samples/pcie_pipe/ -- -DWarpPipeConnection=listen
west build -d build.pcie_pipe_client/ -b qemu_x86_64 zephyr-samples/pcie_pipe/ -- -DWarpPipeConnection=connect
```

Now you can launch both executables:
<!-- name="pcie-pipe-run" -->
```
west build -t run -d build.pcie_pipe_server/ < /dev/null &
SERVER_PID=$!
sleep 2 # wait for server to start
west build -t run -d build.pcie_pipe_client/ < /dev/null &
CLIENT_PID=$!
```

You should see the following output on the standard output console:
```
Initial memory values:
0x3020100 0x7060504 0xb0a0908 0xf0e0d0c 0x13121110 0x17161514 0x1b1a1918 0x1f1e1d1c
0x23222120 0x27262524 0x2b2a2928 0x2f2e2d2c 0x33323130 0x37363534 0x3b3a3938 0x3f3e3d3c
0x43424140 0x47464544 0x4b4a4948 0x4f4e4d4c 0x53525150 0x57565554 0x5b5a5958 0x5f5e5d5c
0x63626160 0x67666564 0x6b6a6968 0x6f6e6d6c 0x73727170 0x77767574 0x7b7a7978 0x7f7e7d7c
Updated memory values:
0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab
0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab
0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab
0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab 0xabababab
Allocated 1 vectors
1 MSI-X Vectors connected
Received MSI-X IRQ!
Received MSI-X IRQ!
```

After that, you can terminate all background processes:
<!-- name="pcie-pipe-teardown" -->
```
sleep 120
kill -INT $CLIENT_PID
kill -INT $SERVER_PID
```
