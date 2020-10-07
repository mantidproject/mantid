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


EXECUTABLE_SWITCHER = {"linux": ["launch_mantidworkbench.sh", "mantidworkbench"],
                       "darwin": ["MantidWorkbench"],
                       "win32": ["MantidWorkbench.exe"]}


def get_mantid_executables_for_platform(platform):
    workbench_executables = EXECUTABLE_SWITCHER.get(platform, None)
    if workbench_executables is None:
        raise RuntimeError(f"Unknown platform {platform}.")
    return workbench_executables


def get_mantid_executable_path(platform):
    directory = ConfigService.getPropertiesDir().replace('\\', '/')
    for executable in get_mantid_executables_for_platform(platform):
        workbench_exe = os.path.join(directory, executable)
        if os.path.exists(workbench_exe):
            return workbench_exe
    raise RuntimeError(f"Could not find path to {workbench_exe}.")


def create_test_script(test_file_path):
    test_script_path = NamedTemporaryFile(suffix=".py").name
    with open(test_script_path, "w") as file:
        file.write(f"test_file_path = '{test_file_path}'")
        file.write("\nwith open(test_file_path, 'w') as file:")
        file.write("\n    file.write('Hello Mantid')")
    return test_script_path


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

        #self._test_file = NamedTemporaryFile(suffix=".txt").name()
        self._test_file = ConfigService.getString("defaultsave.directory") + "/test_file.txt"
        self._test_script = create_test_script(self._test_file)

        self._executable = get_mantid_executable_path(sys.platform)

    def runTest(self):
        process = subprocess.Popen([self._executable, "--execute", self._test_script, "--quit"],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # Wait for the process to finish execution, and assert it was successfully
        self.assertTrue(process.wait() == 0)
        # Assert that the test script runs successfully by creating a .txt file
        self.assertTrue(os.path.exists(self._test_file))

    def cleanup(self):
        remove_file(self._test_script)
        remove_file(self._test_file)
