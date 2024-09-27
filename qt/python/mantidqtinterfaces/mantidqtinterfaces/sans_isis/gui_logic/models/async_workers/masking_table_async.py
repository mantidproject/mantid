# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from typing import TYPE_CHECKING

from mantid.api import AnalysisDataService
from mantidqt.utils.async_qt_adaptor import IQtAsync, qt_async_task
from mantidqt.utils.asynchronous import AsyncTaskSuccess, AsyncTaskFailure
from sans_core.algorithm_detail.mask_sans_workspace import mask_workspace
from sans_core.algorithm_detail.move_sans_instrument_component import move_component, MoveTypes
from sans_core.state.Serializer import Serializer
from sans_core.common.enums import DetectorType
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.general_functions import create_unmanaged_algorithm

if TYPE_CHECKING:
    from mantidqtinterfaces.sans_isis.gui_logic.presenter.masking_table_presenter import MaskingTablePresenter


class MaskingTableAsync(IQtAsync):
    def __init__(self, parent_presenter: "MaskingTablePresenter"):
        super().__init__()
        self._parent_presenter = parent_presenter

    def error_cb_slot(self, result: AsyncTaskFailure) -> None:
        self._parent_presenter.on_processing_error_masking_display(error=str(result))

    def success_cb_slot(self, result: AsyncTaskSuccess) -> None:
        self._parent_presenter.on_processing_successful_masking_display(result=result.output)

    def finished_cb_slot(self) -> None:
        self._parent_presenter.on_processing_finished_masking_display()

    @qt_async_task
    def load_and_mask_workspace(self, state, workspace_name):
        workspace_to_mask = self._load_workspace(state, workspace_name)
        return self._run_mask_workspace(state, workspace_to_mask)

    def _load_workspace(self, state, workspace_name):
        self._prepare_to_load_scatter_sample_only(state)
        self._handle_multi_period_data(state)

        serialized_state = Serializer.to_json(state)
        workspace = self._perform_load(serialized_state)
        self._perform_move(state, workspace)
        self._store_in_ads_as_hidden(workspace_name, workspace)
        return workspace

    def _run_mask_workspace(self, state, workspace_to_mask):
        mask_info = state.mask

        both_detectors = [DetectorType.LAB.value, DetectorType.HAB.value]
        lab_only = [DetectorType.LAB.value]
        detectors = both_detectors if DetectorType.HAB.value in mask_info.detectors else lab_only

        for detector in detectors:
            mask_workspace(component_as_string=detector, workspace=workspace_to_mask, state=state)

        return workspace_to_mask

    @staticmethod
    def _prepare_to_load_scatter_sample_only(state):
        # We only want to load the data for the scatter sample. Hence we set everything else to an empty string.
        # This is ok since we are changing a copy of the state which is not being used for the actual data reduction.
        state.data.sample_transmission = ""
        state.data.sample_direct = ""
        state.data.can_scatter = ""
        state.data.can_transmission = ""
        state.data.can_direct = ""

    @staticmethod
    def _handle_multi_period_data(state):
        # If the data is multi-period data, then we select only the first period.
        if state.data.sample_scatter_is_multi_period and state.data.sample_scatter_period == 0:
            state.data.sample_scatter_period = 1

    def _perform_load(self, serialized_state):
        load_algorithm = self._create_load_algorithm(serialized_state)
        load_algorithm.execute()
        return load_algorithm.getProperty("SampleScatterWorkspace").value

    def _perform_move(self, state, workspace):
        move_component(state=state, component_name="", workspace=workspace, move_type=MoveTypes.RESET_POSITION)
        move_component(component_name=None, state=state, workspace=workspace, move_type=MoveTypes.INITIAL_MOVE)

    def _create_load_algorithm(self, serialized_state):
        load_name = "SANSLoad"
        load_options = {
            "SANSState": serialized_state,
            "PublishToCache": True,
            "UseCached": True,
            "SampleScatterWorkspace": EMPTY_NAME,
            "SampleScatterMonitorWorkspace": EMPTY_NAME,
            "SampleTransmissionWorkspace": EMPTY_NAME,
            "SampleDirectWorkspace": EMPTY_NAME,
            "CanScatterWorkspace": EMPTY_NAME,
            "CanScatterMonitorWorkspace": EMPTY_NAME,
            "CanTransmissionWorkspace": EMPTY_NAME,
            "CanDirectWorkspace": EMPTY_NAME,
        }
        return create_unmanaged_algorithm(load_name, **load_options)

    @staticmethod
    def _store_in_ads_as_hidden(workspace_name, workspace):
        AnalysisDataService.addOrReplace(workspace_name, workspace)
