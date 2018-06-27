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

import re

from qtpy.QtCore import Qt, Signal
from qtpy.QtGui import QColor, QIcon
from qtpy.QtWidgets import (QAbstractItemView, QAction, QActionGroup, QFileDialog, QHBoxLayout, QLineEdit, QListWidget,
                            QListWidgetItem, QMenu, QPushButton, QVBoxLayout, QWidget)

from mantidqt.utils.flowlayout import FlowLayout
from mantidqt.py3compat import Enum
from workbench.plotting.figuremanager import QAppThreadCall

export_types = [
    ('Export to EPS', '.eps'),
    ('Export to PDF', '.pdf'),
    ('Export to PNG', '.png'),
    ('Export to SVG', '.svg')
]


class SortType(Enum):
    Name = 0
    LastShown = 1


class PlotSelectorView(QWidget):
    """
    The view to the plot selector, a PyQt widget.
    """

    DEBUG_MODE = False

    # A signal to capture when keys are pressed
    deleteKeyPressed = Signal(int)
    enterKeyPressed = Signal(int)

    def __init__(self, presenter, parent=None, is_run_as_unit_test=False):
        """
        Initialise a new instance of PlotSelectorWidget
        :param presenter: The presenter controlling this view
        :param parent: Optional - the parent QWidget
        :param is_run_as_unit_test: Optional - True if this is
        running as a unit test, in which case skip file dialogs
        """
        super(PlotSelectorView, self).__init__(parent)
        self.presenter = presenter
        self.is_run_as_unit_test = is_run_as_unit_test

        self.sort_order = Qt.AscendingOrder
        self.sort_type = SortType.Name

        self.show_button = QPushButton('Show')
        self.select_all_button = QPushButton('Select All')
        self.close_button = QPushButton('Close')
        self.sort_button = self._make_sort_button()
        self.export_button = self._make_export_button()
        self.filter_box = self._make_filter_box()
        self.list_widget = self._make_list_widget()

        # Add the context menu
        self.list_widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.context_menu, self.export_menu = self._make_context_menu()
        self.list_widget.customContextMenuRequested.connect(self.context_menu_opened)

        buttons_layout = FlowLayout()
        buttons_layout.setSpacing(1)
        buttons_layout.addWidget(self.show_button)
        buttons_layout.addWidget(self.select_all_button)
        buttons_layout.addWidget(self.close_button)
        buttons_layout.addWidget(self.sort_button)
        buttons_layout.addWidget(self.export_button)

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
        self.list_widget.doubleClicked.connect(self.presenter.show_single_selected)
        self.filter_box.textChanged.connect(self.presenter.filter_text_changed)
        self.show_button.clicked.connect(self.presenter.show_multiple_selected)
        self.select_all_button.clicked.connect(self.list_widget.selectAll)
        self.close_button.clicked.connect(self.presenter.close_action_called)
        self.deleteKeyPressed.connect(self.presenter.close_action_called)

        if self.DEBUG_MODE:
            self.list_widget.clicked.connect(self.show_debug_info)

    def show_debug_info(self):
        item = self.list_widget.currentItem()
        widget = self.list_widget.itemWidget(item)
        print("Plot name: {}".format(widget.plot_name))
        print("Plot text: {}".format(widget.line_edit.text()))
        print("Sort key: {}".format(item.data(Qt.InitialSortOrderRole)))

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

    def _make_list_widget(self):
        """
        Make a list showing the names of the plots, with close buttons
        :return: A QListWidget object which will contain plot widgets
        """
        list_widget = QListWidget(self)
        list_widget.setSelectionMode(QAbstractItemView.ExtendedSelection)
        list_widget.setSortingEnabled(True)
        return list_widget

    def _make_context_menu(self):
        context_menu = QMenu()
        context_menu.addAction("Show", self.presenter.show_multiple_selected)
        context_menu.addAction("Rename", self.rename_selected_in_context_menu)

        export_menu = context_menu.addMenu("Export")
        for text, extension in export_types:
            export_menu.addAction(text, lambda ext=extension: self.export_plots(ext))

        context_menu.addAction("Close", self.presenter.close_action_called)
        return context_menu, export_menu

    def context_menu_opened(self, position):
        self.context_menu.exec_(self.mapToGlobal(position))

    # ------------------------ Plot Updates ------------------------

    def append_to_plot_list(self, plot_name, is_shown_by_filter):
        real_item = PlotNameWidget(self.presenter, plot_name, self, self.is_run_as_unit_test)
        widget_item = HumanReadableSortItem()
        sort_key = self.presenter.get_initial_sort_key(plot_name)
        widget_item.setData(Qt.InitialSortOrderRole, sort_key)
        size_hint = real_item.sizeHint()
        widget_item.setSizeHint(size_hint)
        self.list_widget.addItem(widget_item)
        self.list_widget.setItemWidget(widget_item, real_item)
        widget_item.setHidden(not is_shown_by_filter)

    def set_plot_list(self, plot_list):
        """
        Populate the plot list from the Presenter. This is reserved
        for a 'things have gone wrong' scenario, and should only be
        used when errors are encountered.
        :param plot_list: the list of plot names (list of strings)
        """
        self.list_widget.clear()
        self.filter_box.clear()
        for plot_name in plot_list:
            self.append_to_plot_list(plot_name, True)

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
        self.list_widget.item(row).setData(Qt.InitialSortOrderRole, new_name)
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
            if not item.isHidden():
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
        if widget is None or item.isHidden():
            return None
        return widget.plot_name

    def get_filter_text(self):
        """
        Returns the currently set text in the filter box
        :return: A string with current filter text
        """
        return self.filter_box.text()

    # ----------------------- Plot Filtering ------------------------

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

    def unhide_all_plots(self):
        for row in range(len(self.list_widget)):
            item = self.list_widget.item(row)
            item.setHidden(False)

    def filter_plot_list(self, plot_list):
        for row in range(len(self.list_widget)):
            item = self.list_widget.item(row)
            widget = self.list_widget.itemWidget(item)
            if widget.plot_name in plot_list:
                item.setHidden(False)
            else:
                item.setHidden(True)

    # ------------------------ Plot Renaming ------------------------

    def rename_selected_in_context_menu(self):
        plot_name = self.get_currently_selected_plot_name()
        row, widget = self._get_row_and_widget_from_label_text(plot_name)
        widget.toggle_plot_name_editable(True)

    # ----------------------- Plot Sorting --------------------------
    """
    How the sorting works
    
    The QListWidgetItem has a text string set via .setData with the
    Qt.InitialSortOrderRole as the role for this string (not strictly
    used as intended in Qt). If the sorting is by name this is just
    the name of the plot.
    
    If sorting by last active this is the number the plot was last 
    active, or if never active it is the plot name with an 'A' 
    appended to the front. For example ['1', '2', 'AUnshownPlot'].
    
    QListWidgetItem is subclassed by HumanReadableSortItem to
    override the < operator. This uses the text with the 
    InitialSortOrderRole to sort, and sorts in a human readable way,
    for example ['Figure 1', 'Figure 2', 'Figure 10'] as opposed to
    the ['Figure 1', 'Figure 10', 'Figure 2'].
    """

    def _make_sort_button(self):
        sort_button = QPushButton("Sort")
        sort_menu = QMenu()

        ascending_action = QAction("Ascending", sort_menu, checkable=True)
        ascending_action.setChecked(True)
        ascending_action.toggled.connect(self.presenter.set_sort_order)
        descending_action = QAction("Descending", sort_menu, checkable=True)

        order_group = QActionGroup(sort_menu)
        order_group.addAction(ascending_action)
        order_group.addAction(descending_action)

        name_action = QAction("Name", sort_menu, checkable=True)
        name_action.setChecked(True)
        name_action.toggled.connect(self.presenter.set_sort_type)
        last_shown_action = QAction("Last Shown", sort_menu, checkable=True)

        sort_type_group = QActionGroup(sort_menu)
        sort_type_group.addAction(name_action)
        sort_type_group.addAction(last_shown_action)

        sort_menu.addAction(ascending_action)
        sort_menu.addAction(descending_action)
        sort_menu.addSeparator()
        sort_menu.addAction(name_action)
        sort_menu.addAction(last_shown_action)

        sort_button.setMenu(sort_menu)
        return sort_button

    def sort_ascending(self):
        self.sort_order = Qt.AscendingOrder
        self.list_widget.sortItems(self.sort_order)

    def sort_descending(self):
        self.sort_order = Qt.DescendingOrder
        self.list_widget.sortItems(self.sort_order)

    def sort_by_name(self):
        self.sort_type = SortType.Name
        self.list_widget.sortItems(self.sort_order)

    def sort_by_last_shown(self):
        self.sort_type = SortType.LastShown
        self.list_widget.sortItems(self.sort_order)

    def set_sort_keys(self, sort_names_dict):
        self.list_widget.setSortingEnabled(False)
        for row in range(len(self.list_widget)):
            item = self.list_widget.item(row)
            widget = self.list_widget.itemWidget(item)
            item.setData(Qt.InitialSortOrderRole, str(sort_names_dict[widget.plot_name]))
        self.list_widget.sortItems(self.sort_order)
        self.list_widget.setSortingEnabled(True)

    # ---------------------- Plot Exporting -------------------------

    def _make_export_button(self):
        export_button = QPushButton("Export")
        export_menu = QMenu()
        for text, extension in export_types:
            export_menu.addAction(text, lambda ext=extension: self.export_plots(ext))
        export_button.setMenu(export_menu)
        return export_button

    def export_plots(self, extension):
        path = ""
        if not self.is_run_as_unit_test:
            path = QFileDialog.getExistingDirectory(None, 'Select folder for exported plots')
        self.presenter.export_plots(path, extension)


