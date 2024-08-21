# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from PyQt5 import QtGui
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


class Predicate(MantidQt.MantidWidgets.Batch.RowPredicate):
    def __init__(self, meetsCriteria):
        super(MantidQt.MantidWidgets.Batch.RowPredicate, self).__init__()
        self.meetsCriteria = meetsCriteria

    def rowMeetsCriteria(self, location):
        return bool(self.meetsCriteria(location))


def make_regex_filter(table, text, col=0):
    try:
        regex = re.compile(text)
        return Predicate(lambda location: regex.match(table.cellAt(location, col).contentText()))
    except re.error:
        return Predicate(lambda location: True)


class DataProcessorGui(QtGui.QMainWindow, Ui_BatchWidgetWindow):
    def __init__(self):
        super(QtGui.QMainWindow, self).__init__()
        self.setupUi(self)
        self.clipboard = None

    def on_remove_runs_request(self, runs_to_remove):
        self.table.removeRows(runs_to_remove)

    def on_cell_updated(self, row, col, old_content, cell_content):
        cell = self.table.cellAt(row, col)
        if cell_content == "Invalid":
            cell.setIconFilePath(":/invalid.png")
            cell.setBorderColor("darkRed")
            self.table.setCellAt(row, col, cell)
        else:
            cell.setIconFilePath("")
            cell.setBorderColor("darkGrey")
            self.table.setCellAt(row, col, cell)

        print("Updated text at row {} col {} from {} to {}.".format(row.path(), col, old_content, cell_content))

    def on_row_inserted(self, rowLoc):
        print("Row inserted at {}".format(rowLoc.path()))

        if rowLoc.depth() == 1:
            cells = self.table.cellsAt(rowLoc)
            for i, cell in enumerate(cells):
                if i > 0:
                    cells[i] = self.table.deadCell()

            self.table.setCellsAt(rowLoc, cells)

    def on_copy_runs_request(self):
        self.clipboard = self.table.selectedSubtrees()
        self.table.clearSelection()
        if self.clipboard is None:
            print("Bad selection for copy.")

    def on_paste_rows_request(self):
        replacement_roots = self.table.selectedSubtreeRoots()
        if replacement_roots is not None and self.clipboard is not None:
            if replacement_roots:
                self.table.replaceRows(replacement_roots, self.clipboard)
            else:
                self.table.appendSubtreesAt(row([]), self.clipboard)
        else:
            print("Bad selection for paste")

    def on_filter_reset(self):
        self.filterBox.setText("")

    def options_hint_strategy(self):
        return MantidQt.MantidWidgets.AlgorithmHintStrategy(
            "ReflectometryReductionOneAuto",
            [
                "ThetaIn",
                "ThetaOut",
                "InputWorkspace",
                "OutputWorkspace",
                "OutputWorkspaceBinned",
                "OutputWorkspaceWavelength",
                "FirstTransmissionRun",
                "SecondTransmissionRun",
                "MomentumTransferMin",
                "MomentumTransferMax",
                "MomentumTransferStep",
                "ScaleFactor",
            ],
        )

    def setup_layout(self):
        self.table = MantidQt.MantidWidgets.Batch.JobTreeView(
            ["Run(s)", "Angle", "Transmission Run(s)", "Q min", "Q max", "dQ/Q", "Scale", "Options"], cell(""), self
        )
        self.table.setHintsForColumn(7, self.options_hint_strategy())

        self.table_signals = MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table)

        self.table_signals.removeRowsRequested.connect(self.on_remove_runs_request)
        self.table_signals.copyRowsRequested.connect(self.on_copy_runs_request)
        self.table_signals.pasteRowsRequested.connect(self.on_paste_rows_request)
        self.table_signals.cellTextChanged.connect(self.on_cell_updated)
        self.table_signals.rowInserted.connect(self.on_row_inserted)
        self.table_signals.filterReset.connect(self.on_filter_reset)

        self.table.appendChildRowOf(row([]), [cell("A")])
        self.table.appendChildRowOf(row([]), [cell("B")])
        self.table.appendChildRowOf(row([0]), row_from_text("C", "C", "C", "C", "C", "C", "C", "C"))

        self.filterBox = QtGui.QLineEdit(self)
        self.filterBox.textEdited.connect(lambda value: self.table.filterRowsBy(make_regex_filter(self.table, value)))
        self.layoutBase.addWidget(self.filterBox)

        # Add the widget to this interface
        self.layoutBase.addWidget(self.table)
        return True
