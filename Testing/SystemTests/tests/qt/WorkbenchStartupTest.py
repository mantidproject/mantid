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
from qtpy import PYQT5


EXECUTABLE_SWITCHER = {"linux": ["launch_mantidworkbench.sh", "mantidworkbench"],
                       "darwin": ["MantidWorkbench"],
                       "win32": ["MantidWorkbench.exe"]}


def get_mantid_executables_for_platform(platform):
    workbench_executables = EXECUTABLE_SWITCHER.get(platform, None)
    if workbench_executables is None:
        raise RuntimeError("Unknown platform {0}.".format(platform))
    return workbench_executables


def get_mantid_executable_path(platform):
    directory = ConfigService.getPropertiesDir().replace('\\', '/')
    for executable in get_mantid_executables_for_platform(platform):
        workbench_exe = os.path.join(directory, executable)
        if os.path.exists(workbench_exe):
            return workbench_exe
    raise RuntimeError("Could not find path to {0}.".format(workbench_exe))


def create_test_script(test_file_path):
    test_script_path = ConfigService.getString('defaultsave.directory') + '/test_script.py'
    with open(test_script_path, 'w') as file:
        file.write("test_file_path = '" + test_file_path + "'")
        file.write("\nwith open(test_file_path, 'w') as file:")
        file.write("\n    file.write('Hello Mantid')")
    return test_script_path


def remove_file(file_path):
    if os.path.exists(file_path):
        os.remove(file_path)


class WorkbenchStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that Mantid Workbench opens ok for Linux, MacOS and Windows.
    """
    def __init__(self):
        super(WorkbenchStartupTest, self).__init__()

        self._test_file = ConfigService.getString('defaultsave.directory') + '/test_file.txt'
        self._test_script = create_test_script(self._test_file)

        self._executable = get_mantid_executable_path(sys.platform)

        self._cmd = self._executable + " --execute " + self._test_script + " --quit"

    def skipTests(self):
        return not PYQT5

    def runTest(self):
        process = subprocess.Popen(self._cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # Assert the process was executed successfully
        self.assertTrue(process.poll() == 0)
        # Assert that the test script runs successfully by creating a .txt file
        self.assertTrue(os.path.exists(self._test_file))

    def cleanup(self):
        remove_file(self._test_script)
        remove_file(self._test_file)
