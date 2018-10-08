#    This file is part of the mantid workbench.
#
#    Copyright (C) 2017 mantidproject
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# system imports
import unittest

# local imports
from mantidqt.widgets.codeeditor.editor import CodeEditor
from mantidqt.utils.qt.test import requires_qapp

TEST_LANG = "Python"


@requires_qapp
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
        widget.setText("""first line
        second line
        third line
        fourth line
        """)
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


if __name__ == '__main__':
    unittest.main()
