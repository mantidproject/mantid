# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QTreeWidgetItem, QToolButton
from qtpy.QtCore import QObject, Signal


class WorkspaceItemSignals(QObject):

    """
    Thrown when the delete button is pressed.
    """
    delClicked = Signal(str)

    def __init__(self):
        super().__init__()


class WorkspaceItem(QTreeWidgetItem):

    """
    Name of the workspace represented by this item.
    """
    _workspaceName = None

    """
    Delete button.
    """
    _delButton = None

    """
    WorkspaceItem signals.
    """
    signals = None

    def __init__(self, treeWidget, name):
        super().__init__(treeWidget)
        self._workspaceName = name
        self._delButton = QToolButton()
        self._delButton.setMinimumSize(20, 20)
        self._delButton.setMaximumSize(20, 20)
        self._delButton.setText("-")
        self._delButton.setToolTip("Remove the workspace from the list")
        self.signals = WorkspaceItemSignals()
        self._delButton.clicked.connect(
                lambda c : self.signals.delClicked.emit(self._workspaceName))
        self.treeWidget().setItemWidget(self, 1, self._delButton)
        self.treeWidget().resizeColumnToContents(1)

    def get_workspace_name(self):
        """
        Get the name of the workspace.

        Returns:
            str: name of the workspace
        """
        return self._workspaceName
