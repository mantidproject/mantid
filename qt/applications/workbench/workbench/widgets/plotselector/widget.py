from __future__ import absolute_import, print_function

from qtpy.QtCore import Qt
from qtpy.QtGui import (QStandardItem, QStandardItemModel)
from qtpy.QtWidgets import (QAbstractItemView, QListView, QWidget, QPushButton, QVBoxLayout, QHBoxLayout)

from .presenter import PlotSelectorPresenter


class PlotSelectorWidget(QWidget):
    """
    An algorithm selector view implemented with qtpy.
    """
    def __init__(self, parent=None, include_hidden=False):
        """
        Initialise a new instance of AlgorithmSelectorWidget
        :param parent: A parent QWidget
        :param include_hidden: If True the widget must include all hidden algorithms
        """
        self.new_button = None
        self.list_view = None
        self.plot_list = None
        QWidget.__init__(self, parent)
        self.presenter = PlotSelectorPresenter(self)

    def _make_refresh_button(self):
        """
        Make the button that starts the algorithm.
        :return: A QPushButton
        """
        button = QPushButton('Refresh')
        button.clicked.connect(self._on_refresh_button_click)
        return button

    def _make_plot_list(self):
        """
        TODO:
        :return:
        """
        list_view = QListView(self)
        list_view.setSelectionMode(QAbstractItemView.ExtendedSelection)
        plot_list = QStandardItemModel(self.list_view)
        list_view.setModel(plot_list)
        list_view.doubleClicked.connect(self.double_clicked_item)
        return list_view, plot_list

    def _on_refresh_button_click(self):
        self.refresh_plot_list()

    def init_ui(self):
        """
        Create and layout the GUI elements.
        """
        self.new_button = self._make_refresh_button()
        self.list_view, self.plot_list = self._make_plot_list()

        top_layout = QHBoxLayout()
        top_layout.addWidget(self.new_button)
        top_layout.setStretch(1, 1)

        layout = QVBoxLayout()
        layout.addLayout(top_layout)
        layout.addWidget(self.list_view)
        # todo: Without the sizeHint() call the minimum size is not set correctly
        #       This needs some investigation as to why this is.
        layout.sizeHint()
        self.setLayout(layout)

    def set_plot_list(self, plot_list):
        """
        Populate the GUI elements with the data that comes from Presenter.
        :param data: a list of algorithm descriptors.
        """
        self.plot_list.clear()
        for plot in plot_list:
            item = QStandardItem(plot)
            item.setEditable(False)
            self.plot_list.appendRow(item)

    def refresh_plot_list(self):
        self.presenter.update_plot_list()

    def double_clicked_item(self):
        index = self.list_view.currentIndex()
        plot_title = index.data(Qt.DisplayRole)
        print("Double clicked on " + index.data(Qt.DisplayRole))
        self.presenter.make_plot_active(plot_title)
