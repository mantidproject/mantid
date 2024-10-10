# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import subprocess
import sys
import systemtesting

from mantid.kernel import ConfigService
from tempfile import NamedTemporaryFile

TEST_MESSAGE = "Hello Mantid!"
EXECUTABLE_SWITCHER = {
    "linux": ["launch_mantidworkbench.sh", "workbench"],
    "darwin": ["workbench"],
    "win32": ["workbench"],
}
SUBPROCESS_TIMEOUT_SECS = 300


def get_mantid_executables_for_platform(platform):
    workbench_executables = EXECUTABLE_SWITCHER.get(platform, None)
    if workbench_executables is None:
        raise RuntimeError(f"Unknown platform {platform}.")
    return workbench_executables


def start_and_wait_for_completion(args_list):
    process = subprocess.Popen(args_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    exitcode = process.wait(timeout=SUBPROCESS_TIMEOUT_SECS)
    return exitcode, process.stderr.read().decode()


def write_test_script(test_script, test_file):
    with open(test_script, "w") as file:
        file.write(f"\nwith open('{test_file}', 'w') as file:")
        file.write(f"\n    file.write('{TEST_MESSAGE}')")


def remove_file(file_path):
    if os.path.exists(file_path):
        try:
            os.remove(file_path)
        except OSError as ex:
            print(f"Error removing {file_path}: {ex}.")


class WorkbenchStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that Mantid Workbench opens ok for Linux, MacOS and Windows.
    """

    def __init__(self):
        super(WorkbenchStartupTest, self).__init__()

        self._test_file = NamedTemporaryFile(suffix=".txt", delete=False).name.replace("\\", "/")
        self._test_script = NamedTemporaryFile(suffix=".py", delete=False).name.replace("\\", "/")
        self._executables = get_mantid_executables_for_platform(sys.platform)
        write_test_script(self._test_script, self._test_file)

    def runTest(self):
        directory = ConfigService.getPropertiesDir().replace("\\", "/")
        for executable in self._executables:
            file_path = os.path.join(directory, executable)
            executable, module = (file_path, False) if os.path.exists(file_path) else (executable, True)
            arg_list = [executable, "--execute", self._test_script, "--quit"]
            if module:
                arg_list = ["python", "-m"] + arg_list

            exitcode, stderr = start_and_wait_for_completion(arg_list)
            # Was the process successful
            self.assertEqual(0, exitcode)
            # Check for no warnings or errors on startup
            error_warning_lines = [line for line in stderr.split("\n") if "[Error]" in line or "[Warning]" in line]
            self.assertEqual([], error_warning_lines, f"stderr was warning / error output: {error_warning_lines}")
            # Assert that the test script runs successfully by writing to a .txt file
            with open(self._test_file, "r") as file:
                self.assertEqual(TEST_MESSAGE, file.readline())
            remove_file(self._test_file)

    def cleanup(self):
        remove_file(self._test_script)
