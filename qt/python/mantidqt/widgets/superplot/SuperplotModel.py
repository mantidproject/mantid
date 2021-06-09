# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal

from mantid.api import mtd, WorkspaceGroup

from .SuperplotAdsObserver import SuperplotAdsObserver


class SuperplotModel(QObject):

    """
    Constant for bin mode.
    """
    BIN_MODE = "bin"

    """
    Constant for spectrum mode.
    """
    SPECTRUM_MODE = "spectrum"

    """
    Emitted when a workspace is deleted (ADS observation).
    Args:
        str: name of the workspace
    """
    workspaceDeleted = Signal(str)

    """
    Emitted when a workspace is renamed (ADS observation).
    Args:
        str: old name
        str: new name
    """
    workspaceRenamed = Signal(str, str)

    """
    Emitted when a workspace is replaced (ADS observation).
    Args:
        str: name of the workspace
    """
    workspaceReplaced = Signal(str)

    """
    List of managed workspace names.
    """
    _workspaces = None

    """
    List of plotted workspace, spectrum index pairs.
    """
    _plottedData = None

    """
    Plot mode (bins or spectra).
    """
    _plotMode = None

    def __init__(self):
        super().__init__()
        self._workspaces = list()
        self._plottedData = list()
        self._adsObserver = SuperplotAdsObserver()
        self._adsObserver.signals.sig_ws_deleted.connect(
                self.onWorkspaceDeleted)
        self._adsObserver.signals.sig_ws_renamed.connect(
                self.onWorkspaceRenamed)
        self._adsObserver.signals.sig_ws_replaced.connect(
                self.onWorkspaceReplaced)

    def __del__(self):
        del self._adsObserver

    def addWorkspace(self, name):
        """
        Add a workspace to the list. If it is a workspace group, all the members
        are added.

        Args:
            name (str): name of the workspace
        """
        if name not in mtd:
            return
        if isinstance(mtd[name], WorkspaceGroup):
            names = mtd[name].getNames()
        else:
            names = [name]
        for name in names:
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
            self._plottedData = [(n, i) for (n, i) in self._plottedData
                                 if n != name]
        if not self._plottedData:
            self._plotMode = None

    def getWorkspaces(self):
        """
        Get the list of workspace names.

        Returns:
            list(str): list of workspace names
        """
        return self._workspaces.copy()

    def setBinMode(self):
        """
        Set the plot mode to 'bins'.
        """
        self._plotMode = self.BIN_MODE

    def setSpectrumMode(self):
        """
        Set the plot mode to 'spectra'.
        """
        self._plotMode = self.SPECTRUM_MODE

    def isBinMode(self):
        """
        Return true if the plot mode is 'bins'.

        Returns:
            bool: True if 'bins' mode
        """
        return self._plotMode == self.BIN_MODE

    def isSpectrumMode(self):
        """
        Return true if the plot mode is 'spectra'.

        Returns:
            bool: True if 'spectra' mode
        """
        return self._plotMode == self.SPECTRUM_MODE

    def addData(self, workspace, spectrum):
        """
        Add a workspace, spectrum index pair from the plotted data.

        Args:
            workspaces (str): name of the workspace
            spectrum (int): index of the spectrum
        """
        if (workspace, spectrum) not in self._plottedData:
            self._plottedData.append((workspace, spectrum))

    def removeData(self, workspace, spectrum):
        """
        Remove a workspace, spectrum index pair from the plotted data.

        Args:
            workspaces (str): name of the workspace
            spectrum (int): index of the spectrum
        """
        if (workspace, spectrum) in self._plottedData:
            self._plottedData.remove((workspace, spectrum))
        if not self._plottedData:
            self._plotMode = None

    def getPlottedData(self):
        """
        Get the plotted data.

        Returns:
            list(tuple(str, int)): list of workspace, spectrum index pairs
        """
        return self._plottedData.copy()

    def onWorkspaceDeleted(self, wsName):
        """
        Triggered when the ADS reports a workspace deletion. This method deletes
        any reference to the corresponding workspace.

        Args:
            wsName (str): name of the deleted workspace
        """
        if wsName not in self._workspaces:
            return
        self._workspaces.remove(wsName)
        self._plottedData = [(ws, sp) for (ws, sp) in self._plottedData
                             if ws != wsName]
        if not self._plottedData:
            self._plotMode = None
        self.workspaceDeleted.emit(wsName)

    def onWorkspaceRenamed(self, oldName, newName):
        """
        Triggered when the ADS reports a workspace renaming. This method rename
        any reference to this workspace.

        Args:
            oldName (str): old name of the workspace
            newName (str): new name of the workspace
        """
        if oldName not in self._workspaces:
            return
        i = self._workspaces.index(oldName)
        self._workspaces[i] = newName
        for i in range(len(self._plottedData)):
            if self._plottedData[i][0] == oldName:
                self._plottedData[i] = (newName, self._plottedData[i][1])
        self.workspaceRenamed.emit(oldName, newName)

    def onWorkspaceReplaced(self, wsName):
        """
        Triggered when the ADS reports a workspace replacement.

        Args:
            wsName (str): name of the workspace
        """
        if wsName not in self._workspaces:
            return
        self.workspaceReplaced.emit(wsName)
