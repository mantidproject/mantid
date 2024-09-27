# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""A base class to share functionality between SANSReductionCore algorithms."""

import json
import os
from typing import Tuple, Dict

from mantid.api import (
    DataProcessorAlgorithm,
    MatrixWorkspace,
    MatrixWorkspaceProperty,
    PropertyMode,
    IEventWorkspace,
    WorkspaceGroup,
    WorkspaceGroupProperty,
)
from mantid.kernel import Direction, StringListValidator
from mantid.py36compat import dataclass
from sans_core.algorithm_detail.CreateSANSAdjustmentWorkspaces import CreateSANSAdjustmentWorkspaces
from sans_core.algorithm_detail.convert_to_q import convert_workspace
from sans_core.algorithm_detail.crop_helper import get_component_name
from sans_core.algorithm_detail.mask_sans_workspace import mask_workspace
from sans_core.algorithm_detail.move_sans_instrument_component import move_component, MoveTypes
from sans_core.algorithm_detail.scale_sans_workspace import scale_workspace
from sans_core.algorithm_detail.slice_sans_event import slice_sans_event
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import DetectorType, DataType, RebinType
from sans_core.common.general_functions import create_child_algorithm, append_to_sans_file_tag
from sans_core.state.Serializer import Serializer
from sans_core.state.StateObjects.wavelength_interval import WavRange

WsList = Dict[Tuple[float, float], MatrixWorkspace]


@dataclass
class AdjustmentStruct:
    wavelength_adjustment: MatrixWorkspace
    pixel_adjustment: MatrixWorkspace
    wavelength_and_pixel_adjustment: MatrixWorkspace
    calculated_transmission_workspace: MatrixWorkspace
    unfitted_transmission_workspace: MatrixWorkspace


@dataclass
class SumsStruct:
    counts: int
    norm: int


