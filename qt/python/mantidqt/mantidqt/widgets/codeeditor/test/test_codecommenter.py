# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
            "for ii in range(2):",
            "   do_something()",
            "do_something_else()"]

        self.editor = CodeEditor("AlternateCSPython", QFont())
        self.editor.setText('\n'.join(self.lines))
        self.commenter = CodeCommenter(self.editor)

    def test_comment_single_line_no_selection(self):
        self.editor.setCursorPosition(8, 1)
        self.commenter.toggle_comment()
        expected_lines = copy(self.lines)
        expected_lines[8] = '# ' + expected_lines[8]
        self.assertEqual(self.editor.text(), '\n'.join(expected_lines))

    def test_uncomment_with_inline_comment(self):
        """
        Check that uncommenting works correctly when there is an inline comment on the line of code that is to be
        uncommented.
        """
        commented_lines = [
            '#do_something() # inline comment',
            '#do_something() #inline comment',
            '# do_something() # inline comment',
            '# do_something() #inline comment',
            '    #do_something() # inline comment',
            '    #do_something() #inline comment',
            '    # do_something() # inline comment',
            '    # do_something() #inline comment']
        expected_uncommented_lines = [
            'do_something() # inline comment',
            'do_something() #inline comment',
            'do_something() # inline comment',
            'do_something() #inline comment',
            '    do_something() # inline comment',
            '    do_something() #inline comment',
            '    do_something() # inline comment',
            '    do_something() #inline comment']
        uncommented_lines = self.commenter._uncomment_lines(commented_lines)
        self.assertEqual(expected_uncommented_lines, uncommented_lines)

    def test_multiline_uncomment(self):
        start_line, end_line = 0, 3
        self.editor.setSelection(0, 2, 3, 2)
        self.commenter.toggle_comment()
        expected_lines = copy(self.lines)
        expected_lines[start_line:end_line + 1] = [
            line.replace('# ', '') for line in expected_lines[0:end_line + 1]]
        self.assertEqual(self.editor.text(), '\n'.join(expected_lines))

    def test_multiline_comment_for_mix_of_commented_and_uncommented_lines(self):
        start_line, end_line = 5, 8
        self.editor.setSelection(start_line, 5, end_line, 5)
        self.commenter.toggle_comment()
        expected_lines = copy(self.lines)
        expected_lines[start_line:end_line + 1] = [
            '# ' + line for line in expected_lines[start_line:end_line + 1]]
        self.assertEqual(self.editor.text(), '\n'.join(expected_lines))

    def test_multiline_comment_uses_top_level_indentation(self):
        start_line, end_line = 10, 13
        self.editor.setSelection(start_line, 5, end_line, 5)
        self.commenter.toggle_comment()
        expected_lines = ["# for ii in range(2):",
                          "#    do_something()",
                          "# do_something_else()"]
        self.assertEqual(self.editor.text().split('\n')[start_line:end_line], expected_lines)

    def test_comment_preserves_indenting_on_single_line(self):
        iline = 11
        self.editor.setSelection(iline, 5, iline, 6)
        self.commenter.toggle_comment()
        # check commented at indented position
        self.assertEqual(self.editor.text().split('\n')[iline], "   # do_something()")


if __name__ == '__main__':
    unittest.main()
