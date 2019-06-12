# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantidqt.gui_helper import get_qapplication
from mantidqt.widgets import (jobtreeview, manageuserdirectories, instrumentselector)
from qtpy.QtWidgets import QMainWindow, QWidget
from qtpy import QtCore

from qtpy import PYQT5
if not PYQT5:
    raise RuntimeError('Drill is operational only on workbench!')

from Interface.ui.drill.main.ui_main import Ui_MainWindow

class Drill(QMainWindow, Ui_MainWindow):
    def __init__(self):
        QMainWindow.__init__(self)
        Ui_MainWindow.__init__(self)
        self.setupUi(self)

        self.table = jobtreeview.JobTreeView(
            ["Sample", "Transmission", "Absorber", "Beam", "Container", "Reference"],
             self.cell(""), self.centralwidget)

        self.table.setRootIsDecorated(False)

        row_entry = [''] * 6
        self.add_row(row_entry)

        table_signals = jobtreeview.JobTreeViewSignalAdapter(self.table, self)

        self.instrumentselector = instrumentselector.InstrumentSelector(self)

        self.headerLeft.addWidget(self.instrumentselector, 0, QtCore.Qt.AlignLeft)
        self.center.addWidget(self.table)

    def add_row(self, value):
        value = [self.cell(x) for x in value]
        self.table.appendChildRowOf(self.row([]), value)

    def row(self, path):
        return jobtreeview.RowLocation(path)

    def cell(self, text):
        background_color = 'white'
        border_thickness = 1
        border_color = "black"
        border_opacity = 255
        is_editable = True
        return jobtreeview.Cell(text, background_color, border_thickness,
                                border_color, border_opacity, is_editable)

app, within_mantid = get_qapplication()
window = Drill()
window.show()
if not within_mantid:
    sys.exit(app.exec_())
