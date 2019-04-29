# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# flake8: noqa
"""A selection of utility functions related to testing of Qt-based GUI elements.
"""
from __future__ import absolute_import

from unittest import TestCase

from qtpy.QtCore import Qt

from .application import get_application
from .modal_tester import ModalTester
from .gui_window_test import GuiWindowTest


class GuiTest(TestCase):
    """Base class for tests that require a QApplication instance
    GuiTest ensures that a QApplication exists before tests are run
    """
    @classmethod
    def setUpClass(cls):
        """Prepare for test execution.
        Ensure that a (single copy of) QApplication has been created
        """
        get_application(cls.__name__)


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
