# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.widgets import (jobtreeview, manageuserdirectories, instrumentselector)
from qtpy.QtWidgets import (QMainWindow, QWidget)
from qtpy import QtCore
from .ui_main import Ui_MainWindow
from ..base import sheet_view


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

    def setup_center(self, instrument):
        self.sheet_view = sheet_view.SheetView()
        self.center.addWidget(self.sheet_view)

    def setup_footer(self, instrument):
        pass
