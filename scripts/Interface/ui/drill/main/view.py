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
    def on_copy_rows_requested(self):
        pass

    @abstractmethod
    def on_paste_rows_requested(self):
        pass

    @abstractmethod
    def on_insert_row(self):
        pass

    @abstractmethod
    def on_erase_rows(self):
        pass

    @abstractmethod
    def on_cut_rows(self):
        pass

    @abstractmethod
    def on_data_changed(self, row, content):
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

    settings_listeners = []
    job_tree_view = None

    def __init__(self, instrument):
        super(DrillView, self).__init__()
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, 'main.ui'), self)
        self.instrument = instrument
        self.technique = RundexSettings.get_technique(instrument)
        self.setup_header()
        self.setup_center()
        self.show()

    def setup_header(self):
        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.instrumentselector.instrumentSelectionChanged.connect(self.choose_instrument)
        self.headerLeft.addWidget(self.instrumentselector, 0, Qt.AlignLeft)
        self.datadirs.setIcon(icons.get_icon("mdi.folder"))
        self.datadirs.clicked.connect(self.show_directory_manager)
        self.load.setIcon(icons.get_icon("mdi.file-import"))
        self.load.clicked.connect(self.load_rundex)
        self.settings.setIcon(icons.get_icon("mdi.settings"))
        self.settings.clicked.connect(self.show_settings)
        self.paste.setIcon(icons.get_icon("mdi.content-paste"))
        self.copy.setIcon(icons.get_icon("mdi.content-copy"))
        self.cut.setIcon(icons.get_icon("mdi.content-cut"))
        self.erase.setIcon(icons.get_icon("mdi.eraser"))
        self.deleterow.setIcon(icons.get_icon("mdi.table-row-remove"))
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
        row_locations = self.job_tree_view.selectedRowLocations()
        rows = [x.rowRelativeToParent() for x in row_locations]
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

    def setup_center(self):
        if hasattr(self, 'job_tree_view'):
            if self.job_tree_view:
                self.job_tree_view.setParent(None)
                self.job_tree_view = None
        self.job_tree_view = DrillJobTreeView(RundexSettings.COLUMNS[self.technique])
        self.center.addWidget(self.job_tree_view)
        self.table_signals = JobTreeViewSignalAdapter(self.job_tree_view, self)
        self.table_signals.cellTextChanged.connect(self.data_changed)
        self.table_signals.rowInserted.connect(self.row_inserted)
        self.table_signals.appendAndEditAtRowBelowRequested.connect(self.append_and_edit_at_row_below_requested)
        self.table_signals.editAtRowAboveRequested.connect(self.edit_at_row_above_requested)
        self.table_signals.removeRowsRequested.connect(self.remove_rows_requested)
        self.table_signals.copyRowsRequested.connect(self.copy_rows_requested)
        self.table_signals.pasteRowsRequested.connect(self.paste_rows_requested)

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

    def remove_rows_requested(self, rows):
        rows = [item.rowRelativeToParent() for item in rows]
        self.call_settings_listeners(lambda listener: listener.on_rows_removed(rows))

    def copy_rows_requested(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["Drill", "Copy rows button"], False)
        self.call_settings_listeners(lambda listener: listener.on_copy_rows_requested())

    def erase_rows(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["Drill", "Erase rows button"], False)
        self.call_settings_listeners(lambda listener: listener.on_erase_rows())

    def cut_rows(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["Drill", "Cut rows button"], False)
        self.call_settings_listeners(lambda listener: listener.on_cut_rows())

    def paste_rows_requested(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["Drill", "Paste rows button"], False)
        self.call_settings_listeners(lambda listener: listener.on_paste_rows_requested())

    def load_rundex(self):
        fname = QFileDialog.getOpenFileName(self, 'Load rundex', '~')

    def choose_instrument(self, instrument):
        if instrument in ['D11', 'D16', 'D22', 'D33', 'D17', 'FIGARO']:
            config['default.instrument'] = instrument
            self.instrument = instrument
            self.technique = RundexSettings.get_technique(self.instrument)
            self.setup_center()
        else:
            logger.error('Instrument {0} is not supported yet.'.format(instrument))

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories(self).exec_()
