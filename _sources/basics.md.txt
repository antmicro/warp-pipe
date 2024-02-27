# Basic Warp Pipe usage

## Building warp-pipe

To build the library to be installed system-wide you need to run the following commands:

```
mkdir -p build
cmake -S . -B build
make -j $(nproc) -C build
```

If you prefer installing in another directory (e.g. `install`), set the `CMAKE_INSTALL_PREFIX` variable for CMake:

```
mkdir -p build
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=`pwd`/install
make -j $(nproc) -C build
```

After this completes, install the library with:

```
make install -C build
```

```{note}
Please keep in mind that if you installed warp-pipe to a different directory, tools that rely on pkgconfig and want to link warp-pipe may require the `PKG_CONFIG_PATH` variable to point to the installation directory containing the `warp-pipe.pc` file (e.g. `install/lib/pkgconfig`).
```

## Running tests

Tests are build by default while building `warp-pipe` library. To execute tests, run:

```
./build/warppipe-tests
```

## Generating coverage report

To generate tests coverage report, use the following snippet:

```
./scripts/coverage.sh
```

Generated coverage report can be found in the `coverage` folder.

## Viewing warp-pipe logs

As warp-pipe is a library, it does not print to the console by itself.

It uses system logging infrastructure instead.

To see these logs, use:

```
journalctl -f
```

Look for a similar output:

```
Jan 01 02:57:56 host warp-pipe[1255430]: Starting Warp Pipe...
Jan 01 02:57:56 host warp-pipe[1255430]: Warp Pipe listening on 0.0.0.0:2115.
```

```{note}
Keep in mind that the source name may differ, depending on the way you use `warp-pipe` within your application.
```