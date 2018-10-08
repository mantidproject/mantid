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
# flake8: noqa
"""A selection of utility functions related to testing of Qt-based GUI elements.
"""
from __future__ import absolute_import

from inspect import isfunction, ismethod
import gc

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.plugins import setup_library_paths
from .modal_tester import ModalTester


def requires_qapp(cls):
    """
    Converts a unittest.TestCase class to a GUI test case by wrapping all
    test methods in a decorator that makes sure that a QApplication is created.
    Qt widgets don't work without QApplication.

    Note: It seems possible to segfault the Python process if the QApplication object
    is destroyed too late. This seems nearly guaranteed if the QApplication object is stored
    as a global variable at module scope. For this reason this decorator stores
    its instance as an object attribute.

    Usage:

        @requires_qapp
        class MyWidgetTest(unittest.TestCase):

            def test_something(self):
                ...

            def test_something_else(self):
                ...

    :param cls: Class instance
    """
    def do_nothing(self):
      pass

    orig_setUp = getattr(cls, 'setUp', do_nothing)
    orig_tearDown = getattr(cls, 'tearDown', do_nothing)

    def setUp(self):
        qapp = QApplication.instance()
        if qapp is None:
            setup_library_paths()
            cls._qapp = QApplication([cls.__name__])
        else:
            self._qapp = qapp
        orig_setUp(self)

    def tearDown(self):
      orig_tearDown(self)
      if self._qapp is not None:
          self._qapp.closeAllWindows()
          gc.collect()

    cls.setUp = setUp
    cls.tearDown = tearDown
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
