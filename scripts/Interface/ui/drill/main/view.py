# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy.QtWidgets import (QMainWindow, QWidget, QFileDialog, QSizePolicy)
from qtpy.QtGui import QIcon
from qtpy.QtCore import *
from mantidqt.widgets import (manageuserdirectories, instrumentselector)
from mantidqt import icons
from mantid.simpleapi import config, logger
from .ui_main import Ui_MainWindow
from ..sans.job_tree_view import SansJobTreeView
from ..sans.tab_view import SansTabView
from ..refl.job_tree_view import ReflJobTreeView
from ..refl.tab_view import ReflTabView


class DrillView(QMainWindow, Ui_MainWindow):

    sheet_view = None
    tab_view = None

    def __init__(self):
        QMainWindow.__init__(self)
        Ui_MainWindow.__init__(self)
        self.setupUi(self)
        self.instrument = config['default.instrument']
        self.setup_header()
        self.setup_center()
        self.setup_footer()

    def setup_header(self):
        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.instrumentselector.instrumentSelectionChanged.connect(self.choose_instrument)
        self.headerLeft.addWidget(self.instrumentselector, 0, Qt.AlignLeft)
        self.loadRundex.clicked.connect(self.load_rundex)
        self.actionLoadRundex.setShortcut('Ctrl+O')
        self.actionLoadRundex.setStatusTip('Load rundex')
        self.actionLoadRundex.triggered.connect(self.load_rundex)
        self.userDirs.clicked.connect(self.show_directory_manager)
        self.load.setIcon(icons.get_icon("mdi.file-import"))
        self.load.clicked.connect(self.load_rundex)
        self.paste.setIcon(icons.get_icon("mdi.content-paste"))
        self.copy.setIcon(icons.get_icon("mdi.content-copy"))
        self.cut.setIcon(icons.get_icon("mdi.content-cut"))
        self.erase.setIcon(icons.get_icon("mdi.eraser"))
        self.deleterow.setIcon(icons.get_icon("mdi.table-row-remove"))
        self.addrow.setIcon(icons.get_icon("mdi.table-row-plus-after"))
        self.save.setIcon(icons.get_icon("mdi.file-export"))
        self.process.setIcon(icons.get_icon("mdi.launch"))

    def setup_center(self):
        if hasattr(self, 'job_tree_view'):
            if self.job_tree_view:
                self.job_tree_view.setParent(None)
                self.job_tree_view = None
        if self.instrument in ['D11', 'D22', 'D33']:
            self.job_tree_view = SansJobTreeView()
        elif self.instrument in ['D17', 'FIGARO']:
            self.job_tree_view = ReflJobTreeView()
        else:
            pass
        self.center.addWidget(self.job_tree_view)

    def setup_footer(self):
        if hasattr(self, 'tab_view'):
            if self.tab_view:
                self.tab_view.setParent(None)
                self.tab_view = None
        if self.instrument in ['D11', 'D22', 'D33']:
            self.tab_view = SansTabView()
        elif self.instrument in ['D17', 'FIGARO']:
            self.tab_view = ReflTabView()
        else:
            pass
        self.footer.addWidget(self.tab_view)

    def load_rundex(self):
        fname = QFileDialog.getOpenFileName(self, 'Load rundex', '~')

    def choose_instrument(self, instrument):
        if instrument in ['D11', 'D22', 'D33', 'D17', 'FIGARO']:
            config['default.instrument'] = instrument
            self.instrument = instrument
            self.setup_center()
            self.setup_footer()
        else:
            logger.error('Instrument {0} is not supported yet.'.format(instrument))

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories(self).exec_()