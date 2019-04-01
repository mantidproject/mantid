# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package


class CodeCommenter:

    def __init__(self, editor):
        self.editor = editor

    def toggle_comment(self):
        selection_idxs = self._get_selection_idxs()
        self._expand_selection(selection_idxs)
        # Note selection indices to restore highlighting later
        selected_lines = self.editor.selectedText().split('\n')

        if self._are_comments(selected_lines) is True:
            toggled_lines = self._uncomment_lines(selected_lines)
            # Track deleted characters to keep highlighting consistent
            selection_idxs[1] -= 2
            selection_idxs[-1] -= 2
        else:
            toggled_lines = self._comment_lines(selected_lines)
            selection_idxs[1] += 2
            selection_idxs[-1] += 2

        # Replace lines with commented/uncommented lines
        self.editor.replaceSelectedText('\n'.join(toggled_lines))

        # Restore highlighting
        self.editor.setSelection(*selection_idxs)

    def _get_selection_idxs(self):
        if self.editor.selectedText() == '':
            cursor_pos = list(self.editor.getCursorPosition())
            return cursor_pos + cursor_pos
        else:
            return list(self.editor.getSelection())

    def _expand_selection(self, selection_idxs):
        """
        Expands selection from first character of first selected line to
        last character of the last selected line.
        :param selection_idxs: Length 4 list, e.g. [row0, col0, row1, col1]
        """
        line_end_pos = len(self.editor.text().split('\n')[selection_idxs[2]].rstrip())
        line_selection_idxs = [selection_idxs[0], 0,
                               selection_idxs[2], line_end_pos]
        self.editor.setSelection(*line_selection_idxs)

    def _are_comments(self, code_lines):
        for line in code_lines:
            if line.strip():
                if not line.strip().startswith('#'):
                    return False
        return True

    def _comment_lines(self, lines):
        for i in range(len(lines)):
            lines[i] = '# ' + lines[i]
        return lines

    def _uncomment_lines(self, lines):
        for i in range(len(lines)):
            uncommented_line = lines[i].replace('# ', '', 1)
            if uncommented_line == lines[i]:
                uncommented_line = lines[i].replace('#', '', 1)
            lines[i] = uncommented_line
        return lines
