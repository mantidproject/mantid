# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .SuperplotView import SuperplotView
from .SuperplotModel import SuperplotModel

from mantid.api import mtd
from mantid.plots.axesfunctions import plot
from mantid.plots.utility import MantidAxType


class SuperplotPresenter:

    BIN_MODE_TEXT = "Bin"
    SPECTRUM_MODE_TEXT = "Spectrum"
    _view = None
    _model = None
    _canvas = None

    def __init__(self, canvas, parent=None):
        self._view = SuperplotView(self, parent)
        self._model = SuperplotModel()
        self._canvas = canvas

        #initial state
        figure = self._canvas.figure
        axes = figure.gca()
        artists = axes.get_tracked_artists()
        for artist in artists[:-1]:
            ws, specIndex = \
                    axes.get_artists_workspace_and_workspace_index(artist)
            self._model.addWorkspace(ws.name())
            self._model.setSpectrum(ws.name(), specIndex)
            self._model.addData(ws.name(), specIndex)
        ws, specIndex = \
                axes.get_artists_workspace_and_workspace_index(artists[-1])
        self._model.addWorkspace(ws.name())
        self._model.setSpectrum(ws.name(), specIndex)
        names = self._model.getWorkspaces()
        self._view.setWorkspacesList(names)
        self._view.setSelectedWorkspace(names[-1])
        self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                      self.BIN_MODE_TEXT])

        if ws.blocksize() > 1:
            self._view.setMode(self.SPECTRUM_MODE_TEXT)
        else:
            self._view.setMode(self.BIN_MODE_TEXT)
        self._changeCurrentWorkspace(ws.name())

    def getSideView(self):
        return self._view.getSideWidget()

    def getBottomView(self):
        return self._view.getBottomWidget()

    def close(self):
        self._view.close()

    def onVisibilityChanged(self, state):
        """
        Triggered when the visibility of the superplot widget changed. This
        funcion rescale the figure to be sure that the axis and labels are not
        hidden behind the dockwidgets.

        Args:
            state (bool): True if the widget is now visible
        """
        if state:
            self._canvas.figure.tight_layout()
            self._canvas.draw_idle()

    def onAddButtonClicked(self):
        """
        Triggered when the add button is pressed. This function adds the
        workspace to the selection list.
        """
        name = self._view.getSelectedWorkspace()
        self._model.addWorkspace(name)
        names = self._model.getWorkspaces()
        selectedWs = self._view.getSelectedWorkspace()
        self._view.setWorkspacesList(names)
        self._view.setSelectedWorkspace(name)
        self._updatePlot()

    def onDelButtonClicked(self):
        """
        Triggered when the del button is pressed. This function removes the
        selected workspace from the selection list.
        """
        name = self._view.getSelectedWorkspaceFromList()
        if name is None:
            return
        self._model.delWorkspace(name)
        names = self._model.getWorkspaces()
        self._view.setWorkspacesList(names)
        self._view.setSelectedWorkspace(names[-1])
        self._updatePlot()

    def _changeCurrentWorkspace(self, name):
        """
        Change the current workspace.

        Args:
            name (str): name of the workspace
        """
        currentWs = mtd[name]
        currentSpectrumIndex = self._model.getSpectrum(name)
        mode = self._view.getMode()
        if mode == self.SPECTRUM_MODE_TEXT:
            maximum = currentWs.getNumberHistograms()
        else:
            maximum = currentWs.blocksize()
        self._view.setSpectrumSliderMax(maximum - 1)
        self._view.setSpectrumSliderPosition(currentSpectrumIndex)
        self._view.setSpectrumSpinBoxMax(maximum - 1)
        self._view.setSpectrumSpinBoxValue(currentSpectrumIndex)

    def _updateHoldButton(self, wsName, spIndex):
        """
        Update the hold button state.

        Args:
            wsIndex (int): current workspace index
            spindex (int): current spectrum index
        """
        plottedData = self._model.getPlottedData()
        if (wsName, spIndex) in plottedData:
            self._view.checkHoldButton(True)
        else:
            self._view.checkHoldButton(False)

    def _updatePlot(self):
        """
        Update the plot. This function overplots the memorized data with the
        currently selected workspace and spectrum index. It keeps a memory of
        the last plot and removes it if is not part of the memorised data.
        """
        selection = self._view.getSelection()
        currentSpectrumIndex = self._view.getSpectrumSliderPosition()
        plottedData = self._model.getPlottedData()
        mode = self._view.getMode()

        figure = self._canvas.figure
        axes = figure.get_axes()
        axes = axes[0]
        axes.clear()
        for wsName, sp in plottedData:
            if self._model.isSpectrumMode():
                axes.plot(mtd[wsName], wkspIndex=sp)
            else:
                axes.plot(mtd[wsName], wkspIndex=sp, axis=MantidAxType.BIN)

        for wsName, spectra in selection.items():
            if not spectra:
                spectra.append(currentSpectrumIndex)
            for spectrum in spectra:
                if (wsName, spectrum) not in plottedData:
                    if mode == self.SPECTRUM_MODE_TEXT:
                        axes.plot(mtd[wsName], wkspIndex=spectrum)
                    else:
                        axes.plot(mtd[wsName], wkspIndex=spectrum,
                                  axis=MantidAxType.BIN)

        figure.tight_layout()
        axes.relim()
        axes.legend()
        self._canvas.draw_idle()

    def onWorkspaceSelectionChanged(self):
        """
        Triggered when the selected workspace (in the workspace list) changed.

        Args:
            index (int): index of the selected workspace
        """
        name = self._view.getSelectedWorkspaceFromList()
        if name:
            self._changeCurrentWorkspace(name)
            self._updateHoldButton(name, self._view.getSpectrumSliderPosition())
            self._updatePlot()

    def onSpectrumSliderMoved(self, position):
        """
        Triggered when the spectrum slider moved.

        Args:
            position (int): slider position
        """
        self._view.setSpectrumSpinBoxValue(position)
        currentWsName = self._view.getSelectedWorkspaceFromList()
        self._model.setSpectrum(currentWsName, position)
        self._updateHoldButton(currentWsName, position)
        self._updatePlot()

    def onSpectrumSpinBoxChanged(self, value):
        """
        Triggered when the spectrum spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.setSpectrumSliderPosition(value)
        currentWsName = self._view.getSelectedWorkspaceFromList()
        self._model.setSpectrum(currentWsName, value)
        self._updateHoldButton(currentWsName, value)
        self._updatePlot()

    def onHoldButtonToggled(self, state):
        """
        Add or delete the currently selected workspace, spectrum pair from the
        plotted data.

        Args:
            state (bool): status of the two state button (not used)
        """
        wsNames = self._view.getSelectedWorkspacesFromList()
        spectrumIndex = self._view.getSpectrumSliderPosition()
        mode = self._view.getMode()
        if state:
            for wsName in wsNames:
                spectraList = self._view.getSpectraList(wsName)
                spectraList.append(spectrumIndex)
                self._view.setSpectraList(wsName, spectraList)
                self._model.addData(wsName, spectrumIndex)
            if mode == self.SPECTRUM_MODE_TEXT:
                self._model.setSpectrumMode()
                self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT])
            else:
                self._model.setBinMode()
                self._view.setAvailableModes([self.BIN_MODE_TEXT])
        else:
            for wsName in wsNames:
                spectraList = self._view.getSpectraList(wsName)
                if spectrumIndex in spectraList:
                    spectraList.remove(spectrumIndex)
                    self._view.setSpectraList(wsName, spectraList)
                self._model.removeData(wsName, spectrumIndex)
            if not self._model.isBinMode() and not self._model.isSpectrumMode():
                self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                              self.BIN_MODE_TEXT])
                self._view.setMode(mode)

    def onModeChanged(self, mode):
        """
        Triggered when the selected mode changed in the view.

        Args:
            mode (str): new mode
        """
        currentWsName = self._view.getSelectedWorkspaceFromList()
        currentWs = mtd[currentWsName]
        if mode == self.SPECTRUM_MODE_TEXT:
            maximum = currentWs.getNumberHistograms()
        else:
            maximum = currentWs.blocksize()
        self._view.setSpectrumSliderMax(maximum - 1)
        self._view.setSpectrumSliderPosition(0)
        self._view.setSpectrumSpinBoxMax(maximum - 1)
        self._view.setSpectrumSpinBoxValue(0)
        self._updatePlot()
