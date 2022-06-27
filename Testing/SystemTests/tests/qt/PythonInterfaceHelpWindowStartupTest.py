# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from systemtesting import MantidSystemTest

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt.testing import get_application


class PythonInterfaceHelpWindowStartupTest(MantidSystemTest):
    """
    A system test for testing that the python interface help windows open successfully.
    """

    def __init__(self):
        super(InterfaceHelpWindowStartupTest, self).__init__()
        self._interface_manager = InterfaceManager()

        self._interfaces = [interface.split("/") for interface in ConfigService.getString('mantidqt.python_interfaces').split()]

    def runTest(self):
        if len(self._interfaces) == 0:
            self.fail("Failed to find the names of the python interfaces.")

        self._qapp = get_application()  # keep QApp reference alive
        try:
            for category, interface_name in self._interfaces:
                self._attempt_to_open_help_window(category, interface_name)
        finally:
            self._qapp = None

        # Open all interface help windows
        # Open interface help window multiple times without a parent
        # Open interface help window multiple times with a parent widget

    def _attempt_to_open_help_window(self, category, interface_name):
        try:
            url = f"qthelp://org.mantidproject/doc/interfaces/{category}/{interface_name}.html"
            self._interface_manager.showHelpPage(url)
            self._close_interface()
        except Exception as ex:
            self.fail(f"Exception thrown when attempting to open help page for {interface_name}: {ex}.")

    def _close_interface(self):
        """
        Close all opened windows after opening to ensure it is unsubscribed from the ADS. Must be done before the
        next interface is opened because it appears the Qt objects of the previous interface get deleted when a
        python script runs to completion, but the python object itself is not. This means it is still subscribed to the
        ADS because the 'close_event' is not called, but the Qt objects no longer exist. This causes problems when
        an ADS change is observed.
        """
        self._qapp.closeAllWindows()
        self._qapp.sendPostedEvents()
