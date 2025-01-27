# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtCore import QObject, Signal

from mantid.api import mtd, WorkspaceGroup, MatrixWorkspace
from mantid.kernel import logger

from .ads_observer import SuperplotAdsObserver


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
    sig_workspace_deleted = Signal(str)

    """
    Emitted when a workspace is renamed (ADS observation).
    Args:
        str: old name
        str: new name
    """
    sig_workspace_renamed = Signal(str, str)

    """
    Emitted when a workspace is replaced (ADS observation).
    Args:
        str: name of the workspace
    """
    sig_workspace_replaced = Signal(str)

    """
    List of managed workspace names.
    """
    _workspaces = None

    """
    List of plotted workspace, spectrum index pairs.
    """
    _plotted_data = None

    """
    Plot mode (bins or spectra).
    """
    _plot_mode = None

    """
    Colors of workspaces.
    """
    _ws_colors = None

    """
    Whether or not the plotted spectra are normalised.
    """
    _normalised = False

    def __init__(self):
        super().__init__()
        self._workspaces = list()
        self._plotted_data = list()
        self._ws_colors = dict()
        self._ads_observer = SuperplotAdsObserver()
        self._ads_observer.signals.sig_ws_deleted.connect(self.on_workspace_deleted)
        self._ads_observer.signals.sig_ws_renamed.connect(self.on_workspace_renamed)
        self._ads_observer.signals.sig_ws_replaced.connect(self.on_workspace_replaced)

    def __del__(self):
        del self._ads_observer

    def add_workspace(self, name):
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
            if not isinstance(mtd[name], MatrixWorkspace):
                logger.warning('Type of workspace "{}" is not supported by Superplot'.format(name))
                continue
            if name not in self._workspaces:
                self._workspaces.append(name)

    def del_workspace(self, name):
        """
        Remove a workspace from the list.

        Args:
            name (str): name of the workspace
        """
        if name in self._workspaces:
            self._workspaces.remove(name)
            self._plotted_data = [(n, i) for (n, i) in self._plotted_data if n != name]
        if not self._plotted_data:
            self._plot_mode = None

        if name in self._ws_colors:
            del self._ws_colors[name]

    def get_workspaces(self):
        """
        Get the list of workspace names.

        Returns:
            list(str): list of workspace names
        """
        return self._workspaces.copy()

    def normalise(self, normalise: bool):
        """
        Set the spectra normalisation state.

        Args:
            normalise (bool): True if spectra have to be normalised
        """
        self._normalised = normalise

    def is_normalised(self) -> bool:
        """
        Get the spectra normalisation state.

        Returns:
            bool: True if spectra have to be normalised
        """
        return self._normalised

    def set_workspace_color(self, ws_name, color):
        """
        Set the color of a workspace.

        Args:
            ws_name (str): workspace name
            color (str): color (directly coming (and usable) from (by)
                         matplotlib)
        """
        self._ws_colors[ws_name] = color

    def get_workspace_color(self, ws_name):
        """
        Get the color of a workspace.

        Args:
            ws_name (str): workspace name

        Returns:
            (str): color or None
        """
        if ws_name in self._ws_colors:
            return self._ws_colors[ws_name]
        return None

    def set_bin_mode(self):
        """
        Set the plot mode to 'bins'.
        """
        self._plot_mode = self.BIN_MODE

    def set_spectrum_mode(self):
        """
        Set the plot mode to 'spectrum'.
        """
        self._plot_mode = self.SPECTRUM_MODE

    def is_bin_mode(self):
        """
        Return true if the plot mode is 'bins'.

        Returns:
            bool: True if 'bins' mode
        """
        return self._plot_mode == self.BIN_MODE

    def is_spectrum_mode(self):
        """
        Return true if the plot mode is 'spectra'.

        Returns:
            bool: True if 'spectra' mode
        """
        return self._plot_mode == self.SPECTRUM_MODE

    def add_data(self, workspace, spectrum):
        """
        Add a workspace, spectrum index pair from the plotted data.

        Args:
            workspaces (str): name of the workspace
            spectrum (int): index of the spectrum
        """
        if (workspace, spectrum) not in self._plotted_data:
            self._plotted_data.append((workspace, spectrum))

    def remove_data(self, workspace, spectrum):
        """
        Remove a workspace, spectrum index pair from the plotted data.

        Args:
            workspaces (str): name of the workspace
            spectrum (int): index of the spectrum
        """
        if (workspace, spectrum) in self._plotted_data:
            self._plotted_data.remove((workspace, spectrum))
        if not self._plotted_data:
            self._plot_mode = None

    def get_plotted_data(self):
        """
        Get the plotted data.

        Returns:
            list(tuple(str, int)): list of workspace, spectrum index pairs
        """
        return self._plotted_data.copy()

    def on_workspace_deleted(self, ws_name):
        """
        Triggered when the ADS reports a workspace deletion. This method deletes
        any reference to the corresponding workspace.

        Args:
            ws_name (str): name of the deleted workspace
        """
        if ws_name not in self._workspaces:
            return
        self._workspaces.remove(ws_name)
        self._plotted_data = [(ws, sp) for (ws, sp) in self._plotted_data if ws != ws_name]
        if not self._plotted_data:
            self._plot_mode = None
        self.sig_workspace_deleted.emit(ws_name)

    def on_workspace_renamed(self, old_name, new_name):
        """
        Triggered when the ADS reports a workspace renaming. This method rename
        any reference to this workspace.

        Args:
            old_name (str): old name of the workspace
            new_name (str): new name of the workspace
        """
        if old_name not in self._workspaces:
            return
        i = self._workspaces.index(old_name)
        self._workspaces[i] = new_name
        for i in range(len(self._plotted_data)):
            if self._plotted_data[i][0] == old_name:
                self._plotted_data[i] = (new_name, self._plotted_data[i][1])
        self.sig_workspace_renamed.emit(old_name, new_name)

    def on_workspace_replaced(self, ws_name):
        """
        Triggered when the ADS reports a workspace replacement.

        Args:
            ws_name (str): name of the workspace
        """
        if ws_name not in self._workspaces:
            return
        self.sig_workspace_replaced.emit(ws_name)
