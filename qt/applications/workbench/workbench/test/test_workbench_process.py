# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from unittest.mock import Mock, patch

from workbench.app import workbench_process


class WorkbenchProcessQApplicationTest(unittest.TestCase):
    def setUp(self):
        # QApplication is mocked, so a real QIcon cannot be constructed
        qicon_patcher = patch.object(workbench_process, "QIcon")
        self.mock_qicon_cls = qicon_patcher.start()
        self.addCleanup(qicon_patcher.stop)

    def _mock_app(self, colour_scheme):
        mock_app = Mock()
        mock_app.styleHints.return_value.colorScheme.return_value = colour_scheme
        return mock_app

    def test_windows_dark_mode_switches_to_fusion_style(self):
        from qtpy.QtCore import Qt as QtCoreQt

        mock_app = self._mock_app(QtCoreQt.ColorScheme.Dark)

        with (
            patch.object(workbench_process, "QApplication") as mock_qapp_cls,
            patch.object(workbench_process.mtd_env, "is_linux", return_value=False),
            patch.object(workbench_process.mtd_env, "is_windows", return_value=True),
        ):
            mock_qapp_cls.instance.return_value = None
            mock_qapp_cls.return_value = mock_app

            result = workbench_process.qapplication()

        mock_app.setStyle.assert_called_once_with("Fusion")
        self.assertIs(result, mock_app)

    def test_windows_light_mode_no_fusion_styles(self):
        from qtpy.QtCore import Qt as QtCoreQt

        mock_app = self._mock_app(QtCoreQt.ColorScheme.Light)

        with (
            patch.object(workbench_process, "QApplication") as mock_qapp_cls,
            patch.object(workbench_process.mtd_env, "is_linux", return_value=False),
            patch.object(workbench_process.mtd_env, "is_windows", return_value=True),
        ):
            mock_qapp_cls.instance.return_value = None
            mock_qapp_cls.return_value = mock_app

            workbench_process.qapplication()

        mock_app.setStyle.assert_not_called()

    def test_application_window_icon_is_mantid_icon(self):
        from qtpy.QtCore import Qt as QtCoreQt

        mock_app = self._mock_app(QtCoreQt.ColorScheme.Light)

        with patch.object(workbench_process, "QApplication") as mock_qapp_cls:
            mock_qapp_cls.instance.return_value = None
            mock_qapp_cls.return_value = mock_app

            workbench_process.qapplication()

        self.mock_qicon_cls.assert_called_once_with(":/images/MantidIcon.ico")
        mock_app.setWindowIcon.assert_called_once_with(self.mock_qicon_cls.return_value)


if __name__ == "__main__":
    unittest.main()
