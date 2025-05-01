# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import subprocess
import systemtesting

from tempfile import NamedTemporaryFile

TEST_MESSAGE = "Hello Mantid!"
SUBPROCESS_TIMEOUT_SECS = 300


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
        write_test_script(self._test_script, self._test_file)

    def runTest(self):
        arg_list = ["python", "-m", "workbench", "--execute", self._test_script, "--quit"]

        exitcode, stderr = start_and_wait_for_completion(arg_list)
        # Check for no warnings or errors on startup
        error_warning_lines = [line for line in stderr.split("\n") if "[Error]" in line or "[Warning]" in line]
        self.assertEqual([], error_warning_lines, f"stderr was warning / error output: {error_warning_lines}")
        # Assert that the test script runs successfully by writing to a .txt file
        with open(self._test_file, "r") as file:
            self.assertEqual(TEST_MESSAGE, file.readline())
        remove_file(self._test_file)
        # Was the process successful
        self.assertEqual(0, exitcode)

    def cleanup(self):
        remove_file(self._test_script)
