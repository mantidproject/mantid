from mantidqtpython import MantidQt


# Inside the parent view
def setup(self):
    self.table = MantidQt.MantidWidgets.Batch.JobTreeView(
        ["Column 1", "Column 2"], # The table column headings
        cell(""), # The default style and content for new 'empty' cells.
        self # The parent QObject.
    )

    self.table_signals = \
        MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table, self)
    # The signal adapter subscribes to events from the table
    # and emits signals whenever it is notified.

    self.table.appendChildRowOf(row([]), [cell("Value for Column A"), cell("Value for Column B")])


def cell(text):
    return MantidQt.MantidWidgets.Batch.Cell(text)


def row(path):
    return MantidQt.MantidWidgets.Batch.RowLocation(path)
