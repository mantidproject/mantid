# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import tempfile

import systemtesting

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt.testing import get_application
from qtpy import sip
from qtpy.QtCore import QCoreApplication, QSettings, QSignalBlocker, QStandardPaths, Qt
from qtpy.QtWidgets import QLineEdit, QWidget


class DiffractionSettingsLifecycleTest(systemtesting.MantidSystemTest):
    """Verify the normal Workbench window-shutdown path persists Diffraction settings."""

    def runTest(self):
        self._application = get_application()  # Keep QApplication alive for the interface lifetime.
        interface_manager = InterfaceManager()
        original_application = QCoreApplication.applicationName()
        original_organization = QCoreApplication.organizationName()
        original_format = QSettings.defaultFormat()
        interface = None

        with tempfile.TemporaryDirectory() as settings_directory:
            try:
                QCoreApplication.setApplicationName("DiffractionSettingsLifecycleTest")
                QCoreApplication.setOrganizationName("MantidProjectTest")
                QSettings.setDefaultFormat(QSettings.IniFormat)
                QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, settings_directory)

                interface = interface_manager.createSubWindow("Diffraction")
                interface.setAttribute(Qt.WA_DeleteOnClose, True)
                interface.show()

                calibration_finder = interface.findChild(QWidget, "rfCalFile")
                vanadium_finder = interface.findChild(QWidget, "rfVanFile")
                self.assertTrue(calibration_finder is not None)
                self.assertTrue(vanadium_finder is not None)
                calibration = calibration_finder.findChild(QLineEdit, "fileEditor")
                vanadium = vanadium_finder.findChild(QLineEdit, "fileEditor")
                self.assertTrue(calibration is not None)
                self.assertTrue(vanadium is not None)
                calibration_blocker = QSignalBlocker(calibration)
                vanadium_blocker = QSignalBlocker(vanadium)
                calibration.setText("/lifecycle/calibration.cal")
                vanadium.setText("100-102,105")
                del calibration_blocker
                del vanadium_blocker
                del calibration
                del vanadium
                del calibration_finder
                del vanadium_finder

                interface.close()
                sip.delete(interface)

                self.assertTrue(sip.isdeleted(interface), "Diffraction interface was not destroyed during shutdown")

                settings = QSettings()
                settings.beginGroup("CustomInterfaces/DEMON")
                self.assertEqual(settings.value("last_cal_file"), "/lifecycle/calibration.cal")
                self.assertEqual(settings.value("last_van_files"), "100-102,105")
                settings.endGroup()
            finally:
                QCoreApplication.setApplicationName(original_application)
                QCoreApplication.setOrganizationName(original_organization)
                QSettings.setDefaultFormat(original_format)
                QSettings.setPath(
                    QSettings.IniFormat,
                    QSettings.UserScope,
                    QStandardPaths.writableLocation(QStandardPaths.GenericConfigLocation),
                )
