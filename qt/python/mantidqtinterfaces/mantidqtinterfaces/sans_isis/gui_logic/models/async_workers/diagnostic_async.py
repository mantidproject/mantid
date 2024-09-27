# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AnalysisDataService
from mantid.api import AlgorithmPropertyWithValue
from mantid.simpleapi import SumSpectra, ConvertAxesToRealSpace
from mantidqt.utils.async_qt_adaptor import IQtAsync, qt_async_task
from mantidqt.utils.asynchronous import AsyncTaskSuccess
from sans_core.algorithm_detail.batch_execution import provide_loaded_data, create_unmanaged_algorithm, add_to_group
from sans_core.algorithm_detail.crop_helper import get_component_name
from sans_core.algorithm_detail.mask_sans_workspace import mask_workspace
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import IntegralEnum, DetectorType, SANSDataType
from sans_core.common.file_information import get_instrument_paths_for_sans_file
from sans_core.common.general_functions import parse_diagnostic_settings
from sans_core.common.xml_parsing import get_named_elements_from_ipf_file
from sans_core.common.plotting import get_plotting_module

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    # Avoid circular import at runtime
    from mantidqtinterfaces.sans_isis.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter


class DiagnosticsAsync(IQtAsync):
    def __init__(self, parent_presenter: "DiagnosticsPagePresenter"):
        super().__init__()
        self._parent_presenter = parent_presenter

    def finished_cb_slot(self) -> None:
        self._parent_presenter.on_processing_finished()

    def success_cb_slot(self, result: AsyncTaskSuccess) -> None:
        self._parent_presenter.on_processing_success(output=result.output)

    @qt_async_task
    def run_integral(self, integral_ranges, mask, integral, detector, state):
        ranges = self._parse_range(integral_ranges)
        input_workspaces = self._load_workspace(state)

        is_multi_range = len(ranges) > 1

        output_workspaces = []
        for input_workspace in input_workspaces:
            input_workspace_name = input_workspace.name()
            if is_multi_range:
                AnalysisDataService.remove(input_workspace_name + "_ranges")
            input_workspace = self._crop_workspace(detector.value, input_workspace)

            if mask:
                input_workspace = self._apply_mask(state, input_workspace, detector.value)

            x_dim, y_dim = self._get_detector_size_from_sans_file(state, detector)

            output_workspace = self._integrate_ranges(
                ranges, integral, mask, detector, input_workspace_name, input_workspace, x_dim, y_dim, is_multi_range
            )
            self._plot_graph(output_workspace)

            output_workspaces.append(output_workspace)

        return output_workspaces

    def _integrate_ranges(self, ranges, integral, mask, detector, input_workspace_name, input_workspace, x_dim, y_dim, is_multi_range):
        for integral_range in ranges:
            output_workspace = self._generate_output_workspace_name(integral_range, integral, mask, detector, input_workspace_name)
            output_workspace = self._run_algorithm(input_workspace, integral_range, integral, output_workspace, x_dim, y_dim)

            if is_multi_range:
                add_to_group(output_workspace, input_workspace_name + "_ranges")

        if is_multi_range:
            return AnalysisDataService.retrieve(input_workspace_name + "_ranges")
        else:
            return output_workspace

    def _parse_range(self, range):
        if range:
            return parse_diagnostic_settings(range)
        else:
            return [[0, AlgorithmPropertyWithValue.EMPTY_INT]]

    def _load_workspace(self, state):
        workspace_to_name = {
            SANSDataType.SAMPLE_SCATTER: "SampleScatterWorkspace",
            SANSDataType.SAMPLE_TRANSMISSION: "SampleTransmissionWorkspace",
            SANSDataType.SAMPLE_DIRECT: "SampleDirectWorkspace",
            SANSDataType.CAN_SCATTER: "CanScatterWorkspace",
            SANSDataType.CAN_TRANSMISSION: "CanTransmissionWorkspace",
            SANSDataType.CAN_DIRECT: "CanDirectWorkspace",
        }

        workspace_to_monitor = {
            SANSDataType.SAMPLE_SCATTER: "SampleScatterMonitorWorkspace",
            SANSDataType.CAN_SCATTER: "CanScatterMonitorWorkspace",
        }

        workspaces, monitors = provide_loaded_data(state, False, workspace_to_name, workspace_to_monitor)

        return workspaces[SANSDataType.SAMPLE_SCATTER]

    def _crop_workspace(self, component, workspace):
        crop_name = "CropToComponent"
        component_to_crop = DetectorType(component)
        component_to_crop = get_component_name(workspace, component_to_crop)
        crop_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME, "ComponentNames": component_to_crop}

        crop_alg = create_unmanaged_algorithm(crop_name, **crop_options)
        crop_alg.execute()
        output_workspace = crop_alg.getProperty("OutputWorkspace").value

        return output_workspace

    def _apply_mask(self, state, workspace, component):
        output_ws = mask_workspace(component_as_string=component, workspace=workspace, state=state)
        return output_ws

    def _get_detector_size_from_sans_file(self, state, detector):
        instrument_file = get_instrument_paths_for_sans_file(state.data.sample_scatter)

        if detector == DetectorType.HAB:
            x_dim = get_named_elements_from_ipf_file(instrument_file[1], "high-angle-detector-num-columns", float)[
                "high-angle-detector-num-columns"
            ]
            y_dim = get_named_elements_from_ipf_file(instrument_file[1], "high-angle-detector-num-rows", float)[
                "high-angle-detector-num-rows"
            ]
        else:
            x_dim = get_named_elements_from_ipf_file(instrument_file[1], "low-angle-detector-num-columns", float)[
                "low-angle-detector-num-columns"
            ]
            y_dim = get_named_elements_from_ipf_file(instrument_file[1], "low-angle-detector-num-rows", float)[
                "low-angle-detector-num-rows"
            ]

        return x_dim, y_dim

    def _plot_graph(self, workspace):
        plotting_module = get_plotting_module()
        if hasattr(plotting_module, "plotSpectrum"):
            return plotting_module.plotSpectrum(workspace, 0)
        elif hasattr(plotting_module, "plot"):
            if not isinstance(workspace, list):
                workspace = [workspace]
            plotting_module.plot(workspace, wksp_indices=[0])

    @staticmethod
    def _generate_output_workspace_name(range, integral, mask, detector, input_workspace_name):
        integral_string = integral.value
        detector_string = detector.value

        return "Run:{}, Range:{}, Direction:{}, Detector:{}, Mask:{}".format(
            input_workspace_name, range, integral_string, detector_string, mask
        )

    def _run_algorithm(self, input_workspace, range, integral, output_workspace, x_dim, y_dim):
        hv_min = range[0]
        hv_max = range[1]

        if integral == IntegralEnum.Horizontal:
            output_workspace = ConvertAxesToRealSpace(
                InputWorkspace=input_workspace,
                OutputWorkspace=output_workspace,
                VerticalAxis="x",
                HorizontalAxis="y",
                NumberVerticalBins=int(x_dim),
                NumberHorizontalBins=int(y_dim),
            )
            output_workspace = SumSpectra(
                InputWorkspace=output_workspace, OutputWorkspace=output_workspace, StartWorkspaceIndex=hv_min, EndWorkspaceIndex=hv_max
            )
        elif integral == IntegralEnum.Vertical:
            output_workspace = ConvertAxesToRealSpace(
                InputWorkspace=input_workspace,
                OutputWorkspace=output_workspace,
                VerticalAxis="y",
                HorizontalAxis="x",
                NumberVerticalBins=int(x_dim),
                NumberHorizontalBins=int(y_dim),
            )
            output_workspace = SumSpectra(
                InputWorkspace=output_workspace, OutputWorkspace=output_workspace, StartWorkspaceIndex=hv_min, EndWorkspaceIndex=hv_max
            )
        elif integral == IntegralEnum.Time:
            output_workspace = SumSpectra(
                InputWorkspace=input_workspace, OutputWorkspace=output_workspace, StartWorkspaceIndex=hv_min, EndWorkspaceIndex=hv_max
            )

        return output_workspace
