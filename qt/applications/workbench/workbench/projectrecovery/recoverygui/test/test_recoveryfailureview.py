# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

from __future__ import (absolute_import, unicode_literals)

import unittest

from qtpy.QtWidgets import QTableWidgetItem

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from workbench.projectrecovery.recoverygui.recoveryfailureview import RecoveryFailureView


@start_qapplication
class RecoveryFailureViewTest(unittest.TestCase):
    def setUp(self):
        self.prw = RecoveryFailureView(mock.MagicMock())

        self.prw.ui = mock.MagicMock()

    def test_reject(self):
        self.prw.reject()

        self.assertEqual(1, self.prw.presenter.start_mantid_normally.call_count)

    def test_set_progress_bar_maximum(self):
        self.prw.set_progress_bar_maximum(1)

        self.prw.ui.progressBar.setMaximum.assert_called_with(1)

    def test_change_start_mantid_button(self):
        self.prw.change_start_mantid_button("123")

        self.prw.ui.pushButton_3.setText.assert_called_with("123")

    def test_update_progress_bar(self):
        self.prw.update_progress_bar(1)

        self.prw.ui.progressBar.setValue.assert_called_with(1)

    def test_onClickLastCheckpoint(self):
        self.prw.onClickLastCheckpoint()

        self.prw.presenter.recover_last()

    def test_onClickSelectedCheckpoint(self):
        self.prw.ui.tableWidget.selectedItems.return_value = \
            [QTableWidgetItem("1"), QTableWidgetItem("2"), QTableWidgetItem("No")]
        self.prw.onClickSelectedCheckpoint()

        self.prw.presenter.recover_selected_checkpoint.assert_called_with("1")

    def test_onClickOpenSelectedInScriptWindow(self):
        self.prw.ui.tableWidget.selectedItems.return_value = \
            [QTableWidgetItem("1"), QTableWidgetItem("2"), QTableWidgetItem("No")]
        self.prw.onClickOpenSelectedInScriptWindow()

        self.prw.presenter.open_selected_checkpoint_in_editor.assert_called_with("1")

    def test_onClickStartMantidNormally(self):
        self.prw.onClickStartMantidNormally()

        self.assertEqual(1, self.prw.presenter.start_mantid_normally.call_count)
