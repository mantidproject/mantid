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

    """
    List of plotted workspace, spectrum index pairs.
    """
    _plottedData = None

    def __init__(self):
        self._workspaces = list()
        self._plottedData = list()

    def addWorkspace(self, name):
        """
        Add a workspace to the list.

        Args:
            name (str): name of the workspace
        """
        if name not in self._workspaces:
            self._workspaces.append(name)

    def delWorkspace(self, name):
        """
        Remove a workspace from the list.

        Args:
            name (str): name of the workspace
        """
        if name in self._workspaces:
            self._workspaces.remove(name)

    def getWorkspaces(self):
        """
        Get the list of workspace names.

        Returns:
            list(str): list of workspace names
        """
        return [name for name in self._workspaces]

    def toggleData(self, workspace, spectrum):
        """
        Add or remove a workspace, spectrum index pair from the plotted data.

        Args:
            workspaces (str): name of the workspace
            spectrum (int): spectrum index
        """
        if (workspace, spectrum) in self._plottedData:
            self._plottedData.remove((workspace, spectrum))
        else:
            self._plottedData.append((workspace, spectrum))

    def getPlottedData(self):
        """
        Get the plotted data.

        Returns:
            list(tuple(str, int)): list of workspace, spectrum index pairs
        """
        return [data for data in self._plottedData]
