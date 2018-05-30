#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
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
from __future__ import absolute_import, print_function

from qtpy.QtCore import Qt
from qtpy.QtGui import (QStandardItem, QStandardItemModel)
from qtpy.QtWidgets import (QAbstractItemView, QListView, QWidget, QPushButton, QVBoxLayout, QHBoxLayout, QLineEdit)


class PlotSelectorWidget(QWidget):
    """
    The view to the plot selector, a PyQt widget.
    """
    def __init__(self, presenter, parent=None):
        """
        Initialise a new instance of PlotSelectorWidget
        :param presenter: The presenter that created this class
        :param parent: A parent QWidget
        """
        QWidget.__init__(self, parent)
        self.presenter = presenter

        self.close_button = QPushButton('Close')
        self.filter_box = self._make_filter_box()
        self.list_view = self._make_plot_list()

        buttons_layout = QHBoxLayout()
        buttons_layout.addWidget(self.close_button)
        buttons_layout.setStretch(1, 1)

        filter_layout = QHBoxLayout()
        filter_layout.addWidget(self.filter_box)

        layout = QVBoxLayout()
        layout.addLayout(buttons_layout)
        layout.addLayout(filter_layout)
        layout.addWidget(self.list_view)
        # todo: Without the sizeHint() call the minimum size is not set correctly
        #       This needs some investigation as to why this is.
        layout.sizeHint()
        self.setLayout(layout)

        # Connect presenter methods to things in the view
        self.list_view.doubleClicked.connect(self.presenter.list_double_clicked)
        self.filter_box.textChanged.connect(self.presenter.filter_text_changed)
        self.close_button.clicked.connect(self.presenter.close_button_clicked)

    def _make_filter_box(self):
        """
        Make the filter by plot name box
        :return: A QLineEdit object with text and a clear button
        """
        text_box = QLineEdit(self)
        text_box.setPlaceholderText("Filter Plots")
        #text_box.setClearButtonEnabled(True)
        return text_box

    def _make_plot_list(self):
        """
        Make a list showing the names of the plots
        :return: A QListView object which will contain plot names
        """
        list_view = QListView(self)
        list_view.setSelectionMode(QAbstractItemView.ExtendedSelection)
        plot_list = QStandardItemModel(list_view)
        list_view.setModel(plot_list)
        return list_view

    def set_plot_list(self, plot_list):
        """
        Populate the plot list from the Presenter
        :param plot_list: the list of plot names (list of strings)
        """
        self.list_view.model().clear()
        for plot_name in plot_list:
            item = QStandardItem(plot_name)
            item.setEditable(False)
            self.list_view.model().appendRow(item)

    def get_all_selected_plot_names(self):
        selected = self.list_view.selectedIndexes()
        selected_plots = []
        for item in selected:
            selected_plots.append(item.data(Qt.DisplayRole))
        return selected_plots

    def get_currently_selected_plot_name(self):
        index = self.list_view.currentIndex()
        return index.data(Qt.DisplayRole)

    def get_filter_text(self):
        return self.filter_box.text()
