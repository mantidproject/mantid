# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from itertools import chain

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt.testing import get_application
from workbench.config import CONF, WINDOW_ONTOP_FLAGS, WINDOW_STANDARD_FLAGS
from workbench.utils.gather_interfaces import gather_cpp_interface_names

from qtpy.QtCore import Qt

# Import sip after Qt. Modern versions of PyQt ship an internal sip module
# located at PyQt5X.sip. Importing PyQt first sets a shim sip module to point
# to the correct place
import sip


class CppInterfacesStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that the c++ interfaces open ok.
    """

    def __init__(self):
        super(CppInterfacesStartupTest, self).__init__()

        self._app = get_application()
        self._interface_manager = InterfaceManager()
        self._cpp_interface_names = set(chain.from_iterable(gather_cpp_interface_names().values()))

    def runTest(self):
        if len(self._cpp_interface_names) == 0:
            self.fail("Failed to find the names of the c++ interfaces.")

        self._open_interfaces("On Top")
        self._open_interfaces("Floating")

    def _open_interfaces(self, window_behaviour):
        CONF.set("AdditionalWindows/behaviour", window_behaviour)
        for interface_name in self._cpp_interface_names:
            self._attempt_to_open_cpp_interface(interface_name, window_behaviour)

    def _attempt_to_open_cpp_interface(self, interface_name, window_behaviour):
        try:
            interface = self._interface_manager.createSubWindow(interface_name)
            interface.setAttribute(Qt.WA_DeleteOnClose, True)
            interface.show()
            if window_behaviour == "On Top":
                self.assertTrue(
                    interface.windowFlags() & WINDOW_ONTOP_FLAGS,
                    f"{interface_name} is not using correct window flags for 'On Top' behaviour",
                )
            else:
                self.assertTrue(
                    interface.windowFlags() & WINDOW_STANDARD_FLAGS,
                    f"{interface_name} is not using correct window flags for 'Floating' behaviour",
                )
            interface.close()

            # Delete the interface manually because the destructor is not being called as expected on close (even with
            # Qt.WA_DeleteOnClose set to True).
            sip.delete(interface)
            self.assertTrue(sip.isdeleted(interface))

        except Exception as ex:
            self.fail(f"Exception thrown when attempting to open the {interface_name} interface: {ex}.")
