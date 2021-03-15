# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class SuperplotModel:

    """
    List of managed workspace names.
    """
    _workspaces = None

    def __init__(self):
        self._workspaces = list()

    def addWorkspace(self, name):
        """
        Add a workspace to the list.

        Args:
            name (str): name of the workspace
        """
        if name not in self._workspaces:
            self._workspaces.append(name)

    def getWorkspaces(self):
        """
        Get the list of workspace names.

        Returns:
            list(str): list of workspace names
        """
        return [name for name in self._workspaces]
