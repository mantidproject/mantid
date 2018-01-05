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

# third-party library imports

# local imports
from mantidqt.widgets.codeeditor import MultiFileCodeEditor
from mantidqt.utils.qt.testing import requires_qapp


@requires_qapp
class MultiFileCodeEditorTest(unittest.TestCase):

    # Success tests

    def test_default_contains_single_tab(self):
        widget = MultiFileCodeEditor()
        self.assertEqual(1, widget.editor_count)


if __name__ == '__main__':
    unittest.main()
