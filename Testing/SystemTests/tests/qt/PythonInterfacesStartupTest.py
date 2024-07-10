# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from itertools import chain
import os
import systemtesting

from mantidqt.utils.qt.testing import get_application
from workbench.utils.gather_interfaces import gather_python_interface_names
from workbench.config import CONF

from qtpy.QtCore import QCoreApplication, QSettings

ORGANIZATION_NAME = " "

INSTRUMENT_SWITCHER = {"DGS_Reduction.py": "ARCS", "ORNL_SANS.py": "EQSANS", "Powder_Diffraction_Reduction.py": "NOM"}

APP_NAME_SWITCHER = {"DGS_Reduction.py": "python", "ORNL_SANS.py": "python", "Powder_Diffraction_Reduction.py": "python"}


def set_instrument(interface_script_name):
    instrument = INSTRUMENT_SWITCHER.get(interface_script_name, None)
    if instrument is not None:
        app_name = APP_NAME_SWITCHER.get(interface_script_name, "mantid")
        QSettings(ORGANIZATION_NAME, app_name).setValue("instrument_name", instrument)


def set_application_name(interface_script_name):
    app_name = APP_NAME_SWITCHER.get(interface_script_name, "mantid")
    QCoreApplication.setOrganizationName(ORGANIZATION_NAME)
    QCoreApplication.setApplicationName(app_name)


class PythonInterfacesStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that the python interfaces open ok.
    """

    def __init__(self):
        super(PythonInterfacesStartupTest, self).__init__()
        import mantidqtinterfaces

        self._interface_directory = os.path.dirname(mantidqtinterfaces.__file__)
        # The order of the scripts in this iterable is random
        self._interface_scripts = set(chain.from_iterable(gather_python_interface_names().values()))

    def runTest(self):
        if len(self._interface_scripts) == 0:
            self.fail("Failed to find any python interface scripts.")

        self._qapp = get_application()  # keep QApp reference alive
        try:
            self._open_interfaces("On Top")
            self._open_interfaces("Floating")
        finally:
            self._qapp = None

    def _open_interfaces(self, window_behaviour):
        CONF.set("AdditionalWindows/behaviour", window_behaviour)
        for interface_script in self._interface_scripts:
            self._attempt_to_open_and_close_python_interface(interface_script)

    def _attempt_to_open_and_close_python_interface(self, interface_script):
        # Ensures the interfaces close after their script has been executed
        set_application_name(interface_script)
        # Prevents a QDialog popping up when opening certain interfaces
        set_instrument(interface_script)

        try:
            exec(open(os.path.join(self._interface_directory, interface_script)).read())
            self._close_interface()
        except Exception as ex:
            self.fail(f"Exception thrown when attempting to open the {interface_script} interface: {ex}.")

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
