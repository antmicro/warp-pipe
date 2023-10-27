#!/usr/bin/env bash

# Copyright 2023 Antmicro <www.antmicro.com>
# Copyright 2023 Meta
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -euxo pipefail

mkdir -p coverage
lcov --include '*/src/*' --no-external -c -i -d . -o coverage/lcov_base.info
lcov --include '*/src/*' --no-external -c -d . -o coverage/lcov_test.info
genhtml --legend -o coverage --title='PCIe Communication Server' coverage/lcov_base.info coverage/lcov_test.info
