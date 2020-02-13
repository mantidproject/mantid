# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from mantid.py3compat.mock import MagicMock, patch
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.fitpropertybrowser.addfunctiondialog.presenter import AddFunctionDialog
from testhelpers import assertRaisesNothing


@start_qapplication
class AddFunctionDialogPresenterTest(unittest.TestCase):

    def test_initialization_does_not_raise(self):
        assertRaisesNothing(self, AddFunctionDialog)

    @patch('mantidqt.widgets.fitpropertybrowser.addfunctiondialog.presenter.AddFunctionDialog.function_added')
    def test_add_function(self, function_added_mock):
        dialog = AddFunctionDialog(function_names = ["Lorenzian", "Gaussian", "FlatBackground"])
        dialog.view.accept = MagicMock()
        with patch.object(dialog.view.ui.functionBox, 'currentText', lambda: "Gaussian"):
            dialog.action_add_function()
            function_added_mock.emit.assert_called_once_with("Gaussian")
            self.assertEqual(1, dialog.view.accept.call_count)

    @patch('mantidqt.widgets.fitpropertybrowser.addfunctiondialog.presenter.AddFunctionDialog.function_added')
    def test_add_function_give_error_if_function_not_valid(self, function_added_mock):
        dialog = AddFunctionDialog(function_names=["Lorenzian", "Gaussian", "FlatBackground"])
        dialog.view.accept = MagicMock()
        dialog.view.set_error_message = MagicMock()
        with patch.object(dialog.view.ui.functionBox, 'currentText', lambda: "Function"):
            dialog.action_add_function()
            self.assertEqual(1, dialog.view.set_error_message.call_count)
            self.assertEqual(0, function_added_mock.emit.call_count)
            self.assertEqual(0, dialog.view.accept.call_count)


if __name__ == '__main__':
    unittest.main()
