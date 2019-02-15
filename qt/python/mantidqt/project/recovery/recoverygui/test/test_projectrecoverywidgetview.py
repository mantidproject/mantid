# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import mock

from mantidqt.utils.qt.test import GuiTest
from mantidqt.project.recovery.recoverygui.projectrecoverywidgetview import ProjectRecoveryWidgetView


class ProjectRecoveryWidgetViewTest(GuiTest):
    def setUp(self):
        self.prw = ProjectRecoveryWidgetView(mock.MagicMock())
        self.prw.ui = mock.MagicMock()

    def test_reject(self):
        self.prw.reject()

        self.prw.presenter.start_mantid_normally.assert_called_once()

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

        self.prw.presenter.recover_last.assert_called_once()

    def test_onClickOpenLastInScriptWindow(self):
        self.prw.onClickOpenLastInScriptWindow()

        self.prw.presenter.open_last_in_editor.assert_called_once()

    def test_onClickStartMantidNormally(self):
        self.prw.onClickStartMantidNormally()

        self.prw.presenter.start_mantid_normally.assert_called_once()