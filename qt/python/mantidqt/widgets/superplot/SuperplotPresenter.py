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
        self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                      self.BIN_MODE_TEXT])
        for artist in artists:
            ws, specIndex = \
                    axes.get_artists_workspace_and_workspace_index(artist)
            if ws.blocksize() > 1:
                self._model.setSpectrumMode()
                self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT])
            else:
                self._model.setBinMode()
                self._view.setAvailableModes([self.BIN_MODE_TEXT])
            wsName = ws.name()
            self._model.addWorkspace(wsName)
            self._model.addData(wsName, specIndex)

        self._updateList()
        self._updateSpectrumSlider()
        self._updatePlot()

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
        try:
            self._canvas.figure.tight_layout()
            self._canvas.draw_idle()
        except:
            pass

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
        self._updateList()
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
        self._updateList()
        if not self._model.isBinMode() and not self._model.isSpectrumMode():
            mode = self._view.getMode()
            self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                          self.BIN_MODE_TEXT])
            self._view.setMode(mode)
        self._view.setSelectedWorkspacesInList([])
        self._updateSpectrumSlider()
        self._updatePlot()

    def _updateSpectrumSlider(self, position=0):
        """
        Update the spectrum slider and spinbox to match the selected workspaces.

        Args:
            position (int): position that the slider should take (if possible).
                            default is 0
        """
        selection = self._view.getSelection()
        if not selection:
            self._view.setSpectrumSliderPosition(0)
            self._view.setSpectrumSliderMax(0)
            self._view.setSpectrumSpinBoxValue(0)
            self._view.setSpectrumSpinBoxMax(0)
            self._view.setSpectrumDisabled(True)
            return
        maximum = None
        mode = self._view.getMode()
        for wsName in selection:
            ws = mtd[wsName]
            if mode == self.SPECTRUM_MODE_TEXT:
                value = ws.getNumberHistograms()
            else:
                value = ws.blocksize()
            if maximum is None:
                maximum = value
            elif value < maximum:
                maximum = value
        if position >= maximum:
            position = 0
        self._view.setSpectrumDisabled(False)
        self._view.setSpectrumSliderMax(maximum - 1)
        self._view.setSpectrumSliderPosition(position)
        self._view.setSpectrumSpinBoxMax(maximum - 1)
        self._view.setSpectrumSpinBoxValue(position)

    def _updateHoldButton(self):
        """
        Update the hold button state based on the selection.
        """
        selection = self._view.getSelection()
        index = self._view.getSpectrumSliderPosition()
        plottedData = self._model.getPlottedData()
        for ws in selection:
            if (ws, index) not in plottedData:
                self._view.checkHoldButton(False)
                return
        self._view.checkHoldButton(True)

    def _updateList(self):
        """
        Update the workspaces/spectra list.
        """
        names = self._model.getWorkspaces()
        plottedData = self._model.getPlottedData()
        self._view.setWorkspacesList(names)
        for name in names:
            spectra = list()
            for data in plottedData:
                if data[0] == name:
                    spectra.append(data[1])
            self._view.setSpectraList(name, spectra)

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
        axes = figure.gca()
        for line in axes.get_lines():
            line.remove()
        axes.set_prop_cycle(None)
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
            if currentSpectrumIndex not in spectra:
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
                    self._updateSpectrumSlider()
                    self._updatePlot()
                    return

        if spectrumIndex is None:
            self._view.checkHoldButton(False)
            spectrumIndex = 0
        else:
            self._view.checkHoldButton(True)
        self._updateSpectrumSlider(spectrumIndex)
        self._updatePlot()

    def onSpectrumSliderMoved(self, position):
        """
        Triggered when the spectrum slider moved.

        Args:
            position (int): slider position
        """
        self._view.setSpectrumSpinBoxValue(position)
        self._updateHoldButton()
        self._updatePlot()

    def onSpectrumSpinBoxChanged(self, value):
        """
        Triggered when the spectrum spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.setSpectrumSliderPosition(value)
        self._updateHoldButton()
        self._updatePlot()

    def onDelSpectrumButtonClicked(self, wsName, index):
        """
        Triggered when the delete button of a selected spectrum has been
        pressed.

        Args:
            wsName (str): name of the corresponding workspace
            index (int): index of the corresponding spectrum
        """
        selection = self._view.getSelection()
        currentIndex = self._view.getSpectrumSliderPosition()
        mode = self._view.getMode()
        self._model.removeData(wsName, index)
        if not self._model.isBinMode() and not self._model.isSpectrumMode():
            self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                          self.BIN_MODE_TEXT])
            self._view.setMode(mode)
        if wsName in selection:
            if index in selection[wsName]:
                selection[wsName].remove(index)
                currentIndex = 0
                if not selection[wsName]:
                    del selection[wsName]
        self._updateList()
        self._view.setSelection(selection)
        self._updateSpectrumSlider(currentIndex)
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
                self._model.addData(wsName, spectrumIndex)
            if mode == self.SPECTRUM_MODE_TEXT:
                self._model.setSpectrumMode()
                self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT])
            else:
                self._model.setBinMode()
                self._view.setAvailableModes([self.BIN_MODE_TEXT])
        else:
            for wsName in selection:
                for spectrum in selection[wsName]:
                    self._model.removeData(wsName, spectrum)
                selection[wsName] = []
            if not self._model.isBinMode() and not self._model.isSpectrumMode():
                self._view.setAvailableModes([self.SPECTRUM_MODE_TEXT,
                                              self.BIN_MODE_TEXT])
                self._view.setMode(mode)
        self._updateList()
        self._view.setSelection(selection)
        self._updateSpectrumSlider(spectrumIndex)
        self._view.checkHoldButton(False)
        self._updatePlot()

    def onModeChanged(self, mode):
        """
        Triggered when the selected mode changed in the view.

        Args:
            mode (str): new mode
        """
        selection = self._view.getSelection()
        self._updateSpectrumSlider()
        self._updatePlot()

    def onWorkspaceDeleted(self, wsName):
        """
        Triggered when the model reports a workspace deletion.

        Args:
            name (str): name of the workspace
        """
        selection = self._view.getSelection()
        if wsName in selection:
            del selection[wsName]
        self._updateList()
        self._view.setSelection(selection)
        self._updatePlot()

    def onWorkspaceRenamed(self, oldName, newName):
        """
        Triggered when the model reports a workspace renaming.

        Args:
            oldName (str): old name of the workspace
            newName (str): new name of the workspace
        """
        selection = self._view.getSelection()
        if oldName in selection:
            selection[newName] = selection[oldName]
            del selection[oldName]
        self._updateList()
        self._view.setSelection(selection)
        self._updatePlot()

    def onWorkspaceReplaced(self, wsName):
        """
        Triggered when the model reports a workapce replacement.

        Args:
            wsName (str): name of the workspace
        """
        self._updatePlot()
