# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

import re

import qtpy
from qtpy.QtCore import QModelIndex, Qt
from qtpy.QtWidgets import (QWidget, QPushButton, QComboBox, QTreeWidget, QVBoxLayout,
                            QHBoxLayout, QCompleter, QTreeWidgetItem)

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt import block_signals
from mantidqt.widgets.algorithmprogress import AlgorithmProgressWidget

from .algorithm_factory_observer import AlgorithmSelectorFactoryObserver
from .presenter import IAlgorithmSelectorView, SelectedAlgorithm


def get_name_and_version_from_item_label(item_label):
    """
    Extract algorithm name and version from a tree widget item label.
    It must have the format:

        AlgorithmName v.n

    where n in v.n is a number
    :param item_label: A label of a TreeWidgetItem
    :return: SelectedAlgorithm tuple or None if the format is wrong.
    """
    match = re.match(r'(\w+) v\.(\d+)', item_label)
    if match:
        return SelectedAlgorithm(name=match.group(1), version=int(match.group(2)))
    return None


class AlgorithmTreeWidget(QTreeWidget):

    def __init__(self, parent=None):
        super(AlgorithmTreeWidget, self).__init__(parent)
        self.parent = parent

    def mouseDoubleClickEvent(self, mouse_event):
        if mouse_event.button() == Qt.LeftButton:
            if not self.selectedItems()[0].child(0):
                self.parent.execute_algorithm()
            super(AlgorithmTreeWidget, self).mouseDoubleClickEvent(mouse_event)