class SANSReductionCoreBase(DataProcessorAlgorithm):
    def _pyinit_input(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty("SANSState", "", doc="A JSON String which fulfills the SANSState contract.")

        # WORKSPACES
        # Scatter Workspaces
        self.declareProperty(
            MatrixWorkspaceProperty("ScatterWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The scatter workspace. This workspace does not contain monitors.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("ScatterMonitorWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The scatter monitor workspace. This workspace only contains monitors.",
        )

        # Transmission Workspace
        self.declareProperty(
            MatrixWorkspaceProperty("TransmissionWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The transmission workspace.",
        )

        # Direct Workspace
        self.declareProperty(
            MatrixWorkspaceProperty("DirectWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The direct workspace.",
        )

        self.setPropertyGroup("ScatterWorkspace", "Data")
        self.setPropertyGroup("ScatterMonitorWorkspace", "Data")
        self.setPropertyGroup("TransmissionWorkspace", "Data")
        self.setPropertyGroup("DirectWorkspace", "Data")

        # The component
        allowed_detectors = StringListValidator([DetectorType.LAB.value, DetectorType.HAB.value])
        self.declareProperty(
            "Component",
            DetectorType.LAB.value,
            validator=allowed_detectors,
            direction=Direction.Input,
            doc="The component of the instrument which is to be reduced.",
        )

        # The data type
        allowed_data = StringListValidator([DataType.SAMPLE.value, DataType.CAN.value])
        self.declareProperty(
            "DataType",
            DataType.SAMPLE.value,
            validator=allowed_data,
            direction=Direction.Input,
            doc="The component of the instrument which is to be reduced.",
        )

    def _pyinit_output(self):
        # ----------
        # OUTPUT
        # ----------
        self.declareProperty(WorkspaceGroupProperty("OutputWorkspaces", "", direction=Direction.Output), doc="The output workspace.")

        self.declareProperty(
            WorkspaceGroupProperty("SumOfCounts", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sum of the counts of the output workspace.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("SumOfNormFactors", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The sum of the counts of the output workspace.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("CalculatedTransmissionWorkspaces", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The calculated transmission workspace",
        )

        self.declareProperty(
            WorkspaceGroupProperty("UnfittedTransmissionWorkspaces", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The unfitted transmission workspace",
        )

    def _get_cropped_workspace(self, component):
        scatter_workspace = self.getProperty("ScatterWorkspace").value
        alg_name = "CropToComponent"

        component_to_crop = DetectorType(component)
        component_to_crop = get_component_name(scatter_workspace, component_to_crop)

        crop_options = {"InputWorkspace": scatter_workspace, "OutputWorkspace": EMPTY_NAME, "ComponentNames": component_to_crop}

        crop_alg = create_child_algorithm(self, alg_name, **crop_options)
        crop_alg.execute()

        output_workspace = crop_alg.getProperty("OutputWorkspace").value
        return output_workspace

    def _slice(self, state, workspace, monitor_workspace, data_type_as_string):
        returned = slice_sans_event(
            input_ws_monitor=monitor_workspace, state_slice=state.slice, input_ws=workspace, data_type_str=data_type_as_string
        )

        workspace = returned["OutputWorkspace"]
        monitor_workspace = returned["OutputWorkspaceMonitor"]
        slice_event_factor = returned["SliceEventFactor"]

        return workspace, monitor_workspace, slice_event_factor

    def _move(self, state, workspace, component, is_transmission=False):
        # First we set the workspace to zero, since it might have been moved around by the user in the ADS
        # Second we use the initial move to bring the workspace into the correct position
        move_component(component_name="", state=state, move_type=MoveTypes.RESET_POSITION, workspace=workspace)

        move_component(
            component_name=component,
            state=state,
            move_type=MoveTypes.INITIAL_MOVE,
            workspace=workspace,
            is_transmission_workspace=is_transmission,
        )

        return workspace

    def _mask(self, state, workspace, component):
        output_ws = mask_workspace(component_as_string=component, workspace=workspace, state=state)
        return output_ws

    def _convert_to_wavelength(self, workspace, wavelength_state) -> WsList:
        wavelength_name = "SANSConvertToWavelengthAndRebin"
        selected_ranges = wavelength_state.wavelength_interval.selected_ranges
        assert len(selected_ranges) > 0
        wavelength_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "WavelengthPairs": json.dumps(selected_ranges),
            "WavelengthStep": wavelength_state.wavelength_interval.wavelength_step,
            "WavelengthStepType": wavelength_state.wavelength_step_type_lin_log.value,
            # No option for interpolating data is available
            "RebinMode": RebinType.REBIN.value,
        }

        wavelength_alg = create_child_algorithm(self, wavelength_name, **wavelength_options)
        wavelength_alg.execute()
        grouped_ws = wavelength_alg.getProperty("OutputWorkspace").value
        assert len(grouped_ws) == len(selected_ranges)
        processed = {tuple(wav_range): ws for wav_range, ws in zip(selected_ranges, grouped_ws)}
        return processed

    def _scale(self, state, ws_list: WsList):
        instrument = state.data.instrument
        for key, ws in ws_list.items():
            output_ws = scale_workspace(instrument=instrument, state_scale=state.scale, workspace=ws)
            ws_list[key] = output_ws
        return ws_list

    def _adjustment(self, state, workspaces: WsList, monitor_workspace, component_as_string, data_type) -> Dict[WavRange, AdjustmentStruct]:
        transmission_workspace = self._get_transmission_workspace()
        direct_workspace = self._get_direct_workspace()

        if transmission_workspace:
            transmission_workspace = self._move(
                state=state, workspace=transmission_workspace, component=component_as_string, is_transmission=True
            )

        if direct_workspace:
            direct_workspace = self._move(state=state, workspace=direct_workspace, component=component_as_string, is_transmission=True)

        alg = CreateSANSAdjustmentWorkspaces(state=state, component=component_as_string, data_type=data_type)

        adjustments = {}
        for wav_range, ws in workspaces.items():
            returned_dict = alg.create_sans_adjustment_workspaces(
                direct_ws=direct_workspace,
                monitor_ws=monitor_workspace,
                sample_data=ws,
                transmission_ws=transmission_workspace,
                wav_range=wav_range,
            )
            adjustments[wav_range] = AdjustmentStruct(
                wavelength_adjustment=returned_dict["wavelength_adj"],
                pixel_adjustment=returned_dict["pixel_adj"],
                wavelength_and_pixel_adjustment=returned_dict["wavelength_pixel_adj"],
                calculated_transmission_workspace=returned_dict["calculated_trans_ws"],
                unfitted_transmission_workspace=returned_dict["unfitted_trans_ws"],
            )

        return adjustments

    def _group_workspaces(self, workspaces):
        group = WorkspaceGroup()
        for ws in workspaces.values():
            if ws:
                group.addWorkspace(ws)
        return group

    def _add_metadata(self, state, workspaces):
        for wav_range, ws in workspaces.items():
            if not ws:
                continue  # If we have something disabled but meta-data is still produced

            replace_prop = True
            run = ws.getRun()
            # The wavelength range allows the calling algorithms to get this back out, without
            # having to much around with trying to pass dicts through Properties (which would be
            # easy if the property manager accepted Workspaces2D values...)
            run.addProperty("Wavelength Range", f"{wav_range[0]}-{wav_range[1]}", replace_prop)
            if state.save.user_file:
                run.addProperty("UserFile", os.path.basename(state.save.user_file), replace_prop)
            if state.save.batch_file:
                run.addProperty("BatchFile", os.path.basename(state.save.batch_file), replace_prop)
        return workspaces  # Allow us to chain up these commands

    def _copy_bin_masks(self, workspaces, dummy_workspaces):
        for wav_range in workspaces.keys():
            mask_options = {
                "InputWorkspace": workspaces[wav_range],
                "MaskedWorkspace": dummy_workspaces[wav_range],
                "OutputWorkspace": EMPTY_NAME,
            }
            mask_alg = create_child_algorithm(self, "MaskBinsFromWorkspace", **mask_options)
            mask_alg.execute()
            workspaces[wav_range] = mask_alg.getProperty("OutputWorkspace").value

    def _convert_to_histogram(self, workspaces):
        for wav_range, workspace in workspaces.items():
            if isinstance(workspace, IEventWorkspace):
                convert_name = "RebinToWorkspace"
                convert_options = {
                    "WorkspaceToRebin": workspace,
                    "WorkspaceToMatch": workspace,
                    "OutputWorkspace": "OutputWorkspace",
                    "PreserveEvents": False,
                }
                convert_alg = create_child_algorithm(self, convert_name, **convert_options)
                convert_alg.execute()
                workspace = convert_alg.getProperty("OutputWorkspace").value
                append_to_sans_file_tag(workspace, "_histogram")
                workspaces[wav_range] = workspace

        return workspaces

    def _convert_to_q(self, state, workspaces: WsList, adjustment_dict: Dict[WavRange, AdjustmentStruct]) -> Dict[WavRange, SumsStruct]:
        """
        A conversion to momentum transfer is performed in this step.

        The conversion can be either to the modulus of Q in which case the output is a 1D workspace, or it can
        be a 2D reduction where the y axis is Qy, ie it is a numeric axis.
        @param state: a SANSState object
        @param workspace: the workspace to convert to momentum transfer.
        @param wavelength_adjustment_workspace: the wavelength adjustment workspace.
        @param pixel_adjustment_workspace: the pixel adjustment workspace.
        @param wavelength_and_pixel_adjustment_workspace: the wavelength and pixel adjustment workspace.
        @return: a reduced workspace
        """
        sums = {}
        for wav_range in workspaces.keys():
            adjustment = adjustment_dict[wav_range]

            output_dict = convert_workspace(
                workspace=workspaces[wav_range],
                state_convert_to_q=state.convert_to_q,
                wavelength_adj_workspace=adjustment.wavelength_adjustment,
                wavelength_and_pixel_adj_workspace=adjustment.wavelength_and_pixel_adjustment,
                pixel_adj_workspace=adjustment.pixel_adjustment,
                output_summed_parts=True,
            )

            sums[wav_range] = SumsStruct(counts=output_dict["counts_summed"], norm=output_dict["norm_summed"])
            workspaces[wav_range] = output_dict["output"]
        return sums

    def _get_state(self):
        json_state = self.getProperty("SANSState").value
        state = Serializer.from_json(json_state)
        return state

    def _get_transmission_workspace(self):
        transmission_workspace = self.getProperty("TransmissionWorkspace").value
        return self._get_cloned_workspace(transmission_workspace) if transmission_workspace else None

    def _get_direct_workspace(self):
        direct_workspace = self.getProperty("DirectWorkspace").value
        return self._get_cloned_workspace(direct_workspace) if direct_workspace else None

    def _get_monitor_workspace(self):
        monitor_workspace = self.getProperty("ScatterMonitorWorkspace").value
        return self._get_cloned_workspace(monitor_workspace)

    def _get_cloned_workspace(self, workspace):
        clone_name = "CloneWorkspace"
        clone_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME}
        clone_alg = create_child_algorithm(self, clone_name, **clone_options)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _check_compatibility_mode(self, workspace, monitor_workspace, compatibility):
        is_event_workspace = isinstance(workspace, IEventWorkspace)
        use_dummy_workspace = False
        dummy_mask_workspace = None
        if is_event_workspace:
            if compatibility.use_compatibility_mode:
                # We convert the workspace here to a histogram workspace, since we cannot otherwise
                # compare the results between the old and the new reduction workspace in a meaningful manner.
                # The old one is histogram and the new one is event.
                # Rebin to monitor workspace
                if compatibility.time_rebin_string:
                    rebin_name = "Rebin"
                    rebin_option = {
                        "InputWorkspace": workspace,
                        "Params": compatibility.time_rebin_string,
                        "OutputWorkspace": EMPTY_NAME,
                        "PreserveEvents": False,
                    }
                    rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                    rebin_alg.execute()
                    workspace = rebin_alg.getProperty("OutputWorkspace").value
                else:
                    rebin_name = "RebinToWorkspace"
                    rebin_option = {
                        "WorkspaceToRebin": workspace,
                        "WorkspaceToMatch": monitor_workspace,
                        "OutputWorkspace": EMPTY_NAME,
                        "PreserveEvents": False,
                    }
                    rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                    rebin_alg.execute()
                    workspace = rebin_alg.getProperty("OutputWorkspace").value
            else:
                # If not using compatibility mode, we create a histogram from the workspace, which will store
                # the bin masking.
                # Extract a single spectrum to make operations as quick as possible.
                # We only need the mask flags, not the y data.
                use_dummy_workspace = True

                # Extract only a single spectrum so dummy workspace which contains bin masks is a small as possible
                # (cheaper operations).
                # This is find because we only care about the mask flags in this workspace, not the y data.
                extract_spectrum_name = "ExtractSingleSpectrum"
                extract_spectrum_option = {"InputWorkspace": workspace, "OutputWorkspace": "dummy_mask_workspace", "WorkspaceIndex": 0}
                extract_spectrum_alg = create_child_algorithm(self, extract_spectrum_name, **extract_spectrum_option)
                extract_spectrum_alg.execute()
                dummy_mask_workspace = extract_spectrum_alg.getProperty("OutputWorkspace").value

                rebin_name = "RebinToWorkspace"
                rebin_option = {
                    "WorkspaceToRebin": dummy_mask_workspace,
                    "WorkspaceToMatch": monitor_workspace,
                    "OutputWorkspace": "dummy_mask_workspace",
                    "PreserveEvents": False,
                }
                rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                rebin_alg.execute()
                dummy_mask_workspace = rebin_alg.getProperty("OutputWorkspace").value
        return workspace, dummy_mask_workspace, use_dummy_workspace
