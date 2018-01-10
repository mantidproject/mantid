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

# std imports
import unittest

# 3rd party imports
from qtpy.QtWidgets import QStatusBar
import six

# local imports
from mantidqt.widgets.codeeditor.editor import ExecutableCodeEditor, ExecutableCodeEditorPresenter
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution

if six.PY2:
    import mock
else:
    from unittest import mock


class ExecutableCodeEditorPresenterTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.create_autospec(ExecutableCodeEditor)
        self.view.set_status_message = mock.MagicMock()
        self.view.set_editor_readonly = mock.MagicMock()

        self.presenter = ExecutableCodeEditorPresenter(self.view)
        self.view.set_status_message.reset_mock()

    def test_construction_sets_idle_status(self):
        ExecutableCodeEditorPresenter(self.view)
        self.view.set_status_message.assert_called_once_with("Status: Idle")

    def test_execute_all_async_with_empty_code_does_nothing(self):
        self.view.text = mock.MagicMock(return_value="")
        self.presenter.model.execute_async = mock.MagicMock()

        self.presenter.req_execute_all_async()
        self.view.text.assert_called_once_with()
        self.view.set_editor_readonly.assert_not_called()
        self.view.set_status_message.assert_not_called()
        self.presenter.model.execute_async.assert_not_called()

    def test_execute_all_async_with_successful_code(self):
        code_str = "x = 1 + 2"
        self.view.text = mock.MagicMock(return_value=code_str)
        self.model.execute_async = mock.MagicMock()

        self.presenter.req_execute_all_async()
        self.view.text.assert_called_once_with()
        self.view.set_editor_readonly.assert_called_once_with(True)
        self.view.set_status_message.assert_called_once_with("Status: Running")
        self.model.execute_async.assert_called_once_with(code_str)


if __name__ == '__main__':
    unittest.main()
