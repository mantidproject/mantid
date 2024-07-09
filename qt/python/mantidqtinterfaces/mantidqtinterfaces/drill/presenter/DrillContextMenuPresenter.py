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
        allColumns = self._view.columns
        hiddenColumns = self._view.table.getHiddenColumns()
        groups = list()
        sampleGroupNames = self._samplesModel.getSampleGroups().keys()
        for name in sampleGroupNames:
            if name is not None and name not in groups:
                groups.append(name)
        self._menu.setColumns(allColumns, hiddenColumns)
        self._menu.setGroups(groups)

        # connect signals
        self._menu.toogleColumnVisibility.connect(self.onToggleColumnVisibility)
        self._menu.groupSelectedRows.connect(self.onGroupSelectedRows)
        self._menu.ungroupSelectedRows.connect(self.onUngroupSelectedRows)
        self._menu.setMasterRow.connect(self.onSetMasterRow)
        self._menu.addToGroup.connect(self.onAddToGroup)

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
        self._view.groupSelectedRows.emit()

    def onUngroupSelectedRows(self):
        """
        Triggered when the user wants to ungroup the selected rows.
        """
        self._view.ungroupSelectedRows.emit()

    def onSetMasterRow(self):
        """
        Triggered when the user wants to set the selected row as the master row
        of its group.
        """
        self._view.setMasterRow.emit()

    def onAddToGroup(self, group):
        """
        Triggered when the user wants to add the selected rows to an existing
        group.

        Args:
            group (str): name of the group
        """
        rows = self._view.table.getRowsFromSelectedCells()
        self._samplesModel.addToGroup(rows, group)
