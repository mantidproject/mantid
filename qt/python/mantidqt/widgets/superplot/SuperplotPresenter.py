# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .SuperplotView import SuperplotView
from .SuperplotModel import SuperplotModel

from mantid.api import mtd
from mantid.plots.plotfunctions import plot, get_plot_fig


class SuperplotPresenter:

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
            ws, specNum = axes.get_artists_workspace_and_spec_num(artist)
            self._model.addWorkspace(ws.name())
            self._model.setSpectrum(ws.name(), specNum)
            self._model.toggleData(ws.name(), specNum)
        ws, specNum = axes.get_artists_workspace_and_spec_num(artists[-1])
        self._model.addWorkspace(ws.name())
        self._model.setSpectrum(ws.name(), specNum)
        names = self._model.getWorkspaces()
        self._view.setWorkspacesList(names)
        self._view.setWorkspaceSliderPosition(len(artists) - 1)
        self._view.setWorkspaceSpinBoxValue(len(artists) - 1)
        self._view.setSpectrumSliderMax(ws.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(specNum)
        self._view.setSpectrumSpinBoxMax(ws.getNumberHistograms() - 1)
        self._view.setSpectrumSpinBoxValue(specNum)
        self._view.setSelectedWorkspace(len(artists) - 1)

    def getSideView(self):
        return self._view.getSideWidget()

    def getBottomView(self):
        return self._view.getBottomWidget()

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
        self._view.setSelectedWorkspace(names.index(selectedWs))

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
        self._updatePlot()

    def _changeCurrentWorkspace(self, index):
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[index]
        currentWs = mtd[currentWsName]
        currenSpectrum = self._model.getSpectrum(currentWsName)
        self._view.setSpectrumSliderMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(currenSpectrum)
        self._view.setSpectrumSpinBoxMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSpinBoxValue(currenSpectrum)

    def _updatePlot(self):
        """
        Update the plot. This function overplots the memorized data with the
        currently selected workspace and spectrum index.
        """
        currentWorkspaceIndex = self._view.getWorkspaceSliderPosition()
        currentSpectrumIndex = self._view.getSpectrumSliderPosition()
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[currentWorkspaceIndex]
        plottedData = self._model.getPlottedData()
        if (currentWsName, currentSpectrumIndex) in plottedData:
            self._view.checkHoldButton(True)
        else:
            self._view.checkHoldButton(False)
            plottedData.append((currentWsName, currentSpectrumIndex))

        figure, _ = get_plot_fig(fig=self._canvas.figure)
        for i in plottedData:
            plot([i[0]], spectrum_nums=[i[1] + 1], overplot=True, fig=figure)

    def onWorkspaceSelectionChanged(self, index):
        """
        Triggered when the selected workspace (in the workspace list) changed.

        Args:
            index (int): index of the selected workspace
        """
        self._view.setWorkspaceSliderPosition(index)
        self._view.setWorkspaceSpinBoxValue(index)
        self._changeCurrentWorkspace(index)
        self._updatePlot()

    def onWorkspaceSliderMoved(self, position):
        """
        Triggered when the workspace slider moved.

        Args:
            value (int): slider value
        """
        self._view.setWorkspaceSpinBoxValue(position)
        self._view.setSelectedWorkspace(position)
        self._updatePlot()

    def onWorkspaceSpinBoxChanged(self, value):
        """
        Triggered when the workspace spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.setWorkspaceSliderPosition(value)
        self._view.setSelectedWorkspace(value)
        self._changeCurrentWorkspace(value)
        self._updatePlot()

    def onSpectrumSliderMoved(self, position):
        """
        Triggered when the spectrum slider moved.

        Args:
            position (int): slider position
        """
        self._view.setSpectrumSpinBoxValue(position)
        workspaceNames = self._model.getWorkspaces()
        currentWsIndex = self._view.getWorkspaceSliderPosition()
        currentWsName = workspaceNames[currentWsIndex]
        self._model.setSpectrum(currentWsName, position)
        self._updatePlot()

    def onSpectrumSpinBoxChanged(self, value):
        """
        Triggered when the spectrum spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.setSpectrumSliderPosition(value)
        workspaceNames = self._model.getWorkspaces()
        currentWsIndex = self._view.getWorkspaceSliderPosition()
        currentWsName = workspaceNames[currentWsIndex]
        self._model.setSpectrum(currentWsName, value)
        self._updatePlot()

    def onHoldButtonToggled(self, state):
        """
        Add or delete the currently selected workspace, spectrum pair from the
        plotted data.

        Args:
            state (bool): status of the two state button (not used)
        """
        wsIndex = self._view.getWorkspaceSliderPosition()
        spectrumIndex = self._view.getSpectrumSliderPosition()
        names = self._model.getWorkspaces()
        self._model.toggleData(names[wsIndex], spectrumIndex)
