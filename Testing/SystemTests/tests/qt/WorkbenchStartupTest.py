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


class WorkbenchStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that Mantid Workbench opens ok for Linux, MacOS and Windows.
    """
    def __init__(self):
        super(WorkbenchStartupTest, self).__init__()

        self._executable = get_mantid_executable_path(sys.platform)

    def runTest(self):
        process = subprocess.Popen(self._executable, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # Assert that mantid workbench is running
        self.assertTrue(process.poll() is None)

        out, _ = process.communicate()
        self.assertTrue(b'ConfigService-[Information] This is Mantid' in out)

        # Assert that mantid workbench has stopped running
        process.kill()
        self.assertTrue(process.poll())
