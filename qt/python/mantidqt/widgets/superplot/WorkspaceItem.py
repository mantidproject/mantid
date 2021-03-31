# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QTreeWidgetItem


class WorkspaceItem(QTreeWidgetItem):

    """
    Name of the workspace represented by this item.
    """
    _workspaceName = None

    def __init__(self, treeWidget, name):
        super().__init__(treeWidget)
        self._workspaceName = name

    def getWorkspaceName(self):
        """
        Get the name of the workspace.

        Returns:
            str: name of the workspace
        """
        return self._workspaceName
