# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting

# Must be imported after systemtesting to avoid an error.
import sip

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.usersubwindowfactory import UserSubWindowFactory
from mantidqt.utils.qt.testing import get_application

from qtpy.QtCore import Qt


class CppInterfacesStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that the c++ interfaces open ok.
    """
    def __init__(self):
        super(CppInterfacesStartupTest, self).__init__()

        self._app = get_application()
        self._interface_manager = InterfaceManager()
        self._cpp_interface_names = UserSubWindowFactory.Instance().keys()

    def runTest(self):
        if len(self._cpp_interface_names) == 0:
            self.fail("Failed to find the names of the c++ interfaces.")

        for interface_name in self._cpp_interface_names:
            self._attempt_to_open_cpp_interface(interface_name)

    def _attempt_to_open_cpp_interface(self, interface_name):
        try:
            interface = self._interface_manager.createSubWindow(interface_name)
            interface.setAttribute(Qt.WA_DeleteOnClose, True)
            interface.show()
            interface.close()

            # Delete the interface manually because the destructor is not being called as expected on close (even with
            # Qt.WA_DeleteOnClose set to True).
            sip.delete(interface)
            self.assertTrue(sip.isdeleted(interface))

        except Exception as ex:
            self.fail(f"Exception thrown when attempting to open the {interface_name} interface: {ex}.")
