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

import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter

MANTID_API_IMPORT = "from mantid.simpleapi import *\n"
PERMISSION_BOX_FUNC = ('mantidqt.widgets.codeeditor.scriptcompatibility.'
                       'permission_box_to_prepend_import')


@start_qapplication
class MultiPythonFileInterpreterTest(unittest.TestCase, QtWidgetFinder):

    def test_default_contains_single_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)

    def test_add_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assertEqual(2, widget.editor_count)

    def test_open_file_in_new_tab_import_added(self):
        test_string = "Test file\nLoad()"
        widget = MultiPythonFileInterpreter()
        mock_open_func = mock.mock_open(read_data=test_string)
        with mock.patch(widget.__module__ + '.open', mock_open_func, create=True):
            with mock.patch(PERMISSION_BOX_FUNC, lambda: True):
                widget.open_file_in_new_tab(test_string)
        self.assertEqual(widget.current_editor().editor.isModified(), True,
                         msg="Script not marked as modified.")
        self.assertIn(MANTID_API_IMPORT, widget.current_editor().editor.text(),
                      msg="'simpleapi' import not added to script.")

    def test_open_file_in_new_tab_no_import_added(self):
        test_string = "Test file\n"
        widget = MultiPythonFileInterpreter()
        mock_open_func = mock.mock_open(read_data=test_string)
        with mock.patch(widget.__module__ + '.open', mock_open_func, create=True):
            with mock.patch(PERMISSION_BOX_FUNC, lambda: True):
                widget.open_file_in_new_tab(test_string)
        self.assertNotIn(MANTID_API_IMPORT,
                         widget.current_editor().editor.text())


if __name__ == '__main__':
    unittest.main()