class PlotNameWidget(QWidget):
    def __init__(self, presenter, plot_name="", parent=None, is_run_as_unit_test=False):
        super(PlotNameWidget, self).__init__(parent)

        self.presenter = presenter
        self.plot_name = plot_name
        self.is_run_as_unit_test = is_run_as_unit_test

        self.line_edit = QLineEdit(self.plot_name)
        self.line_edit.setReadOnly(True)
        self.line_edit.setFrame(False)
        self.line_edit.setStyleSheet("* { background-color: rgba(0, 0, 0, 0); }")
        self.line_edit.setAttribute(Qt.WA_TransparentForMouseEvents, True)
        self.line_edit.editingFinished.connect(self.rename_plot)

        rename_icon = QIcon.fromTheme('insert-text')
        self.rename_button = QPushButton(rename_icon, "")
        self.rename_button.setFlat(True)
        self.rename_button.setMaximumWidth(self.rename_button.iconSize().width() * 2)
        self.rename_button.setCheckable(True)
        self.rename_button.toggled.connect(self.rename_button_toggled)
        self.skip_next_toggle = False

        close_icon = QIcon.fromTheme('window-close')
        self.close_button = QPushButton(close_icon, "")
        self.close_button.setFlat(True)
        self.close_button.setMaximumWidth(self.close_button.iconSize().width() * 2)
        self.close_button.clicked.connect(lambda: self.close_pressed(self.line_edit.text()))

        self.layout = QHBoxLayout()

        # Get rid of the top and bottom margins - the button provides
        # some natural margin anyway. Get rid of right margin and
        # reduce spacing to get buttons closer together.
        margins = self.layout.contentsMargins()
        self.layout.setContentsMargins(5, 0, 0, 0)
        self.layout.setSpacing(0)

        self.layout.addWidget(self.line_edit)
        self.layout.addWidget(self.rename_button)
        self.layout.addWidget(self.close_button)

        self.layout.sizeHint()
        self.setLayout(self.layout)

    def set_plot_name(self, new_name):
        self.plot_name = new_name
        self.line_edit.setText(new_name)

    def show_pressed(self, plot_name):
        self.presenter.show_plot(plot_name)

    def close_pressed(self, plot_name):
        self.presenter.close_single_plot(plot_name)

    def rename_button_toggled(self, checked):
        if checked:
            self.toggle_plot_name_editable(True, toggle_rename_button=False)

    def toggle_plot_name_editable(self, editable, toggle_rename_button=True):
        self.line_edit.setReadOnly(not editable)
        self.line_edit.setAttribute(Qt.WA_TransparentForMouseEvents, not editable)

        # This is a sneaky way to avoid the issue of two calls to
        # this toggle method, by effectively disabling the button
        # press in edit mode.
        self.rename_button.setAttribute(Qt.WA_TransparentForMouseEvents, editable)

        if toggle_rename_button:
            self.rename_button.setChecked(editable)

        if editable:
            self.line_edit.setFocus()
            self.line_edit.selectAll()
        else:
            self.line_edit.setSelection(0, 0)

    def rename_plot(self):
        self.presenter.rename_figure(self.line_edit.text(), self.plot_name)
        self.toggle_plot_name_editable(False)

    def export_plot(self, extension):
        path = ""
        if not self.is_run_as_unit_test:
            path = QFileDialog.getExistingDirectory(None, 'Select folder for exported plot')
        self.presenter.export_plots(self.plot_name, path, extension)


class HumanReadableSortItem(QListWidgetItem):
    def convert(self, text):
        return int(text) if text.isdigit() else text.lower()

    def __lt__(self, other):
        self_key = [self.convert(c) for c in re.split('([0-9]+)', self.data(Qt.InitialSortOrderRole))]
        other_key = [self.convert(c) for c in re.split('([0-9]+)', other.data(Qt.InitialSortOrderRole))]
        return self_key < other_key

