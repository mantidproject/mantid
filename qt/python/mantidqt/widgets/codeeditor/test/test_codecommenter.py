# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package

import unittest
from copy import copy

from qtpy.QtGui import QFont

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.codeeditor.codecommenter import CodeCommenter
from mantidqt.widgets.codeeditor.editor import CodeEditor


@start_qapplication
class CodeCommenterTest(unittest.TestCase):

    def setUp(self):
        self.lines = [
            "# Mantid Repository : https://github.com/mantidproject/mantid",
            "# ",
            "# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,",
            "#     NScD Oak Ridge National Laboratory, European Spallation Source",
            "#     & Institut Laue - Langevin",
            "# SPDX - License - Identifier: GPL - 3.0 +",
            "#  This file is part of the mantidqt package",
            "",
            "import numpy",
            "# import mantid",
            "do_something()"]

        self.editor = CodeEditor("AlternateCSPython", QFont())
        self.editor.setText('\n'.join(self.lines))
        self.commenter = CodeCommenter(self.editor)

    def test_comment_single_line_no_selection(self):
        self.editor.setCursorPosition(8, 1)
        self.commenter.toggle_comment()
        expected_lines = copy(self.lines)
        expected_lines[8] = '# ' + expected_lines[8]
        self.assertEqual(self.editor.text(), '\n'.join(expected_lines))

    def test_multiline_uncomment(self):
        start_line, end_line = 0, 3
        self.editor.setSelection(0, 2, 3, 2)
        self.commenter.toggle_comment()
        expected_lines = copy(self.lines)
        expected_lines[start_line:end_line+1] = [
            line.replace('# ', '') for line in expected_lines[0:end_line+1]]
        self.assertEqual(self.editor.text(), '\n'.join(expected_lines))

    def test_multiline_comment(self):
        start_line, end_line = 5, 8
        self.editor.setSelection(start_line, 5, end_line, 5)
        self.commenter.toggle_comment()
        expected_lines = copy(self.lines)
        expected_lines[start_line:end_line+1] = [
            '# ' + line for line in expected_lines[start_line:end_line+1]]
        self.assertEqual(self.editor.text(), '\n'.join(expected_lines))


if __name__ == '__main__':
    unittest.main()
