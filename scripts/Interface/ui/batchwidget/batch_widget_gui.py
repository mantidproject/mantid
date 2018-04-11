from __future__ import (absolute_import, division, print_function)
try:
    from mantidplot import *
    canMantidPlot = True
except ImportError:
    canMantidPlot = False

from PyQt4 import QtGui
from mantidqtpython import MantidQt
from ui.batchwidget.ui_batch_widget_window import Ui_BatchWidgetWindow


def at(path):
    return MantidQt.MantidWidgets.Batch.RowLocation(path)


class DataProcessorGui(QtGui.QMainWindow, Ui_BatchWidgetWindow):

    def __init__(self):
        super(QtGui.QMainWindow, self).__init__()
        self.setupUi(self)

    def on_remove_runs_request(self, runs_to_remove):
        self.table.removeRows(runs_to_remove)

    def setup_layout(self):
        self.table = MantidQt.MantidWidgets.Batch.JobTreeView(["Colum1", "Column2"], self)
        self.table_signals = MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table)
        self.table_signals.removeRowsRequested.connect(self.on_remove_runs_request)
        self.table.appendChildRowOf(at([]))

        # Add the widget to this interface
        self.layoutBase.addWidget(self.table)
        return True
