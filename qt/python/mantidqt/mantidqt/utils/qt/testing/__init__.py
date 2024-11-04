# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# flake8: noqa
"""A selection of utility functions related to testing of Qt-based GUI elements."""

from qtpy.QtCore import Qt, QObject

from .application import get_application
from .modal_tester import ModalTester
from .gui_window_test import GuiWindowTest


def start_qapplication(cls):
    """
    Unittest decorator. Adds or augments the setUpClass classmethod
    to the given class. It starts the QApplication object
    if it is not already started
    @param cls: Class being decorated
    """

    def do_nothing(_):
        pass

    def setUpClass(cls):
        get_application()
        setUpClass_orig()

    setUpClass_orig = cls.setUpClass if hasattr(cls, "setUpClass") else do_nothing
    setattr(cls, "setUpClass", classmethod(setUpClass))
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
