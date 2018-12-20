from mantidqtpython import MantidQt
import re


class Predicate(MantidQt.MantidWidgets.Batch.RowPredicate):
    def __init__(self, meetsCriteria):
        super(MantidQt.MantidWidgets.Batch.RowPredicate, self).__init__()
        self.meetsCriteria = meetsCriteria

    def rowMeetsCriteria(self, location):
        return bool(self.meetsCriteria(location))


def make_regex_filter(table, text, col = 0):
    try:
        regex = re.compile(text)
        return Predicate(lambda location: regex.match(table.cellAt(location, col).contentText()))
    except re.error:
        return Predicate(lambda location: True)


def row(path):
    return MantidQt.MantidWidgets.Batch.RowLocation(path)


def empty_cell():
    return cell("")


def cell(cell_text):
    return MantidQt.MantidWidgets.Batch.Cell(cell_text)


def row_from_text(*cell_texts):
    return [cell(cell_text) for cell_text in cell_texts]


# Inside the parent view
def setup(self):
    self.table = MantidQt.MantidWidgets.Batch.JobTreeView(["Column 1"], empty_cell(), self)
    self.table_signals = MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table, self)

    self.table.appendChildRowOf(row([]),  [cell("DD")]) # DD
    self.table.appendChildRowOf(row([0]), [cell("DC")]) #   DC
    self.table.appendChildRowOf(row([0]), [cell("A9")]) #     A9
    self.table.appendChildRowOf(row([]),  [cell("B0")]) # B0
    self.table.appendChildRowOf(row([]),  [cell("C0")]) # C0
    self.table.appendChildRowOf(row([2]), [cell("A1")]) #   A1

    self.table.filterRowsBy(make_regex_filter(self.table, "A[0-9]+", col=0))
    # Applying this filter excludes B0 since neither itself not it's decendant's contents
    # match the regex given.
