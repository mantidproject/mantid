# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

# system imports
import os
import unittest

# third-party library imports

# local imports
from mantidqt.utils.qt.test import GuiTest
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter


class MultiPythonFileInterpreterTest(GuiTest):

    def test_default_contains_single_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)

    def test_add_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assertEqual(2, widget.editor_count)

    def test_tab_session_restore(self):
        widget = MultiPythonFileInterpreter()
        widget.prev_session_tabs = [
            os.path.join(os.path.dirname(__file__), u'__init__.py'),
            __file__]
        widget.restore_session_tabs()
        self.assertEqual(2, widget.editor_count)

    def test_tab_session_restore_nothing_to_restore(self):
        widget = MultiPythonFileInterpreter()
        widget.prev_session_tabs = None
        widget.restore_session_tabs()
        self.assertEqual(1, widget.editor_count)  # default empty tab should be open

    def test_tab_session_restore_path_doesnt_exist(self):
        widget = MultiPythonFileInterpreter()
        widget.prev_session_tabs = ['FileDoesntExist']
        widget.restore_session_tabs()
        self.assertEqual(1, widget.editor_count)  # default empty tab should be open


if __name__ == '__main__':
    unittest.main()
