# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from qtpy.QtCore import Qt
from qtpy.QtTest import QTest

from mantidqt.widgets.fitpropertybrowser.addfunctiondialog.view import AddFunctionDialogView
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class AddFunctionDialogPresenterTest(unittest.TestCase):
    TEST_FUNCTION_NAMES = ['func1', 'secondfunc']

    def test_construction_with_no_functions_gives_empty_list(self):
        view = AddFunctionDialogView()

        self.assertTrue(view is not None)
        self.assertEqual(0, view.ui.functionBox.count())

    def test_construction_with_functions_gives_list_of_same_length(self):
        view = AddFunctionDialogView(function_names=self.TEST_FUNCTION_NAMES)

        self.assertTrue(view is not None)
        self.assertEqual(len(self.TEST_FUNCTION_NAMES), view.ui.functionBox.count())

    def test_construction_with_functions_and_default_sets_placeholder_to_default(self):
        view = AddFunctionDialogView(function_names=self.TEST_FUNCTION_NAMES,
                                     default_function_name=self.TEST_FUNCTION_NAMES[1])

        self.assertTrue(view is not None)
        self.assertEqual(len(self.TEST_FUNCTION_NAMES), view.ui.functionBox.count())
        self.assertEqual(self.TEST_FUNCTION_NAMES[1], view.ui.functionBox.lineEdit().placeholderText())

    def test_pressing_return_in_box_activates_completer(self):
        view = AddFunctionDialogView(function_names=self.TEST_FUNCTION_NAMES)

        QTest.keyPress(view.ui.functionBox, Qt.Key_F)
        QTest.keyPress(view.ui.functionBox, Qt.Key_U)
        QTest.keyPress(view.ui.functionBox, Qt.Key_N)
        QTest.keyPress(view.ui.functionBox, Qt.Key_C)
        QTest.keyPress(view.ui.functionBox, Qt.Key_1)
        QTest.keyPress(view.ui.functionBox, Qt.Key_Return)

        self.assertEqual(self.TEST_FUNCTION_NAMES[0], view.ui.functionBox.currentText())


if __name__ == '__main__':
    unittest.main()
