# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class DrillExportPresenter:
    """
    Export view.
    """

    _view = None

    """
    Export model.
    """
    _model = None

    def __init__(self, view, model):
        """
        Create the export presenter.

        Args:
            view (DrillExportDialog): export view
            model (DrillExportModel): export model
        """
        self._view = view
        self._model = model
        self._view.setPresenter(self)
        algorithms = self._model.getAlgorithms()
        tooltips = self._model.getAlgorithmDocs()
        self._view.setAlgorithms(algorithms, tooltips)
        states = dict()
        for a in algorithms:
            states[a] = self._model.isAlgorithmActivated(a)
        self._view.setAlgorithmCheckStates(states)
        self._view.accepted.connect(self.onAccept)
        self._view.show()

    def onAccept(self):
        """
        Triggered when the view has been validated. This method saves the
        activation state of each algorithm in the model.
        """
        states = self._view.getAlgorithmCheckStates()
        for a, s in states.items():
            if s:
                self._model.activateAlgorithm(a)
            else:
                self._model.inactivateAlgorithm(a)
