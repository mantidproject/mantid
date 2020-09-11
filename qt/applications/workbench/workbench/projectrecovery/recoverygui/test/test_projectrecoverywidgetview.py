# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from workbench.projectrecovery.recoverygui.projectrecoverywidgetview import ProjectRecoveryWidgetView


@start_qapplication
class ProjectRecoveryWidgetViewTest(unittest.TestCase):
    def setUp(self):
        self.prw = ProjectRecoveryWidgetView(mock.MagicMock())
        self.prw.ui = mock.MagicMock()

    def test_reject(self):
        self.prw.reject()

        self.assertEqual(1, self.prw.presenter.start_mantid_normally.call_count)

    def test_set_progress_bar_maximum(self):
        self.prw.set_progress_bar_maximum(1)

        self.prw.ui.progressBar.setMaximum.assert_called_with(1)

    def test_change_start_mantid_button(self):
        self.prw.change_start_mantid_button("123")

        self.prw.ui.startmantidButton.setText.assert_called_with("123")

    def test_update_progress_bar(self):
        self.prw.update_progress_bar(1)

        self.prw.ui.progressBar.setValue.assert_called_with(1)

    def test_onClickLastCheckpoint(self):
        self.prw.onClickLastCheckpoint()

        self.assertEqual(1, self.prw.presenter.recover_last.call_count)

    def test_onClickOpenLastInScriptWindow(self):
        self.prw.onClickOpenLastInScriptWindow()

        self.assertEqual(1, self.prw.presenter.open_last_in_editor.call_count)

    def test_onClickStartMantidNormally(self):
        self.prw.onClickStartMantidNormally()

        self.assertEqual(1, self.prw.presenter.start_mantid_normally.call_count)

    def test_progress_bar_connection_is_attempted(self):
        self.prw.connect_progress_bar()

        self.assertEqual(1,
                         self.prw.presenter.project_recovery.loader.multi_file_interpreter.current_editor.call_count)
        self.prw.editor.connect_to_progress_reports.assert_called_once_with(self.prw.update_progress_bar)

    def test_progress_bar_disconnect_is_attempted_on_exit_if_editor_present(self):
        self.prw.editor = mock.MagicMock()
        self.prw.close()

        self.assertEqual(1, self.prw.editor.disconnect_from_progress_reports.call_count)

    def test_progress_bar_disconnect_is_not_attempted_on_exit_if_editor_is_not_present(self):
        # This would test would raise an AttributeError if it attempted to make a call on None
        self.prw.editor = None
        self.prw.close()
