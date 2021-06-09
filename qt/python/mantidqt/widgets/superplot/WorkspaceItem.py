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
    sig_del_clicked = Signal(str)

    def __init__(self):
        super().__init__()


class WorkspaceItem(QTreeWidgetItem):

    """
    Name of the workspace represented by this item.
    """
    _workspace_name = None

    """
    Delete button.
    """
    _del_button = None

    """
    WorkspaceItem signals.
    """
    signals = None

    def __init__(self, treeWidget, name):
        super().__init__(treeWidget)
        self._workspace_name = name
        self._del_button = QToolButton()
        self._del_button.setMinimumSize(20, 20)
        self._del_button.setMaximumSize(20, 20)
        self._del_button.setText("-")
        self._del_button.setToolTip("Remove the workspace from the list")
        self.signals = WorkspaceItemSignals()
        self._del_button.clicked.connect(
                lambda c : self.signals.sig_del_clicked.emit(
                    self._workspace_name))
        self.treeWidget().setItemWidget(self, 1, self._del_button)
        self.treeWidget().resizeColumnToContents(1)

    def get_workspace_name(self):
        """
        Get the name of the workspace.

        Returns:
            str: name of the workspace
        """
        return self._workspace_name
