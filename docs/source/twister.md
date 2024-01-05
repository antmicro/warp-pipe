# Testing with Twister

This repository overrides default Zephyr `west twister` command to allow running tests that requires PCIe device implementing `warp-pipe`.
It requires [memory-mock](#memory-mock) to be built and available in the `build_memory_mock` directory.

(zephyr-setup)=
## Setting up Zephyr environment

To run any Zephyr samples and tests from this repository, initialize the Zephyr environment with `west`.

```
west init --local --mf zephyr-samples/warp-pipe-zephyr.yml
west update -o=--depth=1 -n
west zephyr-export
```

## Running Twister

To start tests defined in a suite from the `zephyr-tests` directory, run:

```
west twister --testsuite-root zephyr-tests/
```

You can expect the following output of a test suite:

```
INFO    - Using Ninja..
INFO    - Zephyr version: f89c5ddd1aaf
INFO    - Using 'zephyr' toolchain.
INFO    - Selecting default platforms per test case
INFO    - Building initial testsuite list...
INFO    - Writing JSON report /__w/warp-pipe/warp-pipe/twister-out/testplan.json
INFO    - JOBS: 4
INFO    - Adding tasks to the queue...
INFO    - Added initial list of jobs to queue
INFO    - Total complete:    1/   1  100%  skipped:    0, failed:    0, error:    0
INFO    - 1 test scenarios (1 test instances) selected, 0 configurations skipped (0 by static filter, 0 at runtime).
INFO    - 1 of 1 test configurations passed (100.00%), 0 failed, 0 errored, 0 skipped with 0 warnings in 15.76 seconds
INFO    - In total 4 test cases were executed, 0 skipped on 1 out of total 647 platforms (0.15%)
INFO    - 1 test configurations executed on platforms, 0 test configurations were only built.
INFO    - Saving reports...
INFO    - Writing JSON report /__w/warp-pipe/warp-pipe/twister-out/twister.json
INFO    - Writing xunit report /__w/warp-pipe/warp-pipe/twister-out/twister.xml...
INFO    - Writing xunit report /__w/warp-pipe/warp-pipe/twister-out/twister_report.xml...
INFO    - Run completed
```

## Test configuration

This version of Zephyr Twister provides a `warppipe` harness.

The definition of a sample test (stored in `testcase.yaml`) is as follows:

```
tests:
  testing.ztest:
    harness: warppipe
    harness_config:
      type: ../build_memory_mock/memory_mock
    platform_allow:
      - native_sim_64
    tags: tests
```

```{note}
The `type` field in `harness_config` is used to provide a device implementation.

While the name of this field is not descriptive, it is used to avoid changing the YML schema in Twister.
```