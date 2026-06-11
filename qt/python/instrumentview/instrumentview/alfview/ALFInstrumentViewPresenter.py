# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import override
from instrumentview.alfview.ALFInstrumentViewView import ALFInstrumentViewView
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.ComponentSelectionUtils import subtrees_of_component_indices

from mantid.simpleapi import CreateSampleWorkspace, AnalysisDataService, Rebin
from qtpy.QtCore import QObject, QMetaObject, Q_ARG


class ALFInstrumentViewPresenter(FullInstrumentViewPresenter):
    """Minimal presenter used by the C++ ALF python bridge.

    This keeps the import and construction path lightweight so the C++ side can
    always acquire a Qt widget from the `view` attribute.
    """

    def __init__(self, view=None):
        _placeholder_ws = CreateSampleWorkspace(InstrumentName="ALF", StoreInADS=False, OutputWorkspace="test_alfview")
        self.init_view_and_model(_placeholder_ws)

    def update_view(self, ws_name: str):
        ws = AnalysisDataService.retrieve(ws_name)
        if ws is None:
            return
        self.init_view_and_model(ws)

    def init_view_and_model(self, ws):
        super().__init__(ALFInstrumentViewView(), FullInstrumentViewModel(ws))
        self._view._select_bank_tube.toggle()
        self._view._render_mode_combo_box.setCurrentText(self._view._RENDER_MODE_SHAPES_FAST)
        # self._view.set_default_projection(ProjectionType.SIDE_BY_SIDE)

    def selected_detector_ids(self):
        return []

    def notify_cpp_callback(self, callback_name: str):
        relay = self._view.findChild(QObject, "ALFPythonCallbackRelay")
        if relay is not None:
            QMetaObject.invokeMethod(relay, b"notify", Q_ARG(str, callback_name))

    def selected_detector_indices_by_tube(self):
        return subtrees_of_component_indices(
            self._model._component_idxs[self._model._detector_is_picked], self._model._workspace.componentInfo()
        )

    def update_picked_detectors_on_view(self) -> None:
        super().update_picked_detectors_on_view()
        self.notify_cpp_callback("notify_whole_tube_selected")

    @override
    def _update_line_plot_ws_and_draw(self, unit: str) -> None:
        # Avoid plotting since as it gives some errors
        pass

    def rebin_button_clicked(self, params: str) -> None:
        # Rewrites the active workspace in the model, a miracle if it works
        Rebin(InputWorkspace=self._model._workspace, Params=params, OutputWorkspace=self._model._workspace.name())
