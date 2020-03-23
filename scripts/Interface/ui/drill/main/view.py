# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import (QMainWindow, QWidget, QFileDialog, QSizePolicy)
from qtpy.QtGui import QIcon
from qtpy.QtCore import *
from qtpy import uic
import os
from abc import ABCMeta, abstractmethod
from six import with_metaclass

from mantidqt.widgets import (manageuserdirectories, instrumentselector)
from mantidqt.widgets.jobtreeview import *
from mantidqt import icons
from mantid.kernel import UsageService, FeatureType, config, logger
from .specifications import RundexSettings


class DrillEventListener(with_metaclass(ABCMeta, object)):
    """
    Defines the elements which a presenter can listen to in this View
    """

    @abstractmethod
    def on_load_rundex_clicked(self):
        pass

    @abstractmethod
    def on_save_rundex_clicked(self):
        pass

    @abstractmethod
    def on_process_clicked(self, contents):
        pass

    @abstractmethod
    def on_row_inserted(self):
        pass

    @abstractmethod
    def on_rows_removed(self, rows):
        pass

    @abstractmethod
    def on_copy_rows_requested(self, rows):
        pass

    @abstractmethod
    def on_paste_rows_requested(self):
        pass

    @abstractmethod
    def on_insert_row(self):
        pass

    @abstractmethod
    def on_erase_rows(self, rows):
        pass

    @abstractmethod
    def on_cut_rows(self, rows):
        pass

    @abstractmethod
    def on_data_changed(self, row, content):
        pass

    @abstractmethod
    def on_instrument_changed(self, instrument):
        pass


class DrillJobTreeView(JobTreeView):

    def __init__(self, columns):
        JobTreeView.__init__(self, columns, self.cell_style(''))
        self.setRootIsDecorated(False)
        self.add_row([''] * len(columns))

    def add_row(self, value):
        value = [self.cell_style(x) for x in value]
        self.appendChildRowOf(self.row([]), value)

    def row(self, path):
        return RowLocation(path)

    def cell_style(self, text):
        background_color = 'white'
        border_thickness = 1
        border_color = 'black'
        border_opacity = 255
        is_editable = True
        return Cell(text, background_color, border_thickness,
                    border_color, border_opacity, is_editable)


