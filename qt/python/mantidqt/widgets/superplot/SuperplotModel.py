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

    """
    Plot mode (bins or spectra).
    """
    _plotMode = None

    def __init__(self):
        self._workspaces = list()
        self._spectra = dict()
        self._plottedData = list()
        self._plotMode = None

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

    def setBinMode(self):
        """
        Set the plot mode to 'bins'.
        """
        self._plotMode = "bin"

    def setSpectrumMode(self):
        """
        Set the plot mode to 'spectra'.
        """
        self._plotMode = "spectrum"

    def isBinMode(self):
        """
        Return true if the plot mode is 'bins'.

        Returns:
            bool: True if 'bins' mode
        """
        return self._plotMode == "bin"

    def isSpectrumMode(self):
        """
        Return true if the plot mode is 'spectra'.

        Returns:
            bool: True if 'spectra' mode
        """
        return self._plotMode == "spectrum"

    def addData(self, workspace, spectrum):
        """
        Add a workspace, spectrum index pair from the plotted data.

        Args:
            workspaces (str): name of the workspace
            spectrum (int): index of the spectrum
        """
        self._plottedData.append((workspace, spectrum))

    def removeData(self, workspace, spectrum):
        """
        Remove a workspace, spectrum index pair from the plotted data.

        Args:
            workspaces (str): name of the workspace
            spectrum (int): index of the spectrum
        """
        self._plottedData.remove((workspace, spectrum))
        if not self._plottedData:
            self._plotMode = None

    def getPlottedData(self):
        """
        Get the plotted data.

        Returns:
            list(tuple(str, int)): list of workspace, spectrum index pairs
        """
        return [data for data in self._plottedData]
