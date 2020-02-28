# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy.QtWidgets import (QMainWindow, QWidget, QFileDialog, QSizePolicy)
from qtpy.QtGui import QIcon
from qtpy.QtCore import *
from qtpy import uic
from mantidqt.widgets import (manageuserdirectories, instrumentselector)
from mantidqt.widgets.jobtreeview import *
from mantidqt import icons
from mantid.simpleapi import config, logger
from .helpers import *
from .sans_job_tree_view import SansJobTreeView
from .refl_job_tree_view import ReflJobTreeView
import os


class DrillView(QMainWindow):

    sheet_view = None
    tab_view = None

    def __init__(self):
        super(DrillView, self).__init__()
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, 'main.ui'), self)
        self.instrument = config['default.instrument']
        self.technique = getTechnique(self.instrument)
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
        self.processRows.clicked.connect(self.process_rows)
        self.processAll.setIcon(icons.get_icon("mdi.sigma"))
        self.processAll.clicked.connect(self.process_all)
        self.stop.setIcon(icons.get_icon("mdi.stop"))

    def process_all(self):
        pass

    def process_rows(self):
        selected = self.get_selected_rows()
        from mantid.simpleapi import SANSILLAutoProcess
        for row in selected:
            contents = self.get_row(RowLocation([row]))
            args = {
                'SampleRuns': contents[0],
                'OutputWorkspace': 'test'
            }
            SANSILLAutoProcess(**args)

    def get_selected_rows(self):
        row_locations = self.job_tree_view.selectedRowLocations()
        rows = [x.rowRelativeToParent() for x in row_locations]
        return rows

    def get_row(self, row_location):
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

        if self.technique == 'sans':
            self.job_tree_view = SansJobTreeView()
        elif self.technique == 'refl':
            self.job_tree_view = ReflJobTreeView()
        else:
            pass
        self.center.addWidget(self.job_tree_view)
        self.table_signals = JobTreeViewSignalAdapter(self.job_tree_view, self)

    def load_rundex(self):
        fname = QFileDialog.getOpenFileName(self, 'Load rundex', '~')

    def choose_instrument(self, instrument):
        if instrument in ['D11', 'D16', 'D22', 'D33', 'D17', 'FIGARO']:
            config['default.instrument'] = instrument
            self.instrument = instrument
            self.technique = getTechnique(self.instrument)
            self.setup_center()
        else:
            logger.error('Instrument {0} is not supported yet.'.format(instrument))

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories(self).exec_()