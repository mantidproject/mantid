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
"""A selection of utility functions related to testing of Qt-based GUI elements.
"""
from __future__ import absolute_import

from inspect import getmembers, isfunction, ismethod

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.plugins import setup_library_paths


# Reference to created QApplication instance so that item is kept alive
QAPP = None


def gui_test(test_method):
    """
    Decorator for GUI test methods. Creates a QApplication before
    executing the test.
    :param test_method: A test method.
    """
    def _wrapper(self):
        global QAPP
        if not QAPP:
            setup_library_paths()
            QAPP = QApplication([''])
        test_method(self)
        QAPP.closeAllWindows()

    return _wrapper


def gui_test_case(cls):
    """
    Converts a unittest.TestCase class to a GUI test case by wrapping all
    test methods in gui_test decorator. Usage:

        @gui_test_case
        class MyWidgetTest(unittest.TestCase):

            def test_something(self):
                ...

            def test_something_else(self):
                ...

    Which is equivalent to the definition:

        class MyWidgetTest(unittest.TestCase):

            @gui_test
            def test_something(self):
                ...

            @gui_test
            def test_something_else(self):
                ...

    :param cls: Class instance
    """
    def is_test_method(name, x):
        # Python 3 returns a functions type for methods not bound to an instance
        return name.startswith('test') and (isfunction(x) or ismethod(x))

    for name in dir(cls):
        attr = getattr(cls, name)
        if is_test_method(name, attr):
            setattr(cls, name, gui_test(attr))
    return cls


def select_item_in_tree(tree, item_label):
    """
    Select an item in a QTreeWidget with a given label.
    :param tree: A QTreeWidget
    :param item_label: A label text on the item to select.
    """
    items = tree.findItems(item_label, Qt.MatchExactly | Qt.MatchRecursive)
    tree.setCurrentItem(items[0])


def select_item_in_combo_box(combo_box, item_text):
    """
    Select an item in a QComboBox with a given text.
    :param combo_box: A QComboBox.
    :param item_text: A text of the item to select.
    """
    i = combo_box.findText(item_text)
    combo_box.setCurrentIndex(i)
