# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import systemtesting

from mantid.kernel import ConfigService
from mantidqt.utils.qt.testing import get_application

from qtpy.QtCore import QCoreApplication, QSettings


class PythonInterfacesStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that the python interfaces open ok.
    """
    def __init__(self):
        super(PythonInterfacesStartupTest, self).__init__()

        self._app = get_application()

        interface_directory = ConfigService.getString('mantidqt.python_interfaces_directory')
        self._python_interface_paths = [os.path.join(interface_directory, interface.split("/")[1]) for interface in
                                        ConfigService.getString('mantidqt.python_interfaces').split()]

        # Prevents a QDialog popping up when opening the DGS Reduction interface
        QCoreApplication.setOrganizationName(" ")
        QCoreApplication.setApplicationName("python")
        QSettings().setValue("instrument_name", "ARCS")

    def runTest(self):
        if len(self._python_interface_paths) == 0:
            self.fail("Failed to find the paths to any of the python interfaces.")

        for interface_path in self._python_interface_paths:
            self._attempt_to_open_python_interface(interface_path)

    def _attempt_to_open_python_interface(self, interface_path):
        try:
            exec(open(interface_path).read())
            exit()
        except Exception as ex:
            self.fail("Exception thrown when attempting to open the {0} interface: {1}".format(interface_path, str(ex)))
