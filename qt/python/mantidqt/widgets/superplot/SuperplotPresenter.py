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
        self._view.setWorkspaceSliderPosition(len(names))
        self._view.setWorkspaceSpinBoxValue(len(names))
        self._view.setSelectedWorkspace(len(names))
        self._changeCurrentWorkspace(len(names))

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
        self._view.setSelectedWorkspace(names.index(selectedWs) + 1)

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
        currentWsName = workspaceNames[index - 1]
        currentWs = mtd[currentWsName]
        currentSpectrumIndex = self._model.getSpectrum(currentWsName)
        self._view.setSpectrumSliderMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(currentSpectrumIndex)
        self._view.setSpectrumSpinBoxMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSpinBoxValue(currentSpectrumIndex)

    def _updateHoldButton(self, wsIndex, spIndex):
        """
        Update the hold button state.

        Args:
            wsIndex (int): current workspace index
            spindex (int): current spectrum index
        """
        plottedData = self._model.getPlottedData()
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[wsIndex - 1]
        if (currentWsName, spIndex) in plottedData:
            self._view.checkHoldButton(True)
        else:
            self._view.checkHoldButton(False)

    def _updatePlot(self):
        """
        Update the plot. This function overplots the memorized data with the
        currently selected workspace and spectrum index. It keeps a memory of
        the last plot and removes it if is not part of the memorised data.
        """
        currentWorkspaceIndex = self._view.getWorkspaceSliderPosition()
        currentSpectrumIndex = self._view.getSpectrumSliderPosition()
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[currentWorkspaceIndex - 1]
        plottedData = self._model.getPlottedData()

        figure = self._canvas.figure
        axes = figure.get_axes()
        axes = axes[0]
        axes.clear()
        for wsName, sp in plottedData:
            axes.plot(mtd[wsName], wkspIndex=sp)
        if (currentWsName, currentSpectrumIndex) not in plottedData:
            axes.plot(mtd[currentWsName], wkspIndex=currentSpectrumIndex)

        figure.tight_layout()
        axes.relim()
        axes.legend()
        self._canvas.draw_idle()

    def onWorkspaceSelectionChanged(self, index):
        """
        Triggered when the selected workspace (in the workspace list) changed.

        Args:
            index (int): index of the selected workspace
        """
        index = index + 1
        self._view.setWorkspaceSliderPosition(index)
        self._view.setWorkspaceSpinBoxValue(index)
        self._changeCurrentWorkspace(index)
        self._updateHoldButton(index, self._view.getSpectrumSliderPosition())
        self._updatePlot()

    def onWorkspaceSliderMoved(self, position):
        """
        Triggered when the workspace slider moved.

        Args:
            value (int): slider value
        """
        self._view.setWorkspaceSpinBoxValue(position)
        self._view.setSelectedWorkspace(position)
        self._changeCurrentWorkspace(position)
        self._updateHoldButton(position, self._view.getSpectrumSliderPosition())
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
        self._updateHoldButton(value, self._view.getSpectrumSliderPosition())
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
        currentWsName = workspaceNames[currentWsIndex - 1]
        self._model.setSpectrum(currentWsName, position)
        self._updateHoldButton(currentWsIndex, position)
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
        currentWsName = workspaceNames[currentWsIndex - 1]
        self._model.setSpectrum(currentWsName, value)
        self._updateHoldButton(currentWsIndex, value)
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
        if state:
            self._model.addData(names[wsIndex - 1], spectrumIndex)
        else:
            self._model.removeData(names[wsIndex - 1], spectrumIndex)
