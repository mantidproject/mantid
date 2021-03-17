# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .SuperplotModel import SuperplotModel

from mantid.api import mtd


class SuperplotPresenter:

    _view = None
    _model = None

    def __init__(self, view):
        self._view = view
        self._model = SuperplotModel()

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
        self._view.plotData(plottedData)

    def onWorkspaceSelectionChanged(self, index):
        """
        Triggered when the selected workspace (in the workspace list) changed.

        Args:
            index (int): index of the selected workspace
        """
        self._view.setWorkspaceSliderPosition(index)
        self._view.setWorkspaceSpinBoxValue(index)
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[index]
        currentWs = mtd[currentWsName]
        self._view.setSpectrumSliderMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(0)
        self._view.setSpectrumSpinBoxMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSpinBoxValue(0)
        self._updatePlot()

    def onWorkspaceSliderMoved(self, position):
        """
        Triggered when the workspace slider moved.

        Args:
            value (int): slider value
        """
        self._view.setWorkspaceSpinBoxValue(position)
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[position]
        currentWs = mtd[currentWsName]
        self._view.setSpectrumSliderMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(0)
        self._view.setSpectrumSpinBoxMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSpinBoxValue(0)
        self._updatePlot()

    def onWorkspaceSpinBoxChanged(self, value):
        """
        Triggered when the workspace spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.setWorkspaceSliderPosition(value)
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[value]
        currentWs = mtd[currentWsName]
        self._view.setSpectrumSliderMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(0)
        self._view.setSpectrumSpinBoxMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSpinBoxValue(0)
        self._updatePlot()

    def onSpectrumSliderMoved(self, position):
        """
        Triggered when the spectrum slider moved.

        Args:
            position (int): slider position
        """
        self._view.setSpectrumSpinBoxValue(position)
        self._updatePlot()

    def onSpectrumSpinBoxChanged(self, value):
        """
        Triggered when the spectrum spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.setSpectrumSliderPosition(value)
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
