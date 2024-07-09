# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QMenu
from qtpy.QtCore import Signal

from mantidqt import icons


class DrillContextMenu(QMenu):
    """
    DrillContextMenuPresenter
    """

    _presenter = None

    """
    Signal sent when the visibility of a column is changed.
    """
    toogleColumnVisibility = Signal(str)

    """
    Sent to group the selected rows.
    """
    groupSelectedRows = Signal()

    """
    Sent to ungroup the selected rows.
    """
    ungroupSelectedRows = Signal()

    """
    Sent to set the selected row as the master row of its group.
    """
    setMasterRow = Signal()

    """
    Sent when the selected rows have to be added to an existing group.
    Args:
        (str): name of the group
    """
    addToGroup = Signal(str)

    def __init__(self, position, parent=None):
        """
        Create a DrillContextMenu.

        Args:
            position (QPoint): position of the context menu
            parent (QWidget): parent widget
        """
        super().__init__(parent)
        self._position = position
        self._colMenu = self.addMenu("Add/Delete column")
        action = self.addAction("Group selected rows")
        action.triggered.connect(self.groupSelectedRows.emit)
        self._groupMenu = self.addMenu("Add to a group")
        action = self.addAction("Ungroup selected rows")
        action.triggered.connect(self.ungroupSelectedRows.emit)
        action = self.addAction("Set row as master row")
        action.triggered.connect(self.setMasterRow.emit)

    def setPresenter(self, presenter):
        """
        Set the context menu presenter.

        Args:
            presenter (DrillContextMenuPresenter): context menu presenter
        """
        self._presenter = presenter

    def setColumns(self, allColumns, hiddenColumns):
        """
        Set the columns in the context menu.

        Args:
            allColumns (list(str)): list of column names
            hiddenColumns (list(str)): list of hidden column names
        """
        for column in allColumns:
            if column in hiddenColumns:
                action = self._colMenu.addAction(icons.get_icon("mdi.close"), column)
            else:
                action = self._colMenu.addAction(icons.get_icon("mdi.check"), column)
            action.triggered.connect(lambda _, c=column: self.toogleColumnVisibility.emit(c))

    def setGroups(self, groups):
        """
        Set the list of existing groups in the context menu.

        Args:
            groups (list(str)): list of group names
        """
        for group in groups:
            action = self._groupMenu.addAction(group)
            action.triggered.connect(lambda _, g=group: self.addToGroup.emit(g))

    def show(self):
        """
        Show the context menu.
        """
        self.exec(self._position)
