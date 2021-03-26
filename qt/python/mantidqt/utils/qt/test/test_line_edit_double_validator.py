# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator
from mantidqt.utils.qt.testing import start_qapplication

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication, QLineEdit
from qtpy.QtTest import QTest


@start_qapplication
class LineEditDoubleValidatorTest(unittest.TestCase):
    def setUp(self):
        self.initial_value = "0.0"

        self.line_edit = QLineEdit(self.initial_value)
        self.line_edit.show()

    def tearDown(self):
        self.assertTrue(self.line_edit.close())

    def test_initialization_of_the_validator_does_not_raise(self):
        try:
            _ = self._create_validator()
        except Exception as ex:
            self.fail(f"An exception was thrown when attempting to create the validator {ex}.")

    def test_that_editing_the_line_edit_with_a_valid_double_will_work(self):
        valid_entry = "1.0"

        self._validator = self._create_validator()
        self._edit_line_edit(valid_entry)

        self.assertEqual(self.line_edit.text(), valid_entry)

    def test_that_editing_the_line_edit_with_a_valid_double_in_e_notation_will_work(self):
        valid_entry = "3e5"

        self._validator = self._create_validator()
        self._edit_line_edit(valid_entry)

        self.assertEqual(self.line_edit.text(), valid_entry)

    def test_that_editing_the_line_edit_with_an_empty_string_will_reset_to_the_last_valid_value(self):
        invalid_entry = ""

        self._validator = self._create_validator()
        self._edit_line_edit(invalid_entry)

        self.assertEqual(self.line_edit.text(), self.initial_value)

    def test_that_editing_the_line_edit_with_broken_e_notation_will_reset_to_the_last_valid_value(self):
        invalid_entry = "e"

        self._validator = self._create_validator()
        self._edit_line_edit(invalid_entry)

        self.assertEqual(self.line_edit.text(), self.initial_value)

    def _create_validator(self):
        validator = LineEditDoubleValidator(self.line_edit, self.initial_value)
        self.line_edit.setValidator(validator)
        return validator

    def _edit_line_edit(self, key_entry):
        QTest.mouseClick(self.line_edit, Qt.LeftButton)
        self.line_edit.selectAll()
        QTest.keyClick(self.line_edit, Qt.Key_Backspace)
        QTest.keyClicks(self.line_edit, key_entry)
        QTest.keyClick(self.line_edit, Qt.Key_Enter)
        QApplication.sendPostedEvents()
