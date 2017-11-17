from __future__ import absolute_import
from qtpy.QtWidgets import QWidget, QPushButton, QComboBox, QTreeWidget, QVBoxLayout, QHBoxLayout, QCompleter
from qtpy.QtWidgets import QTreeWidgetItem
from .presenter import IAlgorithmSelectorView


class AlgorithmSelectorWidget(QWidget, IAlgorithmSelectorView):
    """
    An algorithm selector view implemented with qtpy.
    """
    def __init__(self, parent=None):
        self.execute_button = None
        self.search_box = None
        self.tree = None
        # self.algorithm_key = ''
        QWidget.__init__(self, parent)
        IAlgorithmSelectorView.__init__(self)

    def _make_search_box(self):
        """
        Make a algorithm search box.
        :return: A QComboBox
        """
        search_box = QComboBox(self)
        search_box.setEditable(True)
        search_box.completer().setCompletionMode(QCompleter.PopupCompletion)
        return search_box

    def _make_tree_widget(self):
        """
        Make a tree widget for displaying algorithms in their categories.
        :return: A QTreeWidget
        """
        tree = QTreeWidget(self)
        tree.setColumnCount(1)
        tree.setHeaderHidden(True)
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
        for key, value in sorted(algorithm_data.items()):
            if key == self.algorithm_key:
                for name in value:
                    item_list.append(QTreeWidgetItem([name]))
            else:
                cat_item = QTreeWidgetItem([key])
                item_list.append(cat_item)
                cat_item_list = []
                self._add_tree_items(cat_item_list, value)
                cat_item.addChildren(cat_item_list)

    def init_ui(self):
        """
        Create and layout the GUI elements.
        """
        self.execute_button = QPushButton('Execute')
        self.search_box = self._make_search_box()
        self.tree = self._make_tree_widget()

        top_layout = QHBoxLayout()
        top_layout.addWidget(self.execute_button)
        top_layout.addWidget(self.search_box)
        top_layout.setStretch(1, 1)

        layout = QVBoxLayout()
        layout.addLayout(top_layout)
        layout.addWidget(self.tree)
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
