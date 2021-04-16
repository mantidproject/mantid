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

        self._model.workspaceDeleted.connect(self.onWorkspaceDeleted)
        self._model.workspaceRenamed.connect(self.onWorkspaceRenamed)
        self._model.workspaceReplaced.connect(self.onWorkspaceReplaced)

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
        self._view.setSelectedWorkspacesInList([names[-1]])
        self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                      self.BIN_MODE_TEXT])

        if ws.blocksize() > 1:
            self._view.setMode(self.SPECTRUM_MODE_TEXT)
        else:
            self._view.setMode(self.BIN_MODE_TEXT)
        self._updateSpectrumSlider([ws.name()], 0)

    def getSideView(self):
        return self._view.getSideWidget()

    def getBottomView(self):
        return self._view.getBottomWidget()

    def close(self):
        self._view.close()
        del self._model

    def onResize(self):
        """
        Triggered when one of the dockwidgets is resized.
        """
        self._canvas.figure.tight_layout()
        self._canvas.draw_idle()

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
        selection = self._view.getSelectedWorkspacesFromList()
        addedWorkspace = self._view.getSelectedWorkspace()
        self._model.addWorkspace(addedWorkspace)
        names = self._model.getWorkspaces()
        plottedData = self._model.getPlottedData()
        self._view.setWorkspacesList(names)
        for name in names:
            spectra = list()
            for data in plottedData:
                if data[0] == name:
                    spectra.append(data[1])
            self._view.setSpectraList(name, spectra)
        self._view.setSelectedWorkspacesInList(selection)
        self._updatePlot()

    def onDelButtonClicked(self, wsName=None):
        """
        Triggered when the del button is pressed. This function removes the
        selected workspace from the selection list.
        """
        if wsName is None:
            selectedWorkspaces = self._view.getSelectedWorkspacesFromList()
        else:
            selectedWorkspaces = [wsName]
        for selectedWorkspace in selectedWorkspaces:
            self._model.delWorkspace(selectedWorkspace)
            self._view.removeWorkspace(selectedWorkspace)
        names = self._model.getWorkspaces()
        plottedData = self._model.getPlottedData()
        self._view.setWorkspacesList(names)
        for name in names:
            spectra = list()
            for data in plottedData:
                if data[0] == name:
                    spectra.append(data[1])
            self._view.setSpectraList(name, spectra)
        if names:
            self._view.setSelectedWorkspacesInList([names[-1]])
        self._updatePlot()

    def _updateSpectrumSlider(self, wsNames, position):
        """
        Update the spectrum slider and spinbox to match the selected workspaces.

        Args:
            wsNames (list(str)): list of workspace names
            position (int): position that the slider should take
        """
        if not wsNames:
            return
        maximum = None
        for wsName in wsNames:
            ws = mtd[wsName]
            nbHist = ws.getNumberHistograms()
            if maximum is None:
                maximum = nbHist
            elif nbHist < maximum:
                maximum = nbHist
        self._view.setSpectrumDisabled(False)
        self._view.setSpectrumSliderPosition(position)
        self._view.setSpectrumSliderMax(nbHist  - 1)
        self._view.setSpectrumSpinBoxValue(position)
        self._view.setSpectrumSpinBoxMax(maximum - 1)

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
                axisType = MantidAxType.SPECTRUM
            else:
                axisType = MantidAxType.BIN
            lines = axes.plot(mtd[wsName], wkspIndex=sp, axis=axisType)
            line = lines[0]
            self._view.modifySpectrumLabel(wsName, sp, line.get_label(),
                                           line.get_color())

        for wsName, spectra in selection.items():
            if not spectra:
                spectra.append(currentSpectrumIndex)
            for spectrum in spectra:
                if (wsName, spectrum) not in plottedData:
                    if mode == self.SPECTRUM_MODE_TEXT:
                        axisType = MantidAxType.SPECTRUM
                    else:
                        axisType = MantidAxType.BIN
                    lines = axes.plot(mtd[wsName], wkspIndex=spectrum,
                                      axis=axisType)
                    line = lines[0]
                    self._view.modifySpectrumLabel(wsName, spectrum,
                                                   line.get_label(),
                                                   line.get_color())

        if selection or plottedData:
            figure.tight_layout()
            axes.relim()
            axes.legend()
        self._canvas.draw_idle()

    def onWorkspaceSelectionChanged(self):
        """
        Triggered when the selected workspace (in the workspace tree) changed.
        """
        selection = self._view.getSelection()
        spectrumIndex = None
        for ws in selection:
            for sp in selection[ws]:
                if spectrumIndex is None:
                    spectrumIndex = sp
                if sp != spectrumIndex:
                    spectrumIndex = 0

        if spectrumIndex is None:
            self._view.checkHoldButton(False)
            spectrumIndex = 0
        else:
            self._view.checkHoldButton(True)
        self._view.setSpectrumSliderPosition(spectrumIndex)
        self._view.setSpectrumSpinBoxValue(spectrumIndex)
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

    def onDelSpectrumButtonClicked(self, wsName, index):
        """
        Triggered when the delete button of a selected spectrum has been
        pressed.

        Args:
            wsName (str): name of the corresponding workspace
            index (int): index of the corresponding spectrum
        """
        mode = self._view.getMode()
        self._model.removeData(wsName, index)
        spectraList = self._view.getSpectraList(wsName)
        spectraList.remove(index)
        self._view.setSpectraList(wsName, spectraList)
        if not self._model.isBinMode() and not self._model.isSpectrumMode():
            self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                          self.BIN_MODE_TEXT])
            self._view.setMode(mode)
        self._view.setSelectedWorkspacesInList([wsName])
        self._updatePlot()

    def onHoldButtonToggled(self, state):
        """
        Add or delete the currently selected workspace, spectrum pairs from the
        plotted data.

        Args:
            state (bool): status of the two state button
        """
        selection = self._view.getSelection()
        spectrumIndex = self._view.getSpectrumSliderPosition()
        mode = self._view.getMode()
        if state:
            for wsName in selection:
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
            for wsName in selection:
                spectraList = self._view.getSpectraList(wsName)
                for spectrum in selection[wsName]:
                    spectraList.remove(spectrum)
                    self._model.removeData(wsName, spectrum)
                self._view.setSpectraList(wsName, spectraList)
            if not self._model.isBinMode() and not self._model.isSpectrumMode():
                self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                              self.BIN_MODE_TEXT])
                self._view.setMode(mode)
        self._view.setSelectedWorkspacesInList(selection.keys())
        self._updatePlot()

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

    def onWorkspaceDeleted(self, wsName):
        """
        Triggered when the model reports a workspace deletion.

        Args:
            name (str): name of the workspace
        """
        selection = self._view.getSelectedWorkspacesFromList()
        names = self._model.getWorkspaces()
        if wsName in selection:
            if names:
                selection = names[-1]
        plottedData = self._model.getPlottedData()
        self._view.setWorkspacesList(names)
        for name in names:
            spectra = list()
            for data in plottedData:
                if data[0] == name:
                    spectra.append(data[1])
            self._view.setSpectraList(name, spectra)
        self._view.setSelectedWorkspacesInList(selection)
        self._updatePlot()

    def onWorkspaceRenamed(self, oldName, newName):
        """
        Triggered when the model reports a workspace renaming.

        Args:
            oldName (str): old name of the workspace
            newName (str): new name of the workspace
        """
        selection = self._view.getSelectedWorkspacesFromList()
        if oldName in selection:
            i = selection.index(oldName)
            selection[i] = newName
        names = self._model.getWorkspaces()
        plottedData = self._model.getPlottedData()
        self._view.setWorkspacesList(names)
        for name in names:
            spectra = list()
            for data in plottedData:
                if data[0] == name:
                    spectra.append(data[1])
            self._view.setSpectraList(name, spectra)
        self._view.setSelectedWorkspacesInList(selection)
        self._updatePlot()

    def onWorkspaceReplaced(self, wsName):
        """
        Triggered when the model reports a workapce replacement.

        Args:
            wsName (str): name of the workspace
        """
        self._updatePlot()
