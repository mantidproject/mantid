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
        self._view.setWorkspacesList(names)

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

    def _changeCurrentWorkspace(self, wsIndex):
        """
        Change the current workspace on the view.

        Args:
            index (int): workspace index
        """
        workspaceNames = self._model.getWorkspaces()
        currentWsName = workspaceNames[wsIndex]
        currentWs = mtd[currentWsName]
        self._view.setSpectrumSliderMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(0)
        plottedData = self._model.getPlottedData()
        currentSpectrumIndex = self._view.getSpectrumSliderPosition()
        if (currentWsName, currentSpectrumIndex) in plottedData:
            self._view.checkHoldButton(True)
        else:
            self._view.checkHoldButton(False)
            plottedData.append((currentWsName, currentSpectrumIndex))
        self._view.plotData(plottedData)

    def onWorkspaceSliderMoved(self, position):
        """
        Triggered when the workspace slider moved.

        Args:
            value (int): slider value
        """
        self._view.setWorkspaceSpinBoxValue(position)
        self._changeCurrentWorkspace(value)

    def onWorkspaceSpinBoxChanged(self, value):
        """
        Triggered when the workspace spinbox changed.

        Args:
            value (int): spinbox value
        """
        self._view.setWorkspaceSliderPosition(value)
        self._changeCurrentWorkspace(value)

    def onSpectrumSliderMoved(self, value):
        """
        Triggered when the spectrum slider position changed.

        Args:
            value (int): slider position
        """
        wsIndex = self._view.getWorkspaceSliderPosition()
        spectrumIndex = self._view.getSpectrumSliderPosition()
        names = self._model.getWorkspaces()
        currentWsName = names[wsIndex]
        currentWs = mtd[currentWsName]
        plottedData = self._model.getPlottedData()
        if (currentWsName, spectrumIndex) in plottedData:
            self._view.checkHoldButton(True)
        else:
            self._view.checkHoldButton(False)
            plottedData.append((currentWsName, value))
        self._view.plotData(plottedData)

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
