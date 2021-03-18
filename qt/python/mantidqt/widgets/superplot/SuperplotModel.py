# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import mtd, WorkspaceGroup


class SuperplotModel:

    """
    List of managed workspace names.
    """
    _workspaces = None

    """
    Correspondance between workspace and current spectra.
    """
    _spectra = None

    """
    List of plotted workspace, spectrum index pairs.
    """
    _plottedData = None

    def __init__(self):
        self._workspaces = list()
        self._spectra = dict()
        self._plottedData = list()

    def addWorkspace(self, name):
        """
        Add a workspace to the list. If it is a workspace group, all the members
        are added.

        Args:
            name (str): name of the workspace
        """
        if name in self._workspaces:
            return
        if isinstance(mtd[name], WorkspaceGroup):
            names = mtd[name].getNames()
            for name in names:
                self._workspaces.append(name)
                self._spectra[name] = 0
        else:
            self._workspaces.append(name)
            self._spectra[name] = 0

    def delWorkspace(self, name):
        """
        Remove a workspace from the list.

        Args:
            name (str): name of the workspace
        """
        if name in self._workspaces:
            self._workspaces.remove(name)
            del self._spectra[name]
            self._plottedData = [(n, i) for (n, i) in self._plottedData
                                 if n != name]

    def setSpectrum(self, name, num):
        """
        Set the current spectrum of a managed workspace.

        Args:
            name (str): name of the workspace
            num (int): index of the spectrum
        """
        self._spectra[name] = num

    def getSpectrum(self, name):
        """
        Get the current spectrum of a managed workspace.

        Args:
            name (str): name of the workspace

        Returns:
            int: index of the spectrum
        """
        return self._spectra[name]

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
