# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package.

from qtpy.QtCore import Qt, QSortFilterProxyModel
from mantidqt.widgets.workspacedisplay.table.view import TableWorkspaceDisplayView


class GroupTableWorkspaceDisplayView(TableWorkspaceDisplayView):
    """Specialization of a table view to display peaks
    Designed specifically to be used by PeaksViewerView
    """

    def __init__(self, *args, **kwargs):
        TableWorkspaceDisplayView.__init__(self, *args, **kwargs)
        self.source_model = TableWorkspaceDisplayView.model(self)
        self.proxy_model = QSortFilterProxyModel()
        self.proxy_model.setSourceModel(self.table_model)
        self.setModel(self.proxy_model)

    def model(self):
        return self.source_model

    def sortBySelectedColumn(self, selected_column, sort_ascending):
        self.sortByColumn(selected_column, Qt.AscendingOrder if sort_ascending else Qt.DescendingOrder)
