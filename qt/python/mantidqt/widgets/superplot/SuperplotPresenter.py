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

    def onWorkspaceSliderMoved(self, value):
        """
        Triggered when the workspace slider moved.

        Args:
            value (int): slider value
        """
        names = self._model.getWorkspaces()
        currentWsName = names[value]
        currentWs = mtd[currentWsName]
        self._view.setSpectrumSliderMax(currentWs.getNumberHistograms() - 1)
        self._view.setSpectrumSliderPosition(0)
        self._view.plotData([(currentWsName, 0)])

    def onSpectrumSliderMoved(self, value):
        """
        Triggered when the spectrum slider position changed.

        Args:
            value (int): slider position
        """
        wsIndex = self._view.getWorkspaceSliderPosition()
        names = self._model.getWorkspaces()
        currentWsName = names[wsIndex]
        currentWs = mtd[currentWsName]
        self._view.plotData([(currentWsName, value)])

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
