# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from mantid.py3compat.mock import patch
from mantidqt.widgets.fitpropertybrowser.addfunctiondialog.presenter import AddFunctionDialog
from testhelpers import assertRaisesNothing


@patch('mantidqt.widgets.fitpropertybrowser.addfunctiondialog.view.AddFunctionDialogView')
class AddFunctionDialogPresenterTest(unittest.TestCase):

    def test_initialization_does_not_raise(self, mock_view):
        assertRaisesNothing(self, AddFunctionDialog, view=mock_view)

    def test_add_function(self, mock_view):
        dialog = AddFunctionDialog(view=mock_view)
        with patch.object(mock_view.ui.functionBox, 'currentText', lambda: "Gaussian"):
            with patch.object(mock_view, 'is_text_in_function_list', lambda x: True):
                dialog.action_add_function()
                mock_view.function_added.emit.assert_called_once_with("Gaussian")
                self.assertEqual(1, dialog.view.accept.call_count)

    def test_add_function_give_error_if_function_not_valid(self, mock_view):
        dialog = AddFunctionDialog(view=mock_view)
        with patch.object(mock_view, 'is_text_in_function_list', lambda x: False):
            dialog.action_add_function()
            self.assertEqual(1, dialog.view.set_error_message.call_count)
            self.assertEqual(0, mock_view.function_added.emit.call_count)
            self.assertEqual(0, dialog.view.accept.call_count)


if __name__ == '__main__':
    unittest.main()
