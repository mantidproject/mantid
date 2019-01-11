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

# std imports
import unittest

# 3rd party imports
import six

# local imports
from mantidqt.widgets.codeeditor.interpreter import PythonFileInterpreter
from mantidqt.utils.qt.test import GuiTest

if six.PY2:
    import mock
else:
    from unittest import mock


class PythonFileInterpreterTest(GuiTest):

    def test_construction(self):
        w = PythonFileInterpreter()
        self.assertTrue("Status: Idle", w.status.currentMessage())

    def test_empty_code_does_nothing_on_exec(self):
        w = PythonFileInterpreter()
        w._presenter.model.execute_async = mock.MagicMock()
        w.execute_async()
        w._presenter.model.execute_async.assert_not_called()
        self.assertTrue("Status: Idle", w.status.currentMessage())

    def test_constructor_populates_editor_with_content(self):
        w = PythonFileInterpreter(content='# my funky code')
        self.assertEqual('# my funky code', w.editor.text())

    def test_constructor_respects_filename(self):
        w = PythonFileInterpreter(filename='test.py')
        self.assertEqual('test.py', w.filename)

    def test_successful_execution(self):
        w = PythonFileInterpreter()
        w.editor.setText("x = 1 + 2")
        w.execute_async()
        self.assertTrue("Status: Idle", w.status.currentMessage())


if __name__ == '__main__':
    unittest.main()
