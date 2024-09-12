# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtpython import MantidQt


def empty_cell():
    return MantidQt.MantidWidgets.Batch.Cell("")


# Inside the parent view
def setup(self):
    self.table = MantidQt.MantidWidgets.Batch.JobTreeView(["Column 1", "Column 2"], empty_cell(), self)

    self.table_signals = MantidQt.MantidWidgets.Batch.JobTreeViewSignalAdapter(self.table, self)

    self.table_signals.rowInserted.connect(self.on_row_inserted)
    # The rowInserted signal is fired every time a user inserts a row.
    # It is NOT fired if we manually insert a row.


def on_row_inserted(self, rowLocation):
    # If the depth is more than two then we can safely 'rollback' the insertion.
    if rowLocation.depth() > 2:
        self.table.removeRowAt(rowLocation)