class AlgorithmSelectorWidget(IAlgorithmSelectorView, QWidget):
    """
    An algorithm selector view implemented with qtpy.
    """

    def __init__(self, parent=None, include_hidden=False):
        """
        Initialise a new instance of AlgorithmSelectorWidget
        :param parent: A parent QWidget
        :param include_hidden: If True the widget must include all hidden algorithms
        """
        self.execute_button = None
        self.search_box = None
        self.tree = None
        QWidget.__init__(self, parent)
        IAlgorithmSelectorView.__init__(self, include_hidden)

        self.afo = AlgorithmSelectorFactoryObserver(self)

    def observeUpdate(self, toggle):
        """
        Set whether or not to update AlgorithmSelector if AlgorithmFactory changes
        :param toggle: A bool. If true, we update AlgorithmSelector
        """
        self.afo.observeUpdate(toggle)

    def refresh(self):
        """Update the algorithms list"""
        self.presenter.refresh()

    def _make_execute_button(self):
        """
        Make the button that starts the algorithm.
        :return: A QPushButton
        """
        button = QPushButton('Execute')
        button.clicked.connect(self._on_execute_button_click)
        return button

    def _make_search_box(self):
        """
        Make an algorithm search box.
        :return: A QComboBox
        """
        search_box = QComboBox(self)
        search_box.setEditable(True)
        search_box.completer().setCompletionMode(QCompleter.PopupCompletion)
        # setFilterMode behaviour changing is only available on Qt5, if this check is not present the Qt4 tests fail
        if qtpy.PYQT5:
            search_box.completer().setFilterMode(Qt.MatchContains)
        search_box.setInsertPolicy(QComboBox.NoInsert)
        search_box.editTextChanged.connect(self._on_search_box_selection_changed)
        search_box.lineEdit().returnPressed.connect(self.execute_algorithm)
        return search_box

    def _make_tree_widget(self):
        """
        Make a tree widget for displaying algorithms in their categories.
        :return: A QTreeWidget
        """
        tree = AlgorithmTreeWidget(self)
        tree.setColumnCount(1)
        tree.setHeaderHidden(True)
        tree.itemSelectionChanged.connect(self._on_tree_selection_changed)
        return tree

    def _add_tree_items(self, item_list, algorithm_data):
        """
        Recursively adds QTreeWidgetItems to item_list. Data for the items
        are taken from algorithm_data
        :param item_list: A list of QTreeWidgetItem
        :param algorithm_data: A dict of algorithm descriptors as returned from
            Model.get_algorithm_data()
        :return: None
        """
        for key, sub_tree in sorted(algorithm_data.items()):
            if key == self.algorithm_key:
                for name, versions in sorted(sub_tree.items()):
                    versions = sorted(versions)
                    default_version_item = QTreeWidgetItem(['{0} v.{1}'.format(name, versions[-1])])
                    item_list.append(default_version_item)
                    if len(versions) > 1:
                        for v in versions[:-1]:
                            default_version_item.addChild(QTreeWidgetItem(['{0} v.{1}'.format(name, v)]))
            else:
                cat_item = QTreeWidgetItem([key])
                item_list.append(cat_item)
                cat_item_list = []
                self._add_tree_items(cat_item_list, sub_tree)
                cat_item.addChildren(cat_item_list)

    def _get_selected_tree_item(self):
        """
        Get algorithm selected in the tree widget.
        :return: A SelectedAlgorithm tuple or None if nothing is selected.
        """
        items = self.tree.selectedItems()
        if len(items) == 0:
            return None
        selected_algorithm = get_name_and_version_from_item_label(items[0].text(0))
        if selected_algorithm is not None:
            return selected_algorithm
        return None

    def _get_search_box_selection(self):
        """
        Get algorithm selected in the search box.
        :return: A SelectedAlgorithm tuple or None if nothing is selected.
        """
        text = self.search_box.lineEdit().text()
        i = self.search_box.findText(text)
        if i < 0:
            return None
        return SelectedAlgorithm(name=self.search_box.currentText(), version=-1)

    def _on_search_box_selection_changed(self, text):
        """
        Called when text in the search box is changed by the user or script.
        :param text: New text in the search box.
        """
        # if the function is called without text, avoid doing anything
        if text == '':
            return

        with block_signals(self.tree):
            self.tree.setCurrentIndex(QModelIndex())
        with block_signals(self.search_box):
            i = self.search_box.findText(text)
            if i >= 0:
                self.search_box.setCurrentIndex(i)

    def _on_tree_selection_changed(self):
        """
        Called when selection in the tree widget changes by user clicking
        or a script.
        """
        selected_algorithm = self._get_selected_tree_item()
        if selected_algorithm is not None:
            with block_signals(self.search_box):
                i = self.search_box.findText(selected_algorithm.name)
                self.search_box.setCurrentIndex(i)

    def _on_execute_button_click(self):
        self.execute_algorithm()

    def init_ui(self):
        """
        Create and layout the GUI elements.
        """
        self.execute_button = self._make_execute_button()
        self.search_box = self._make_search_box()
        self.tree = self._make_tree_widget()

        top_layout = QHBoxLayout()
        top_layout.addWidget(self.execute_button)
        top_layout.addWidget(self.search_box)
        top_layout.setStretch(1, 1)

        layout = QVBoxLayout()
        layout.addLayout(top_layout)
        layout.addWidget(self.tree)

        algProgWidget = AlgorithmProgressWidget(self)
        algProgWidget.setAttribute(Qt.WA_DeleteOnClose)

        layout.addWidget(algProgWidget)
        # todo: Without the sizeHint() call the minimum size is not set correctly
        #       This needs some investigation as to why this is.
        layout.sizeHint()
        self.setLayout(layout)

    def populate_ui(self, data):
        """
        Populate the GUI elements with the data that comes from Presenter.
        :param data: a list of algorithm descriptors.
        """
        self.search_box.addItems(data[0])
        self.search_box.setCurrentIndex(-1)
        item_list = []
        self._add_tree_items(item_list, data[1])
        self.tree.insertTopLevelItems(0, item_list)

    def get_selected_algorithm(self):
        """
        Get algorithm selected by the user.
        :return: A SelectedAlgorithm namedtuple.
        """
        selected_algorithm = self._get_selected_tree_item()
        if selected_algorithm is not None:
            return selected_algorithm
        return self._get_search_box_selection()

    def execute_algorithm(self):
        """
        Send a signal to a subscriber to execute the selected algorithm
        """
        algorithm = self.get_selected_algorithm()
        if algorithm is not None:
            manager = InterfaceManager()
            dialog = manager.createDialogFromName(algorithm.name, algorithm.version)
            dialog.show()
