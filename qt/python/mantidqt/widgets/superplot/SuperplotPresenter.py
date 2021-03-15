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
        pass

    def onHoldButtonToggled(self, state):
        pass
