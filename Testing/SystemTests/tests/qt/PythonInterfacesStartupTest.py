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

# Frequency_Domain_Analysis_Old.py  -  Excluded because is being deleted in Mantid Version 6.0
# Frequency_Domain_Analysis.py      -  Excluded because it is causing a crash
EXCLUDED_SCRIPTS = ["Frequency_Domain_Analysis_Old.py", "Frequency_Domain_Analysis.py"]

INSTRUMENT_SWITCHER = {"DGS_Reduction.py": "ARCS", "ORNL_SANS.py": "EQSANS", "Powder_Diffraction_Reduction.py": "NOM"}

APP_NAME_SWITCHER = {
    "DGS_Reduction.py": "python",
    "ORNL_SANS.py": "python",
    "Powder_Diffraction_Reduction.py": "python"
}


def set_instrument(interface_script_name):
    instrument = INSTRUMENT_SWITCHER.get(interface_script_name, None)
    if instrument is not None:
        QSettings().setValue("instrument_name", instrument)


def set_application_name(interface_script_name):
    app_name = APP_NAME_SWITCHER.get(interface_script_name, "mantid")
    QCoreApplication.setOrganizationName(" ")
    QCoreApplication.setApplicationName(app_name)


class PythonInterfacesStartupTest(systemtesting.MantidSystemTest):
    """
    A system test for testing that the python interfaces open ok.
    """
    def __init__(self):
        super(PythonInterfacesStartupTest, self).__init__()

        self._app = get_application()

        self._interface_directory = ConfigService.getString('mantidqt.python_interfaces_directory')
        self._interface_scripts = [
            interface.split("/")[1] for interface in ConfigService.getString('mantidqt.python_interfaces').split()
            if interface.split("/")[1] not in EXCLUDED_SCRIPTS
        ]

    def runTest(self):
        if len(self._interface_scripts) == 0:
            self.fail("Failed to find any python interface scripts.")

        for interface_script in self._interface_scripts:
            self._attempt_to_open_python_interface(interface_script)

    def _attempt_to_open_python_interface(self, interface_script):
        # Ensures the interfaces close after their script has been executed
        set_application_name(interface_script)
        # Prevents a QDialog popping up when opening certain interfaces
        set_instrument(interface_script)

        try:
            exec(open(os.path.join(self._interface_directory, interface_script)).read())
        except Exception as ex:
            self.fail(f"Exception thrown when attempting to open the {interface_script} interface: {ex}.")
