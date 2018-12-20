from mantidqtpython import MantidQt


# Inside the parent view
def setup(self):
    self.table = MantidQt.MantidWidgets.Batch.JobTreeView(
        ["Column 1", "Column 2"], cell(""), self)

    self.table_signals = \
        MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table, self)

    self.table.appendChildRowOf(row([]), [cell("Value for Column A"), cell("Value for Column B")])
    self.change_colour_to_red(location=row([0]), column_index=1)


def change_colour_to_red(self, location, column_index):
    cell = self.table.cellAt(location, column_index)
    cell.setBackgroundColor("#FF0000")
    self.table.setCellAt(location, column_index, cell)


def cell(text):
    return MantidQt.MantidWidgets.Batch.Cell(text)


def row(path):
    return MantidQt.MantidWidgets.Batch.RowLocation(path)
