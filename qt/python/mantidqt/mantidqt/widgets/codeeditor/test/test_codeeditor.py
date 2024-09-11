# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
# system imports
import unittest

# local imports
from mantidqt.widgets.codeeditor.editor import CodeEditor
from mantidqt.utils.qt.testing import start_qapplication

TEST_LANG = "Python"


@start_qapplication
class CodeEditorTest(unittest.TestCase):
    # ---------------------------------------------------------------
    # Success tests
    # ---------------------------------------------------------------
    def test_construction_accepts_Python_as_language(self):
        CodeEditor(TEST_LANG)

    def test_default_construction_yields_empty_filename(self):
        widget = CodeEditor(TEST_LANG)
        self.assertEqual("", widget.fileName())

    def test_set_filename_returns_expected_string(self):
        widget = CodeEditor(TEST_LANG)
        test_filename = "myscript.py"
        widget.setFileName(test_filename)
        self.assertEqual(test_filename, widget.fileName())

    def test_set_text_can_be_read_again(self):
        widget = CodeEditor(TEST_LANG)
        code_str = 'print "Hello World!"'
        widget.setText(code_str)
        self.assertEqual(code_str, widget.text())

    def test_default_construction_yields_editable_widget(self):
        widget = CodeEditor(TEST_LANG)
        self.assertFalse(widget.isReadOnly())

    def test_setReadOnly_to_true_sets_readonly_status(self):
        widget = CodeEditor(TEST_LANG)
        widget.setReadOnly(True)
        self.assertTrue(widget.isReadOnly())

    def test_get_selection_for_empty_selection(self):
        widget = CodeEditor(TEST_LANG)
        res = widget.getSelection()
        self.assertEqual((-1, -1, -1, -1), res)

    def test_get_selection_for_non_empty_selection(self):
        widget = CodeEditor(TEST_LANG)
        widget.setText(
            """first line
        second line
        third line
        fourth line
        """
        )
        selected = (0, 2, 3, 4)
        widget.setSelection(*selected)
        res = widget.getSelection()
        self.assertEqual(selected, res)

    # ---------------------------------------------------------------
    # Failure tests
    # ---------------------------------------------------------------
    def test_construction_raises_error_for_unknown_language(self):
        # self.assertRaises causes a segfault here for some reason...
        try:
            CodeEditor("MyCoolLanguage")
        except ValueError:
            pass
        except Exception as exc:
            self.fail("Expected a Value error to be raised but found a " + exc.__name__)


if __name__ == "__main__":
    unittest.main()
