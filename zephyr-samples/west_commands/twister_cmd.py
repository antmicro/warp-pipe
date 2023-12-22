# Copyright (c) 2023 Google
# Copyright (c) 2018-2022 Intel Corporation
# Copyright 2022 NXP
# Copyright 2024 Antmicro <www.antmicro.com>
# Copyright 2024 Meta
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


# This script is based on `twister_cmd.py` from Zephyr repository
# with injections that allows to run warp-pipe harness. 

import argparse
import os
import sys
from pathlib import Path

from west import log
from west.commands import WestCommand

THIS_ZEPHYR = Path(__file__).parent.parent.parent.parent / "zephyr"
ZEPHYR_BASE = Path(os.environ.get('ZEPHYR_BASE', THIS_ZEPHYR))
os.environ['ZEPHYR_BASE'] = str(ZEPHYR_BASE)
twister_path = ZEPHYR_BASE / "scripts"

sys.path.insert(0, str(twister_path))
sys.path.insert(0, str(twister_path / "west_commands"))
sys.path.insert(0, str(twister_path / "pylib" / "twister"))

from twisterlib.environment import add_parse_arguments, parse_arguments
from twisterlib.twister_main import main
import subprocess
import twisterlib.harness
import twisterlib.testinstance
import logging
import time
import shlex
import threading
import twisterlib.handlers
import twister_cmd

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

class Warppipe(twisterlib.harness.Ztest):

    is_warppipe_test = True

    def configure(self, instance):
        super(Warppipe, self).configure(instance)
        self.instance = instance

        config = instance.testsuite.harness_config
        if config:
            self.path = instance.testsuite.source_dir + os.sep + config.get('type', None)

    def run_warppipe_test(self, command, handler):

        env = os.environ.copy()

        with subprocess.Popen(self.path, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, cwd=self.instance.build_dir, env=env) as memory_mock_proc:
            with subprocess.Popen(command, stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT, cwd=self.instance.build_dir, env=env) as cmake_proc:
                out, _ = cmake_proc.communicate()

                if out:
                    with open(os.path.join(self.instance.build_dir, handler.log), "wt") as log:
                        log_msg = out.decode(sys.getdefaultencoding())
                        for line in log_msg.splitlines():
                            self.handle(line)
                        log.write(log_msg)
                memory_mock_proc.kill()

# Register custom Warppipe harness in twisterlib
twisterlib.harness.Warppipe = Warppipe

# based on zephyr/scripts/pylib/twister/twisterlib/testinstance.py
@staticmethod
def testsuite_runnable(testsuite, fixtures):
    can_run = False
    # console harness allows us to run the test and capture data.
    if testsuite.harness in [ 'console', 'ztest', 'pytest', 'test', 'gtest', 'robot',
    'warppipe']:
        can_run = True
        # if we have a fixture that is also being supplied on the
        # command-line, then we need to run the test, not just build it.
        fixture = testsuite.harness_config.get('fixture')
        if fixture:
            can_run = fixture in fixtures

    return can_run

# Override function from TestInstance to allow warppipe harness
twisterlib.testinstance.TestInstance.testsuite_runnable = testsuite_runnable

# based on zephyr/scripts/pylib/twister/twisterlib/handlers.py
def handle(self, harness):
    robot_test = getattr(harness, "is_robot_test", False)
    warppipe_test = getattr(harness, "is_warppipe_test", False)

    command = self._create_command(robot_test)

    logger.debug("Spawning process: " +
                 " ".join(shlex.quote(word) for word in command) + os.linesep +
                 "in directory: " + self.build_dir)

    start_time = time.time()

    env = self._create_env()

    if robot_test:
        harness.run_robot_test(command, self)
        return

    if warppipe_test:
        harness.run_warppipe_test(command, self)
        handler_time = time.time() - start_time
        self._update_instance_info(harness.state, handler_time)
        return

    with subprocess.Popen(command, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, cwd=self.build_dir, env=env) as proc:
        logger.debug("Spawning BinaryHandler Thread for %s" % self.name)
        t = threading.Thread(target=self._output_handler, args=(proc, harness,), daemon=True)
        t.start()
        t.join()
        if t.is_alive():
            self.terminate(proc)
            t.join()
        proc.wait()
        self.returncode = proc.returncode
        self.try_kill_process_by_pid()

    handler_time = time.time() - start_time

    if self.options.coverage:
        subprocess.call(["GCOV_PREFIX=" + self.build_dir,
                         "gcov", self.sourcedir, "-b", "-s", self.build_dir], shell=True)

    # FIXME: This is needed when killing the simulator, the console is
    # garbled and needs to be reset. Did not find a better way to do that.
    if sys.stdout.isatty():
        subprocess.call(["stty", "sane"], stdin=sys.stdout)

    self._update_instance_info(harness.state, handler_time)

    self._final_handle_actions(harness, handler_time)

# Override function from BinaryHandler to enable warppipe handle
twisterlib.handlers.BinaryHandler.handle = handle

class Twister:
    pass

# Override dummy Twister class with original one
Twister = twister_cmd.Twister
