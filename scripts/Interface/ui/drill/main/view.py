# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.widgets import (jobtreeview, manageuserdirectories, instrumentselector)
from qtpy.QtWidgets import (QMainWindow, QWidget, QFileDialog, QSizePolicy)
from qtpy.QtGui import QIcon
from qtpy import QtCore
from .ui_main import Ui_MainWindow
from ..base import (sheet_view, tab_view)
from mantidqt import icons


class DrillView(QMainWindow, Ui_MainWindow):

    sheet_view = None
    tab_view = None

    def __init__(self, instrument):
        QMainWindow.__init__(self)
        Ui_MainWindow.__init__(self)
        self.setupUi(self)
        self.setup_header()
        self.setup_center(instrument)
        self.setup_footer(instrument)

    def setup_header(self):
        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.headerLeft.addWidget(self.instrumentselector, 0, QtCore.Qt.AlignLeft)
        self.loadRundex.clicked.connect(self.load_rundex)
        self.actionLoadRundex.setShortcut('Ctrl+O')
        self.actionLoadRundex.setStatusTip('Load rundex')
        self.actionLoadRundex.triggered.connect(self.load_rundex)
        self.load.setIcon(icons.get_icon("mdi.file-import"))
        self.load.clicked.connect(self.load_rundex)
        self.paste.setIcon(icons.get_icon("mdi.content-paste"))
        self.copy.setIcon(icons.get_icon("mdi.content-copy"))
        self.cut.setIcon(icons.get_icon("mdi.content-cut"))
        self.erase.setIcon(icons.get_icon("mdi.eraser"))
        self.deleterow.setIcon(icons.get_icon("mdi.table-row-remove"))
        self.addrow.setIcon(icons.get_icon("mdi.table-row-plus-after"))
        self.save.setIcon(icons.get_icon("mdi.file-export"))
        self.process.setIcon(icons.get_icon("mdi.eraser"))

    def setup_center(self, instrument):
        self.sheet_view = sheet_view.SheetView()
        self.center.addWidget(self.sheet_view)

    def setup_footer(self, instrument):
        self.tab_view = tab_view.TabView()
        size_policy = QSizePolicy(QSizePolicy.Minimum, QSizePolicy.Maximum)
        self.tab_view.setSizePolicy(size_policy)
        self.footer.addWidget(self.tab_view)

    def load_rundex(self):
        fname = QFileDialog.getOpenFileName(self, 'Load rundex', '~')

    def choose_instrument(self):
        pass

    def update_user_directories(self):
        pass