class DrillView(QMainWindow):

    def __init__(self):
        super(DrillView, self).__init__()
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, 'main.ui'), self)
        self.job_tree_view = None
        self.settings_listeners = []
        self.setup_header()
        self.set_table(list())
        self.buffer = list()  # for copy-paste actions

    def setup_header(self):
        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.instrumentselector.instrumentSelectionChanged.connect(
                self.change_instrument_requested)
        self.headerLeft.addWidget(self.instrumentselector, 0, Qt.AlignLeft)

        self.datadirs.setIcon(icons.get_icon("mdi.folder"))
        self.datadirs.clicked.connect(self.show_directory_manager)

        self.load.setIcon(icons.get_icon("mdi.file-import"))
        self.load.clicked.connect(self.load_rundex)

        self.settings.setIcon(icons.get_icon("mdi.settings"))
        self.settings.clicked.connect(self.show_settings)

        self.paste.setIcon(icons.get_icon("mdi.content-paste"))
        self.paste.clicked.connect(self.paste_rows)

        self.copy.setIcon(icons.get_icon("mdi.content-copy"))
        self.copy.clicked.connect(self.copy_rows)

        self.cut.setIcon(icons.get_icon("mdi.content-cut"))
        self.cut.clicked.connect(self.cut_rows)

        self.erase.setIcon(icons.get_icon("mdi.eraser"))
        self.erase.clicked.connect(self.erase_rows)

        self.deleterow.setIcon(icons.get_icon("mdi.table-row-remove"))
        self.deleterow.clicked.connect(self.remove_rows_from_button)

        self.addrow.setIcon(icons.get_icon("mdi.table-row-plus-after"))

        self.save.setIcon(icons.get_icon("mdi.file-export"))

        self.processRows.setIcon(icons.get_icon("mdi.play"))
        self.processRows.clicked.connect(self.process_selected_rows)

        self.processAll.setIcon(icons.get_icon("mdi.sigma"))
        self.processAll.clicked.connect(self.process_all)

        self.stop.setIcon(icons.get_icon("mdi.stop"))

    def add_listener(self, listener):
        if not isinstance(listener, DrillEventListener):
            raise ValueError(
                "The listener is not of type DrillEventListener but rather {}".format(type(listener)))
        self.settings_listeners.append(listener)

    def clear_listeners(self):
        self.settings_listeners = []

    def call_settings_listeners(self, target):
        for listener in self.settings_listeners:
            target(listener)

    def process_all(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["Drill", "Process all button clicked"], False)
        all = self.get_all_rows()
        contents = []
        for row in all:
            contents.append(self.get_row_contents(RowLocation([row])))
        self.call_settings_listeners(
            lambda listener: listener.on_process_clicked(contents))

    def process_selected_rows(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["Drill", "Process rows button clicked"], False)
        selected = self.get_selected_rows()
        contents = []
        for row in selected:
            contents.append(self.get_row_contents(RowLocation([row])))
        self.call_settings_listeners(
            lambda listener: listener.on_process_clicked(contents))

    def get_selected_rows(self):
        rows = self.job_tree_view.selectedRowLocations()
        return rows

    def get_all_rows(self):
        row_locations = self.job_tree_view.allRowLocations()
        rows = [x.rowRelativeToParent() for x in row_locations]
        return rows

    def get_row_contents(self, row_location):
        cell_data = self.job_tree_view.cellsAt(row_location)
        return [str(x.contentText()) for x in cell_data]

    def show_settings(self):
        settings = QMainWindow(self)
        uic.loadUi(os.path.join(self.here, self.technique + '_settings.ui'), settings)
        settings.show()

    def data_changed(self, row_location, column, old_value, new_value):
        row = row_location.rowRelativeToParent()
        self.call_settings_listeners(
            lambda listener: listener.on_data_changed(row, self.get_row_contents(row_location)))

    def row_inserted(self, row_location):
        if row_location.depth() > 1:
            self.job_tree_view.removeRowAt(row_location)

    def append_and_edit_at_row_below_requested(self):
        self.job_tree_view.appendAndEditAtRowBelow()

    def edit_at_row_above_requested(self):
        self.job_tree_view.editAtRowAbove()

    def remove_rows(self, rows):
        self.job_tree_view.removeRows(rows)

    def remove_rows_from_button(self):
        rows = self.get_selected_rows()
        self.remove_rows(rows)

    def copy_rows(self):
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Copy rows button"], False)
        rows = self.get_selected_rows()
        self.buffer = list()
        for row in rows:
            self.buffer.append(self.get_row_contents(row))

    def erase_rows(self):
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Erase rows button"], False)
        rows = self.get_selected_rows()
        length = len(self.job_tree_view.cellsAt(rows[0]))
        for row in rows:
            for cell in range(0, length):
                self.job_tree_view.setCellAt(row, cell,
                                             self.job_tree_view.cell_style(""))

    def cut_rows(self):
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Cut rows button"], False)
        rows = self.get_selected_rows()
        rows = self.get_selected_rows()
        self.buffer = list()
        for row in rows:
            self.buffer.append(self.get_row_contents(row))
        self.remove_rows(rows)

    def paste_rows(self):
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Paste rows button"], False)
        position = self.get_selected_rows()[-1]
        indice = position.rowRelativeToParent() + 1
        for row in self.buffer:
            position = self.job_tree_view.insertChildRowOf(
                    position.parent(), position.rowRelativeToParent() + 1)
            column = 0
            for txt in row:
                self.job_tree_view.setCellAt(position, column,
                                             self.job_tree_view.cell_style(txt))
                column += 1

    def load_rundex(self):
        fname = QFileDialog.getOpenFileName(self, 'Load rundex', '~')

    def change_instrument_requested(self, instrument):
        self.call_settings_listeners(
                lambda listener: listener.on_instrument_changed(instrument)
                )

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories(self).exec_()

    def set_table(self, columns):
        if self.job_tree_view:
            self.job_tree_view.setParent(None)

        # table widget
        self.job_tree_view = DrillJobTreeView(columns)
        self.center.addWidget(self.job_tree_view)

        # signals connection
        self.table_signals = JobTreeViewSignalAdapter(self.job_tree_view, self)
        self.table_signals.cellTextChanged.connect(self.data_changed)
        self.table_signals.rowInserted.connect(self.row_inserted)
        self.table_signals.appendAndEditAtRowBelowRequested.connect(
                self.append_and_edit_at_row_below_requested)
        self.table_signals.editAtRowAboveRequested.connect(
                self.edit_at_row_above_requested)
        self.table_signals.removeRowsRequested.connect(self.remove_rows)
        self.table_signals.copyRowsRequested.connect(self.copy_rows)
        self.table_signals.pasteRowsRequested.connect(self.paste_rows)
        self.table_signals.cutRowsRequested.connect(self.cut_rows)

