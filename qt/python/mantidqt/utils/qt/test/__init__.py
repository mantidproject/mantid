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

from unittest import TestCase

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication

from mantidqt.utils.qt.plugins import setup_library_paths
from .modal_tester import ModalTester

# Hold on to QAPP reference to avoid garbage collection
_QAPP = None


class GuiTest(TestCase):
    """Base class for tests that require a QApplication instance
    GuiTest ensures that a QApplication exists before tests are run
    """
    @classmethod
    def setUpClass(cls):
        """Prepare for test execution.
        Ensure that a (single copy of) QApplication has been created
        """
        global _QAPP
        if _QAPP is None:
            setup_library_paths()
            _QAPP = QApplication([cls.__name__])


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
