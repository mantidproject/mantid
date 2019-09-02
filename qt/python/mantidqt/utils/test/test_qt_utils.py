# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest

from qtpy.QtCore import QObject, Qt, Slot
from qtpy.QtWidgets import QAction, QMenu, QToolBar

try:
    from qtpy.QtCore import SIGNAL

    NEW_STYLE_SIGNAL = False
except ImportError:
    NEW_STYLE_SIGNAL = True

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt import add_actions, create_action


@start_qapplication
class CreateActionTest(unittest.TestCase):

    def test_parent_and_name_only_required(self):
        class Parent(QObject):
            pass

        parent = Parent()
        action = create_action(parent, "Test Action")
        self.assertTrue(isinstance(action, QAction))
        self.assertEqual(parent, action.parent())

    def test_parent_can_be_none(self):
        action = create_action(None, "Test Action")
        self.assertTrue(isinstance(action, QAction))
        self.assertEqual(action.parent(), None)

    def test_supplied_triggered_callback_attaches_to_triggered_signal(self):
        class Receiver(QObject):
            @Slot()
            def test_slot(self):
                pass

        recv = Receiver()
        action = create_action(None, "Test Action", on_triggered=recv.test_slot)
        if NEW_STYLE_SIGNAL:
            self.assertEqual(1, action.receivers(action.triggered))
        else:
            self.assertEqual(1, action.receivers(SIGNAL("triggered()")))

    def test_shortcut_is_set_if_given(self):
        action = create_action(None, "Test Action", shortcut="Ctrl+S")
        self.assertEqual("Ctrl+S", action.shortcut())

    def test_multiple_shortcuts_are_set_if_given(self):
        expected_shortcuts = ("Ctrl+S", "Ctrl+W")
        action = create_action(None, "Test Action", shortcut=expected_shortcuts)
        for expected, actual in zip(expected_shortcuts, action.shortcuts()):
            self.assertEqual(expected, actual.toString())

    def test_shortcut_context_used_if_shortcut_given(self):
        action = create_action(None, "Test Action", shortcut="Ctrl+S",
                               shortcut_context=Qt.ApplicationShortcut)
        self.assertEqual(Qt.ApplicationShortcut, action.shortcutContext())

    def test_shortcut_context_not_used_if_shortcut_given(self):
        action = create_action(None, "Test Action", shortcut_context=Qt.ApplicationShortcut)
        self.assertEqual(Qt.WindowShortcut, action.shortcutContext())


@start_qapplication
class AddActionsTest(unittest.TestCase):

    def test_add_actions_with_qmenu_target(self):
        test_act_1 = create_action(None, "Test Action 1")
        test_act_2 = create_action(None, "Test Action 2")
        test_menu = QMenu()
        add_actions(test_menu, [test_act_1, test_act_2])
        self.assertFalse(test_menu.isEmpty())

    def test_add_actions_with_qtoolbar_target(self):
        test_act_1 = create_action(None, "Test Action 1")
        test_act_2 = create_action(None, "Test Action 2")
        test_toolbar = QToolBar()
        # There seems to be no easy access to check the size of the list
        # so check the number of children increases by 2
        nchildren_before = len(test_toolbar.children())
        add_actions(test_toolbar, [test_act_1, test_act_2])
        self.assertEqual(nchildren_before + 2, len(test_toolbar.children()))

    def test_add_actions_with_bad_target_raises_attribute_error(self):
        test_act_1 = create_action(None, "Test Action 1")
        test_act_2 = create_action(None, "Test Action 2")
        test_toolbar = QObject()
        self.assertRaises(AttributeError, add_actions, test_toolbar, [test_act_1, test_act_2])

    def test_add_actions_with_invalid_parameter_raises_value_error(self):
        test_act_1 = create_action(None, "Test Action 1")
        test_act_2 = QObject()
        test_toolbar = QToolBar()
        self.assertRaises(ValueError, add_actions, test_toolbar, [test_act_1, test_act_2])


if __name__ == "__main__":
    unittest.main()
