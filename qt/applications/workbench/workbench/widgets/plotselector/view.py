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

from qtpy.QtCore import Qt, Signal
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import (QAbstractItemView, QHBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem,
                            QPushButton, QVBoxLayout, QWidget)

from workbench.plotting.figuremanager import QAppThreadCall

class PlotSelectorView(QWidget):
    """
    The view to the plot selector, a PyQt widget.
    """

    # A signal to capture when delete is pressed
    deleteKeyPressed = Signal(int)

    def __init__(self, presenter, parent=None):
        """
        Initialise a new instance of PlotSelectorWidget
        :param presenter: The presenter controlling this view
        :param parent: Optional - the parent QWidget
        """
        super(PlotSelectorView, self).__init__(parent)
        self.presenter = presenter

        self.close_button = QPushButton('Close')
        self.filter_box = self._make_filter_box()
        self.list_widget = self._make_list_widget()

        buttons_layout = QHBoxLayout()
        buttons_layout.addWidget(self.close_button)
        buttons_layout.setStretch(1, 1)

        filter_layout = QHBoxLayout()
        filter_layout.addWidget(self.filter_box)

        layout = QVBoxLayout()
        layout.addLayout(buttons_layout)
        layout.addLayout(filter_layout)
        layout.addWidget(self.list_widget)
        # todo: Without the sizeHint() call the minimum size is not set correctly
        #       This needs some investigation as to why this is.
        layout.sizeHint()
        self.setLayout(layout)

        # This must happen in the main GUI thread, else segfaults
        self.set_plot_list_orig = self.set_plot_list
        self.set_plot_list = QAppThreadCall(self.set_plot_list_orig)

        # Connect presenter methods to things in the view
        self.list_widget.doubleClicked.connect(self.presenter.list_double_clicked)
        self.filter_box.textChanged.connect(self.presenter.filter_text_changed)
        self.close_button.clicked.connect(self.presenter.close_action_called)
        self.deleteKeyPressed.connect(self.presenter.close_action_called)

    def keyPressEvent(self, event):
        """
        This overrides keyPressEvent from QWidget to emit a signal
        whenever the delete key is pressed.

        This might be better to override on list_view, but there is
        only ever an active selection when focused on the list.
        :param event: A QKeyEvent holding the key that was pressed
        """
        super(PlotSelectorView, self).keyPressEvent(event)
        if event.key() == Qt.Key_Delete:
            self.deleteKeyPressed.emit(event.key())

    def _make_filter_box(self):
        """
        Make the text box to filter the plots by name
        :return: A QLineEdit object with placeholder text and a
                 clear button
        """
        text_box = QLineEdit(self)
        text_box.setPlaceholderText("Filter Plots")
        text_box.setClearButtonEnabled(True)
        return text_box

    def _make_list_widget(self):
        """
        Make a list showing the names of the plots, with close buttons
        :return: A QListWidget object which will contain plot widgets
        """
        list_widget = QListWidget(self)
        list_widget.setSelectionMode(QAbstractItemView.ExtendedSelection)

        list_widget.installEventFilter(self)
        return list_widget

    def set_plot_list(self, plot_list):
        """
        Populate the plot list from the Presenter
        :param plot_list: the list of plot names (list of strings)
        """
        self.list_widget.clear()
        for plot_name in plot_list:
            real_item = WidgetItemWithCloseButton(self.presenter, plot_name)
            widget_item = QListWidgetItem()
            size_hint = real_item.sizeHint()
            widget_item.setSizeHint(size_hint)
            self.list_widget.addItem(widget_item)
            self.list_widget.setItemWidget(widget_item, real_item)

    def get_all_selected_plot_names(self):
        """
        Returns a list with the names of all the currently selected
        plots
        :return: A list of strings with the plot names (figure titles)
        """
        selected = self.list_widget.selectedItems()
        selected_plots = []
        for item in selected:
            widget = self.list_widget.itemWidget(item)
            selected_plots.append(widget.label.text())
        return selected_plots

    def get_currently_selected_plot_name(self):
        """
        Returns a string with the plot name for the currently active
        plot
        :return: A string with the plot name (figure title)
        """
        item = self.list_widget.currentItem()
        widget = self.list_widget.itemWidget(item)
        if widget is None:
            return None
        return widget.label.text()

    def get_filter_text(self):
        """
        Returns the currently set text in the filter box
        :return: A string with current filter text
        """
        return self.filter_box.text()


class WidgetItemWithCloseButton(QWidget):
    def __init__(self, presenter, text="", parent=None):
        super(WidgetItemWithCloseButton, self).__init__(parent)

        self.presenter = presenter

        self.label = QLabel(text)
        self.close_icon = QIcon.fromTheme('window-close')
        self.close_button = QPushButton(self.close_icon, "")
        self.close_button.sizeHint()
        self.close_button.setMaximumWidth(self.close_button.sizeHint().width())
        self.close_button.pressed.connect(lambda: self.x_pressed(self.label.text()))

        self.layout = QHBoxLayout()
        self.layout.addWidget(self.label)
        self.layout.addWidget(self.close_button)

        self.layout.sizeHint()

        self.setLayout(self.layout)

    def x_pressed(self, plot_name):
        self.presenter.single_close_requested(plot_name)
