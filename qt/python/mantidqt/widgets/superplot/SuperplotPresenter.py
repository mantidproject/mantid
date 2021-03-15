# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .SuperplotModel import SuperplotModel


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

    def onHoldButtonToggled(self, state):
        pass
