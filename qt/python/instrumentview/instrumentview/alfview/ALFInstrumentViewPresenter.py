# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from instrumentview.alfview.ALFInstrumentViewView import ALFInstrumentViewView
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from instrumentview.Projections.ProjectionType import ProjectionType
from instrumentview.ComponentSelectionUtils import subtrees_of_component_indices

from mantid.simpleapi import CreateSampleWorkspace
from qtpy.QtCore import QObject, QMetaObject, Q_ARG


class ALFInstrumentViewPresenter(FullInstrumentViewPresenter):
    """Minimal presenter used by the C++ ALF python bridge.

    This keeps the import and construction path lightweight so the C++ side can
    always acquire a Qt widget from the `view` attribute.
    """

    def __init__(self, view=None):
        _placeholder_ws = CreateSampleWorkspace(InstrumentName="ALF", StoreInADS=False, OutputWorkspace="test_alfview")
        super().__init__(ALFInstrumentViewView(), FullInstrumentViewModel(_placeholder_ws))
        self._view.set_default_projection(ProjectionType.SIDE_BY_SIDE)

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

    def get_workspace_name(self):
        return self._model._workspace.name()
