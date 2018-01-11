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
import six

# local imports
from mantidqt.widgets.codeeditor.editor import ExecutableCodeEditor
from mantidqt.utils.qt.testing import requires_qapp

if six.PY2:
    import mock
else:
    from unittest import mock


@requires_qapp
class ExecutableCodeEditorTest(unittest.TestCase):

    def test_construction(self):
        w = ExecutableCodeEditor()
        self.assertTrue("Status: Idle", w.status.currentMessage())

    def test_empty_code_does_nothing_on_exec(self):
        w = ExecutableCodeEditor()
        w._presenter.model.execute_async = mock.MagicMock()

        w.execute_all_async()

        w._presenter.model.execute_async.assert_not_called()
        self.assertTrue("Status: Idle", w.status.currentMessage())

    def test_successful_execution(self):
        w = ExecutableCodeEditor()
        w.editor.setText("x = 1 + 2")
        w.execute_all_async()
        self.assertTrue("Status: Idle", w.status.currentMessage())


if __name__ == '__main__':
    unittest.main()
