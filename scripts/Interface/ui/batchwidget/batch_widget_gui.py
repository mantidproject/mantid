from __future__ import (absolute_import, division, print_function)
try:
    from mantidplot import *
    canMantidPlot = True
except ImportError:
    canMantidPlot = False

from PyQt4 import QtGui
from mantidqtpython import MantidQt
from ui.batchwidget.ui_batch_widget_window import Ui_BatchWidgetWindow
import re


def row(path):
    return MantidQt.MantidWidgets.Batch.RowLocation(path)


def cell(cell_text):
    return MantidQt.MantidWidgets.Batch.Cell(cell_text)


def row_from_text(*cell_texts):
    return [cell(cell_text) for cell_text in cell_texts]


def group_row(group_title, n_columns):
    return [cell(group_title)]


class RegexPredicate(MantidQt.MantidWidgets.Batch.RowPredicate):
    def __init__(self, view, text):
        super(MantidQt.MantidWidgets.Batch.RowPredicate, self).__init__()
        self.view = view
        self.text = text

        try:
            self.re = re.compile(text)
        except re.error:
            self.re = re.compile('')

    def rowMeetsCriteria(self, location):
        return bool(self.re.match(self.view.cellAt(location, 0).contentText()))


class DataProcessorGui(QtGui.QMainWindow, Ui_BatchWidgetWindow):
    def __init__(self):
        super(QtGui.QMainWindow, self).__init__()
        self.setupUi(self)
        self.clipboard = None

    def on_remove_runs_request(self, runs_to_remove):
        self.table.removeRows(runs_to_remove)

    def on_cell_updated(self, row, col, cell_content):
        print("Updated row {} col {} with text {}".format(row.path(), col, cell_content))
        pass

    def on_row_inserted(self, rowLoc):
        print("Row inserted at {}".format(rowLoc.path()))

        def killed_cell(cell):
            cell.disableEditing()
            cell.setBorderThickness(0)
            cell.setBorderColor("transparent")
            return cell

        if rowLoc.depth() > 2:
            print( "Deleting" )
            self.table.removeRowAt(rowLoc)

        if rowLoc.depth() == 1:
            print ("Disabled Editing At {}".format(rowLoc.path()))
            cells = self.table.cellsAt(rowLoc)

            for i, cell in enumerate(cells):
                if i > 0:
                    cells[i] = killed_cell(cell)

            self.table.setCellsAt(rowLoc, cells)


    def on_copy_runs_request(self):
        self.clipboard = self.table.selectedSubtrees()
        self.table.clearSelection()
        if self.clipboard is not None:
            print(self.clipboard[0][0].cells()[0].borderColor())
        else:
            print ("Bad selection for copy.")

    def on_paste_rows_request(self):
        replacement_roots = self.table.selectedSubtreeRoots()
        if replacement_roots is not None and self.clipboard is not None:
            if replacement_roots:
                self.table.replaceRows(replacement_roots, self.clipboard)
            else:
                self.table.appendSubtreesAt(row([]), self.clipboard)
        else:
            print("Bad selection for paste")

    def setup_layout(self):
        self.table = MantidQt.MantidWidgets.Batch.JobTreeView(["Run(s)",
                                                               "Angle",
                                                               "Transmission Run(s)",
                                                               "Q min",
                                                               "Q max",
                                                               "dQ/Q",
                                                               "Scale",
                                                               "Options"], cell(""), self)
        self.table_signals = MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table)

        self.table_signals.removeRowsRequested.connect(self.on_remove_runs_request)
        self.table_signals.copyRowsRequested.connect(self.on_copy_runs_request)
        self.table_signals.pasteRowsRequested.connect(self.on_paste_rows_request)
        self.table_signals.cellChanged.connect(self.on_cell_updated)
        self.table_signals.rowInserted.connect(self.on_row_inserted)

        self.table.appendChildRowOf(row([]), group_row("A", 8))

        self.table.appendChildRowOf(row([]), group_row("B", 8))

        self.table.appendChildRowOf(row([0]), row_from_text("C",
                                                            "C",
                                                            "C",
                                                            "C",
                                                            "C",
                                                            "C",
                                                            "C",
                                                            "C"))
        self.table.enableFiltering()

        self.filterBox = QtGui.QLineEdit(self)
        self.filterBox.textChanged.connect(lambda value: self.table.filterRowsBy(RegexPredicate(self.table, value)))

        self.layoutBase.addWidget(self.filterBox)

        # Add the widget to this interface
        self.layoutBase.addWidget(self.table)
        return True
