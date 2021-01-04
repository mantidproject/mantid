# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class DrillContextMenuPresenter:

    """
    Main DrILL view.
    """
    _view = None

    """
    Main DrILL model.
    """
    _samplesModel = None

    """
    DrillContextMenu view.
    """
    _menu = None

    def __init__(self, view, samplesModel, menu):
        """
        Create a DrillContextMenuPresenter.

        Args:
            view (DrillView): main DrILL view
            samplesModel (DrillModel): main DrILL model
            menu (DrillContextMenu): context menu view
        """
        self._view = view
        self._samplesModel = samplesModel
        self._menu = menu
        self._menu.setPresenter(self)

        # fill the menu
        allColumns, _ = self._samplesModel.getColumnHeaderData()
        hiddenColumns = self._view.table.getHiddenColumns()
        self._menu.setColumns(allColumns, hiddenColumns)

        # connect signals
        self._menu.toogleColumnVisibility.connect(self.onToggleColumnVisibility)
        self._menu.groupSelectedRows.connect(self.onGroupSelectedRows)
        self._menu.ungroupSelectedRows.connect(self.onUngroupSelectedRows)
        self._menu.setMasterRow.connect(self.onSetMasterRow)

        self._menu.show()

    def onToggleColumnVisibility(self, column):
        """
        Triggered when a column visibility is changes from the context menu.

        Args:
            column (str): name of the column
        """
        self._view.table.toggleColumnVisibility(column)

    def onGroupSelectedRows(self):
        """
        Triggered when the user wants to group the selected rows.
        """
        rows = self._view.table.getRowsFromSelectedCells()
        group = self._samplesModel.groupSamples(rows)
        self._view.labelRowsInGroup(
                group, rows, None, "This row belongs to the sample group {}"
                .format(group), None)

    def onUngroupSelectedRows(self):
        """
        Triggered when the user wants to ungroup the selected rows.
        """
        rows = self._view.table.getRowsFromSelectedCells()
        groups = self._samplesModel.ungroupSamples(rows)
        self._view.labelRowsInGroup(None, rows, None)
        if groups:
            for group in groups:
                rows = self._samplesModel.getSamplesGroups()[group]
                self._view.labelRowsInGroup(
                        group, rows, None, "This row belongs to the sample "
                        "group {}".format(group), None)

    def onSetMasterRow(self):
        """
        Triggered when the user wants to set the selected row as the master row
        of its group.
        """
        rows = self._view.table.getRowsFromSelectedCells()
        if len(rows) != 1:
            return
        row = rows[0]
        group = self._samplesModel.setGroupMaster(row)
        if group:
            rows = self._samplesModel.getSamplesGroups()[group]
            self._view.labelRowsInGroup(
                    group, rows, row, None, "This is the master row of the "
                    "group {}".format(group))
