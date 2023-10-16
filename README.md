PCIe Communication Server
=========================

```
Copyright 2023 Antmicro <www.antmicro.com>
Copyright 2023 Meta

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

Building
--------

To build the PCIe Communication Server, use the following snippet:

```
mkdir -p build
cmake -S . -B build
make -j $(nproc) -C build
```

To launch the server, use:
```
./build/server
```

Syslog
------

To view logs, use:

```
journalctl -t pcied -f
```
