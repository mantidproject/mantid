# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.NotebookView import NotebookView
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel

import pyvista as pv


class NotebookPresenter:
    def __init__(self, view: NotebookView, model: FullInstrumentViewModel) -> None:
        self._view = view
        self._model = model
        self._counts_label = "Integrated Counts"
        self._model.setup()
        self.setup()

    def setup(self):
        self._view.subscribe_presenter(self)
        self._view.show_axes()
        self._detector_mesh = pv.PolyData(self._model.detector_positions)
        self._detector_mesh[self._counts_label] = self._model.detector_counts
        self._view.add_detector_mesh(self._detector_mesh, is_projection=self._model.is_2d_projection, scalars=self._counts_label)
        self._view.reset_camera()
