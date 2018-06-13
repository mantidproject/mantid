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
from qtpy.QtWidgets import (QAbstractItemView, QHBoxLayout, QLabel, QLineEdit, QListWidget, QListWidgetItem, QMenu,
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

        # These must happen in the main GUI thread, else segfaults
        self.set_plot_list_orig = self.set_plot_list
        self.set_plot_list = QAppThreadCall(self.set_plot_list_orig)
        self.append_to_plot_list_orig = self.append_to_plot_list
        self.append_to_plot_list = QAppThreadCall(self.append_to_plot_list_orig)
        self.remove_from_plot_list_orig = self.remove_from_plot_list
        self.remove_from_plot_list = QAppThreadCall(self.remove_from_plot_list_orig)

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

    # ------------------------ Plot Updates ------------------------

    def _add_to_plot_list(self, plot_name):
        real_item = PlotNameWidget(self.presenter, plot_name, self)
        widget_item = QListWidgetItem()
        size_hint = real_item.sizeHint()
        widget_item.setSizeHint(size_hint)
        self.list_widget.addItem(widget_item)
        self.list_widget.setItemWidget(widget_item, real_item)

    def set_plot_list(self, plot_list):
        """
        Populate the plot list from the Presenter
        :param plot_list: the list of plot names (list of strings)
        """
        self.list_widget.clear()
        for plot_name in plot_list:
            self._add_to_plot_list(plot_name)

    def append_to_plot_list(self, plot_name):
        self._add_to_plot_list(plot_name)

    def _get_row_and_widget_from_label_text(self, label_text):
        for row in range(len(self.list_widget)):
            item = self.list_widget.item(row)
            widget = self.list_widget.itemWidget(item)
            if widget.plot_name == label_text:
                return row, widget

    def remove_from_plot_list(self, plot_name):
        row, widget = self._get_row_and_widget_from_label_text(plot_name)
        taken_item = self.list_widget.takeItem(row)
        del taken_item
        return

    def rename_in_plot_list(self, new_name, old_name):
        row, widget = self._get_row_and_widget_from_label_text(old_name)
        widget.set_plot_name(new_name)

    # ----------------------- Plot Selection ------------------------

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
            selected_plots.append(widget.plot_name)
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
        return widget.plot_name

    def get_filter_text(self):
        """
        Returns the currently set text in the filter box
        :return: A string with current filter text
        """
        return self.filter_box.text()


class PlotNameWidget(QWidget):
    def __init__(self, presenter, plot_name="", parent=None):
        super(PlotNameWidget, self).__init__(parent)

        self.presenter = presenter
        self.plot_name = plot_name

        self.line_edit = QLineEdit(self.plot_name)
        self.line_edit.setReadOnly(True)
        self.line_edit.setFrame(False)
        self.line_edit.setStyleSheet("* { background-color: rgba(0, 0, 0, 0); }")
        self.line_edit.setAttribute(Qt.WA_TransparentForMouseEvents, True)
        self.line_edit.editingFinished.connect(self.rename_plot)

        self.rename_icon = QIcon.fromTheme('insert-text')
        self.rename_button = QPushButton(self.rename_icon, "")
        self.rename_button.setFlat(True)
        self.rename_button.setMaximumWidth(self.rename_button.sizeHint().width())
        self.rename_button.pressed.connect(self.toggle_plot_name_editable)

        self.close_icon = QIcon.fromTheme('window-close')
        self.close_button = QPushButton(self.close_icon, "")
        self.close_button.setFlat(True)
        self.close_button.setMaximumWidth(self.close_button.sizeHint().width())
        self.close_button.clicked.connect(lambda: self.close_pressed(self.line_edit.text()))

        self.layout = QHBoxLayout()
        self.layout.addWidget(self.line_edit)
        self.layout.addWidget(self.rename_button)
        self.layout.addWidget(self.close_button)

        self.layout.sizeHint()
        self.setLayout(self.layout)

        # Get rid of the top and bottom margins - the button provides
        # some natural margin anyway
        margins = self.layout.contentsMargins()
        self.layout.setContentsMargins(margins.left(), 0, margins.right(), 0)

        # Add the context menu
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.context_menu = self._make_context_menu()
        self.customContextMenuRequested.connect(self.context_menu_opened)

    def set_plot_name(self, new_name):
        self.plot_name = new_name
        self.line_edit.setText(new_name)

    def make_active_pressed(self, plot_name):
        self.presenter.make_plot_active(plot_name)

    def close_pressed(self, plot_name):
        self.presenter.close_single_plot(plot_name)

    def toggle_plot_name_editable(self, editable=None):
        if editable is None:
            editable = self.line_edit.isReadOnly()
        self.line_edit.setReadOnly(not editable)
        self.line_edit.setAttribute(Qt.WA_TransparentForMouseEvents, not editable)
        if editable:
            self.line_edit.setFocus()
            self.line_edit.selectAll()

    def rename_plot(self):
        self.presenter.rename_figure(self.line_edit.text(), self.plot_name)
        self.toggle_plot_name_editable(False)

    def _make_context_menu(self):
        context_menu = QMenu()
        context_menu.addAction("Make Active", lambda: self.make_active_pressed(self.plot_name))
        context_menu.addAction("Rename", lambda: self.toggle_plot_name_editable(True))
        context_menu.addAction("Close", lambda: self.close_pressed(self.plot_name))
        return context_menu

    def context_menu_opened(self, position):
        self.context_menu.exec_(self.mapToGlobal(position))
