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
from qtpy.QtWidgets import (QAbstractItemView, QFileDialog, QHBoxLayout, QLineEdit, QListWidget, QListWidgetItem, QMenu,
                            QPushButton, QVBoxLayout, QWidget)

from mantidqt.utils.flowlayout import FlowLayout
from workbench.plotting.figuremanager import QAppThreadCall

export_types = [
    ('Export to EPS', '.eps'),
    ('Export to PDF', '.pdf'),
    ('Export to PNG', '.png'),
    ('Export to SVG', '.svg')
]


class PlotSelectorView(QWidget):
    """
    The view to the plot selector, a PyQt widget.
    """

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

        self.show_button = QPushButton('Show')
        self.select_all_button = QPushButton('Select All')
        self.close_button = QPushButton('Close')
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
        self.enterKeyPressed.connect(self.presenter.show_multiple_selected)

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
        elif event.key() == Qt.Key_Enter or event.key() == Qt.Key_Return:
            self.enterKeyPressed.emit(event.key())

    def _make_list_widget(self):
        """
        Make a list showing the names of the plots, with close buttons
        :return: A QListWidget object which will contain plot widgets
        """
        list_widget = QListWidget(self)
        list_widget.setSelectionMode(QAbstractItemView.ExtendedSelection)
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

    def _add_to_plot_list(self, plot_name):
        real_item = PlotNameWidget(self.presenter, plot_name, self, self.is_run_as_unit_test)
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

    # ------------------------ Plot Renaming ------------------------

    def rename_selected_in_context_menu(self):
        plot_name = self.get_currently_selected_plot_name()
        row, widget = self._get_row_and_widget_from_label_text(plot_name)
        widget.toggle_plot_name_editable(True)

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

