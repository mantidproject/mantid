#
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, unicode_literals

import unittest

from mantid import UsageService
from mantid.py3compat.mock import patch
from mantidqt.utils.testing.mocks.mock_qt import MockQWidget
from workbench.plugins.exception_handler import exception_logger


class ExceptionHandlerTest(unittest.TestCase):
    @classmethod
    def tearDownClass(cls):
        UsageService.setEnabled(False)

    @patch('workbench.plugins.exception_handler.WorkbenchErrorMessageBox')
    @patch('workbench.plugins.exception_handler.logger')
    def test_exception_logged_no_UsageService(self, mock_logger, mock_WorkbenchErrorMessageBox):
        UsageService.setEnabled(False)

        widget = MockQWidget()
        mock_errorbox = MockQWidget()
        mock_WorkbenchErrorMessageBox.return_value = mock_errorbox

        exception_logger(widget, ValueError, None, None)

        self.assertEqual(1, mock_logger.error.call_count)
        self.assertEqual(1, mock_WorkbenchErrorMessageBox.call_count)
        mock_errorbox.exec_.assert_called_once_with()

    @patch('workbench.plugins.exception_handler.CrashReportPage')
    @patch('workbench.plugins.exception_handler.logger')
    def test_exception_logged(self, mock_logger, mock_CrashReportPage):
        UsageService.setEnabled(True)

        widget = MockQWidget()

        exception_logger(widget, ValueError, None, None)

        self.assertEqual(1, mock_logger.error.call_count)
        mock_CrashReportPage.assert_called_once_with(show_continue_terminate=True)
        # 'user selects' continue working by default
        self.assertEqual(0, widget.close.call_count)
