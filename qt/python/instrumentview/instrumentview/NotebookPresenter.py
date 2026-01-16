# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.NotebookView import NotebookView
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel

from matplotlib.axes import Axes
import numpy as np
import pyvista as pv
from typing import Optional
import warnings


class NotebookPresenter:
    def __init__(self, view: NotebookView, model: FullInstrumentViewModel) -> None:
        self._view = view
        self._model = model
        self._counts_label = "Integrated Counts"
        self._visible_label = "Visible Picked"
        self._model.setup()
        self.setup()

    def setup(self) -> None:
        self._view.subscribe_presenter(self)
        self._view.show_axes()
        self._detector_mesh = pv.PolyData(self._model.detector_positions)
        self._detector_mesh[self._counts_label] = self._model.detector_counts
        self._view.add_detector_mesh(self._detector_mesh, is_projection=self._model.is_2d_projection, scalars=self._counts_label)
        self._pickable_mesh = pv.PolyData(self._model.detector_positions)
        self._pickable_mesh[self._visible_label] = self._model.picked_visibility
        self._view.add_selection_mesh(self._pickable_mesh, scalars=self._visible_label)
        self._view.reset_camera()

    def pick_detectors(self, detector_ids: list[int] | np.ndarray, sum_spectra: bool) -> Optional[Axes]:
        indices = np.where(np.isin(self._model.detector_ids, detector_ids))[0]
        if len(indices) == 0:
            warnings.warn(f"Detectors not found for IDs: {detector_ids}")
            return
        mask = np.full(self._model.detector_ids.shape, False)
        mask[indices] = True
        self._model.negate_picked_visibility(mask)
        self._pickable_mesh[self._visible_label] = self._model.picked_visibility
        self._model.extract_spectra_for_line_plot(self._model.workspace_x_unit, sum_spectra)
        return self._view._plot_spectra(self._model.line_plot_workspace, sum_spectra)
