#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest

from qtpy.QtCore import QObject, Qt
from qtpy.QtWidgets import QAction, QApplication

from mantidqt.utils.qt import create_action

QAPP = None


class CreateActionTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        global QAPP
        if QApplication.instance() is None:
            QAPP = QApplication([''])

    def test_parent_and_name_only_required(self):
        class Parent(QObject):
            pass
        parent = Parent()
        action = create_action(parent, "Test Action")
        self.assertTrue(isinstance(action, QAction))
        self.assertEquals(parent, action.parent())

    def test_parent_can_be_none(self):
        action = create_action(None, "Test Action")
        self.assertTrue(isinstance(action, QAction))
        self.assertTrue(action.parent() is None)

    def test_supplied_triggered_callback_attaches_to_triggered_signal(self):
        class Receiver(QObject):
            def test_slot(self):
                pass
        recv = Receiver()
        action = create_action(None, "Test Action", on_triggered=recv.test_slot)
        self.assertEqual(1, action.receivers(action.triggered))

    def test_shortcut_is_set_if_given(self):
        action = create_action(None, "Test Action", shortcut="Ctrl+S")
        self.assertEqual("Ctrl+S", action.shortcut())

    def test_shortcut_context_used_if_shortcut_given(self):
        action = create_action(None, "Test Action", shortcut="Ctrl+S",
                               shortcut_context=Qt.ApplicationShortcut)
        self.assertEqual(Qt.ApplicationShortcut, action.shortcutContext())

    def test_shortcut_context_not_used_if_shortcut_given(self):
        action = create_action(None, "Test Action", shortcut_context=Qt.ApplicationShortcut)
        self.assertEqual(Qt.WindowShortcut, action.shortcutContext())


if __name__ == "__main__":
    unittest.main()
