from __future__ import (absolute_import, division, print_function)
try:
    from mantidplot import *
    canMantidPlot = True
except ImportError:
    canMantidPlot = False

from PyQt4 import QtGui
from mantidqtpython import MantidQt
from ui.batchwidget.ui_batch_widget_window import Ui_BatchWidgetWindow


def row(path):
    return MantidQt.MantidWidgets.Batch.RowLocation(path)


class DataProcessorGui(QtGui.QMainWindow, Ui_BatchWidgetWindow):
    def __init__(self):
        super(QtGui.QMainWindow, self).__init__()
        self.setupUi(self)

    def on_remove_runs_request(self, runs_to_remove):
        self.table.removeRows(runs_to_remove)

    def on_cell_updated(self, row, col, cell_content):
        print("Updated row {} col {} with text {}".format(row.path(), col, cell_content))

    def on_row_inserted(self, rowLoc):
        print("Row inserted at {}".format(rowLoc.path()))
        self.table.removeRowAt(rowLoc)

    def setup_layout(self):
        self.table = MantidQt.MantidWidgets.Batch.JobTreeView(["Colum1", "Column2"], self)
        self.table_signals = MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table)

        self.table_signals.removeRowsRequested.connect(self.on_remove_runs_request)
        self.table_signals.cellChanged.connect(self.on_cell_updated)
        self.table_signals.rowInserted.connect(self.on_row_inserted)

        self.table.appendChildRowOf(row([]))
        self.table.appendChildRowOf(row([]))

        # Add the widget to this interface
        self.layoutBase.addWidget(self.table)
        return True
