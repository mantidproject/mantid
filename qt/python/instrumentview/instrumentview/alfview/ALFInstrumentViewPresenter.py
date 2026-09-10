# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.alfview.ALFInstrumentViewView import ALFInstrumentViewView
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.ComponentSelectionUtils import detector_component_indices_in_subtrees
from instrumentview.Globals import CurrentTab

import numpy as np
from mantid.simpleapi import CreateSampleWorkspace, Rebin
from qtpy.QtCore import QObject, QMetaObject, Q_ARG


class ALFInstrumentViewPresenter(FullInstrumentViewPresenter):
    """Minimal presenter used by the C++ ALF python bridge.

    This keeps the import and construction path lightweight so the C++ side can
    always acquire a Qt widget from the `view` attribute.
    """

    _ROI_SELECTION_KEY = "Rectangle ROI"

    def __init__(self, view=None):
        _placeholder_ws = CreateSampleWorkspace(InstrumentName="ALF", StoreInADS=False, OutputWorkspace="test_alfview")
        super().__init__(ALFInstrumentViewView(), FullInstrumentViewModel(_placeholder_ws))
        self._view._select_bank_tube.toggle()
        self._view._render_mode_combo_box.setCurrentText(self._view._RENDER_MODE_SHAPES_FAST)
        self._view._projection_combo_box.setCurrentText(ProjectionType.CYLINDRICAL_Y.value)
        self._view.check_sum_spectra_checkbox()

    def update_view(self, ws_name: str):
        self._reset_model_workspace(ws_name)
        self._update_view_main_plotter(refresh_limits=True)

    def selected_detector_ids(self):
        return []

    def notify_cpp_callback(self, callback_name: str):
        relay = self._view.findChild(QObject, "ALFPythonCallbackRelay")
        if relay is not None:
            QMetaObject.invokeMethod(relay, b"notify", Q_ARG(str, callback_name))

    def selected_detector_indices_by_tube(self):
        return detector_component_indices_in_subtrees(
            self._model._component_idxs[self._model._detector_is_picked], self._model._workspace.componentInfo()
        )

    def update_picked_detectors_on_view(self) -> None:
        super().update_picked_detectors_on_view()
        self.notify_cpp_callback("notify_whole_tube_selected")

    def on_roi_shape_changed(self) -> None:
        """Re-derive the selection from the rectangle overlaid on the projection.

        Called on the Qt thread when the rectangle is added, and again by the shape overlay
        manager each time it is dragged, resized or rotated, so that the ALF tube selection
        always follows the rectangle.
        """
        centres = self._model.transformed_detector_positions
        # Projection uses VTK, so must be done on the Qt thread before queueing the rest
        self._view.project_and_cache_detector_points(centres)
        self._callback_queue.put((self._on_roi_shape_changed, (centres,)))

    def _on_roi_shape_changed(self, centres: np.ndarray) -> None:
        mask = self._view.get_shape_mask(centres)
        if self._view.is_select_bank_tube_checked():
            mask = self._model.expand_pickable_mask_to_parent_subtrees(mask)
        # A single stored key so that moving the rectangle replaces the selection rather than adding to it
        self._model.set_detector_key(self._ROI_SELECTION_KEY, mask.tolist(), CurrentTab.Grouping)
        self._model.apply_detector_items([self._ROI_SELECTION_KEY], CurrentTab.Grouping)
        self.update_picked_detectors_on_view()

    def rebin_button_clicked(self, params: str) -> None:
        # Rewrites the active workspace in the model
        Rebin(InputWorkspace=self._model._workspace, Params=params, OutputWorkspace=self._model._workspace.name())
