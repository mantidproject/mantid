# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package
from unittest import TestCase
from unittest.mock import patch

from qtpy.QtWidgets import QInputDialog
from workbench.utils.io import input_qinputdialog


class IOTest(TestCase):

    @patch('workbench.utils.io.QInputDialog')
    def test_input_qinputdialog_setup_is_correct(self, mock_QInputDialogClass):
        mock_QInputDialogClass.TextInput = QInputDialog.TextInput
        mock_QInputDialog = mock_QInputDialogClass()

        _ = input_qinputdialog("prompt")

        mock_QInputDialog.setInputMode.assert_called_with(QInputDialog.TextInput)
        mock_QInputDialog.setLabelText.assert_called_with("prompt")

    @patch('workbench.utils.io.QInputDialog')
    def test_input_qinputdialog_return_value_is_correct_when_dialog_accepted(self, mock_QInputDialogClass):
        mock_QInputDialog = mock_QInputDialogClass()
        mock_QInputDialog.exec_.return_value = True
        mock_QInputDialog.textValue.return_value = "their input"

        user_input = input_qinputdialog()

        self.assertEqual(user_input, "their input")

    @patch('workbench.utils.io.QInputDialog')
    def test_input_qinputdialog_raises_RuntimeError_when_input_cancelled(self, mock_QInputDialogClass):
        mock_QInputDialog = mock_QInputDialogClass()
        mock_QInputDialog.exec_.return_value = False

        self.assertRaises(EOFError, input_qinputdialog)
