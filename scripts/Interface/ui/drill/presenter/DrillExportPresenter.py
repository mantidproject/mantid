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
        self._view.setAlgorithms(self._model.getAlgorithms())
        self._view.show()
