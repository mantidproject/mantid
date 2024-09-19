# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""Finds the beam centre."""

import json

from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress, IEventWorkspace
from mantid.kernel import Direction, StringListValidator
from sans.algorithm_detail.CreateSANSAdjustmentWorkspaces import CreateSANSAdjustmentWorkspaces
from sans.algorithm_detail.convert_to_q import convert_workspace
from sans.algorithm_detail.crop_helper import get_component_name
from sans.algorithm_detail.mask_sans_workspace import mask_workspace
from sans.algorithm_detail.move_sans_instrument_component import move_component, MoveTypes
from sans.algorithm_detail.scale_sans_workspace import scale_workspace
from sans.algorithm_detail.slice_sans_event import slice_sans_event
from sans.algorithm_detail.xml_shapes import quadrant_xml
from SANS.sans.common.constants import EMPTY_NAME
from SANS.sans.common.enums import DetectorType, DataType, MaskingQuadrant, RebinType
from SANS.sans.common.general_functions import create_child_algorithm, append_to_sans_file_tag
from sans.state.AllStates import AllStates
from sans.state.Serializer import Serializer


class SANSBeamCentreFinderCore(DataProcessorAlgorithm):
    def category(self):
        return "SANS\\BeamCentre"

    def summary(self):
        return "Finds the beam centre."

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty("SANSState", "", doc="A JSON string which fulfills the SANSState contract.")

        # WORKSPACES
        # Scatter Workspaces
        self.declareProperty(
            MatrixWorkspaceProperty("ScatterWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The scatter workspace. This workspace does not contain monitors.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("ScatterMonitorWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The scatter monitor workspace. This workspace only condtains monitors.",
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

        self.declareProperty("CompatibilityMode", False, direction=Direction.Input)

        self.declareProperty("Centre1", 0.0, direction=Direction.Input)
        self.declareProperty("Centre2", 0.0, direction=Direction.Input)

        self.declareProperty("RMax", 0.26, direction=Direction.Input)
        self.declareProperty("RMin", 0.06, direction=Direction.Input)

        # ----------
        # OUTPUT
        # ----------
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceLeft", "Left", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The left output workspace.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceTop", "Top", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The top output workspace.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceRight", "Right", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The right output workspace.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspaceBottom", "Bottom", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="The bottom output workspace.",
        )

    def PyExec(self):
        # --------
        # Clone the input workspaces
        # --------
        # Get the input
        state = self._get_state()
        # --------
        # Change cloned state
        # --------
        # Remove phi Masking
        state.mask.phi_min = 90.0
        state.mask.phi_max = -90.0
        state.mask.use_mask_phi_mirror = True

        component_as_string = self.getProperty("Component").value

        # Set test centre
        state.move.detectors[component_as_string].sample_centre_pos1 = self.getProperty("Centre1").value
        state.move.detectors[component_as_string].sample_centre_pos2 = self.getProperty("Centre2").value

        progress = self._get_progress()

        # --------------------------------------------------------------------------------------------------------------
        # 1. Crop workspace by detector name
        #    This will create a reduced copy of the original workspace with only those spectra which are relevant
        #    for this particular reduction.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Cropping ...")
        scatter_data = self._get_cropped_workspace(component_as_string)

        # --------------------------------------------------------------------------------------------
        # 2. Perform dark run subtraction
        #    This will subtract a dark background from the scatter workspace. Note that dark background subtraction
        #    will also affect the transmission calculation later on.
        # --------------------------------------------------------------------------------------------------------------

        # --------------------------------------------------------------------------------------------------------------
        # 3. Create event slice
        #    If we are dealing with an event workspace as input, this will cut out a time-based (user-defined) slice.
        #    In case of a histogram workspace, nothing happens.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Event slicing ...")
        data_type_as_string = self.getProperty("DataType").value
        monitor_scatter_date = self._get_monitor_workspace()
        scatter_data, monitor_scatter_date, slice_event_factor = self._slice(state, scatter_data, monitor_scatter_date, data_type_as_string)

        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN
        # IMPORTANT: This section of the code should only be temporary. It allows us to convert to histogram
        # early on and hence compare the new reduction results with the output of the new reduction chain.
        # Once the new reduction chain is established, we should remove the compatibility feature.
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        compatibility = state.compatibility
        is_event_workspace = isinstance(scatter_data, IEventWorkspace)
        if compatibility.use_compatibility_mode and is_event_workspace:
            # We convert the workspace here to a histogram workspace, since we cannot otherwise
            # compare the results between the old and the new reduction workspace in a meaningful manner.
            # The old one is histogram and the new one is event.
            # Rebin to monitor workspace
            if compatibility.time_rebin_string:
                rebin_name = "Rebin"
                rebin_option = {
                    "InputWorkspace": scatter_data,
                    "Params": compatibility.time_rebin_string,
                    "OutputWorkspace": EMPTY_NAME,
                    "PreserveEvents": False,
                }
                rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                rebin_alg.execute()
                scatter_data = rebin_alg.getProperty("OutputWorkspace").value
            else:
                rebin_name = "RebinToWorkspace"
                rebin_option = {
                    "WorkspaceToRebin": scatter_data,
                    "WorkspaceToMatch": monitor_scatter_date,
                    "OutputWorkspace": EMPTY_NAME,
                    "PreserveEvents": False,
                }
                rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                rebin_alg.execute()
                scatter_data = rebin_alg.getProperty("OutputWorkspace").value
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY END
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # ------------------------------------------------------------
        # 4. Move the workspace into the correct position
        #    The detectors in the workspaces are set such that the beam centre is at (0,0). The position is
        #    a user-specified value which can be obtained with the help of the beam centre finder.
        # ------------------------------------------------------------
        scatter_data = self._move(state=state, workspace=scatter_data, component=component_as_string)
        monitor_scatter_date = self._move(state=state, workspace=monitor_scatter_date, component=component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 5. Apply masking (pixel masking and time masking)
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Masking ...")
        scatter_data = self._mask(state=state, workspace=scatter_data, component=component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 6. Convert to Wavelength
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Converting to wavelength ...")
        scatter_data = self._convert_to_wavelength(state=state, workspace=scatter_data)

        # --------------------------------------------------------------------------------------------------------------
        # 7. Multiply by volume and absolute scale
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Multiplying by volume and absolute scale ...")
        scatter_data = self._scale(state=state, workspace=scatter_data)

        # --------------------------------------------------------------------------------------------------------------
        # 8. Create adjustment workspaces, those are
        #     1. pixel-based adjustments
        #     2. wavelength-based adjustments
        #     3. pixel-and-wavelength-based adjustments
        # Note that steps 4 to 7 could run in parallel if we don't use wide angle correction. If we do then the
        # creation of the adjustment workspaces requires the sample workspace itself and we have to run it sequentially.
        # We could consider to have a serial and a parallel strategy here, depending on the wide angle correction
        # settings. On the other hand it is not clear that this would be an advantage with the GIL.
        # --------------------------------------------------------------------------------------------------------------
        data_type_as_string = self.getProperty("DataType").value
        progress.report("Creating adjustment workspaces ...")
        wavelength_adjustment_workspace, pixel_adjustment_workspace, wavelength_and_pixel_adjustment_workspace = self._adjustment(
            state, scatter_data, monitor_scatter_date, component_as_string, data_type_as_string
        )

        # ------------------------------------------------------------
        # 9. Convert event workspaces to histogram workspaces
        # ------------------------------------------------------------
        progress.report("Converting to histogram mode ...")
        scatter_data = self._convert_to_histogram(scatter_data)

        # ------------------------------------------------------------
        # 10. Split workspace into 4 quadrant workspaces
        # ------------------------------------------------------------
        quadrants = [MaskingQuadrant.LEFT, MaskingQuadrant.RIGHT, MaskingQuadrant.TOP, MaskingQuadrant.BOTTOM]
        quadrant_scatter_reduced = {}
        centre = [0, 0, 0]
        r_min = self.getProperty("RMin").value
        r_max = self.getProperty("RMax").value
        for quadrant in quadrants:
            xml_string = quadrant_xml(centre, r_min, r_max, quadrant)
            quadrant_scatter_data = self._get_cloned_workspace(scatter_data)
            self._mask_quadrants(quadrant_scatter_data, xml_string)

            quadrant_scatter_data, sum_of_counts, sum_of_norms = self._convert_to_q(
                state=state,
                workspace=quadrant_scatter_data,
                wavelength_adjustment_workspace=wavelength_adjustment_workspace,
                pixel_adjustment_workspace=pixel_adjustment_workspace,
                wavelength_and_pixel_adjustment_workspace=wavelength_and_pixel_adjustment_workspace,
            )

            quadrant_scatter_reduced.update({quadrant: quadrant_scatter_data})

        # # ------------------------------------------------------------
        # # Populate the output
        # # ------------------------------------------------------------
        self.setProperty("OutputWorkspaceLeft", quadrant_scatter_reduced[MaskingQuadrant.LEFT])
        self.setProperty("OutputWorkspaceRight", quadrant_scatter_reduced[MaskingQuadrant.RIGHT])
        self.setProperty("OutputWorkspaceTop", quadrant_scatter_reduced[MaskingQuadrant.TOP])
        self.setProperty("OutputWorkspaceBottom", quadrant_scatter_reduced[MaskingQuadrant.BOTTOM])

    def _mask_quadrants(self, workspace, shape):
        mask_name = "MaskDetectorsInShape"
        mask_options = {"Workspace": workspace, "ShapeXML": shape}
        mask_alg = create_child_algorithm(self, mask_name, **mask_options)
        mask_alg.execute()

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
            state_slice=state.slice, input_ws=workspace, input_ws_monitor=monitor_workspace, data_type_str=data_type_as_string
        )

        workspace = returned["OutputWorkspace"]
        monitor_workspace = returned["OutputWorkspaceMonitor"]
        slice_event_factor = returned["SliceEventFactor"]
        return workspace, monitor_workspace, slice_event_factor

    def _move(self, state: AllStates, workspace, component, is_transmission=False):
        # First we set the workspace to zero, since it might have been moved around by the user in the ADS
        # Second we use the initial move to bring the workspace into the correct position
        move_component(state=state, component_name="", move_type=MoveTypes.RESET_POSITION, workspace=workspace)
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

    def _convert_to_wavelength(self, state, workspace):
        wavelength_state = state.wavelength
        wavelength_range = wavelength_state.wavelength_interval.wavelength_full_range

        wavelength_name = "SANSConvertToWavelengthAndRebin"
        wavelength_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "WavelengthPairs": json.dumps([(wavelength_range[0], wavelength_range[1])]),
            "WavelengthStep": wavelength_state.wavelength_interval.wavelength_step,
            "WavelengthStepType": wavelength_state.wavelength_step_type_lin_log.value,
            # Non monitor/transmission data does not support interpolating rebin
            "RebinMode": RebinType.REBIN.value,
        }

        wavelength_alg = create_child_algorithm(self, wavelength_name, **wavelength_options)
        wavelength_alg.execute()
        grouped_ws = wavelength_alg.getProperty("OutputWorkspace").value
        return grouped_ws.getItem(0)

    def _scale(self, state, workspace):
        instrument = state.data.instrument
        output_ws = scale_workspace(workspace=workspace, instrument=instrument, state_scale=state.scale)

        return output_ws

    def _adjustment(self, state, workspace, monitor_workspace, component_as_string, data_type):
        transmission_workspace = self._get_transmission_workspace()
        direct_workspace = self._get_direct_workspace()

        if transmission_workspace:
            transmission_workspace = self._move(
                state=state, workspace=transmission_workspace, component=component_as_string, is_transmission=True
            )

        if direct_workspace:
            direct_workspace = self._move(state=state, workspace=direct_workspace, component=component_as_string, is_transmission=True)

        alg = CreateSANSAdjustmentWorkspaces(state=state, data_type=data_type, component=component_as_string)
        wav_range = state.wavelength.wavelength_interval.wavelength_full_range
        returned_dict = alg.create_sans_adjustment_workspaces(
            direct_ws=direct_workspace,
            monitor_ws=monitor_workspace,
            sample_data=workspace,
            wav_range=wav_range,
            transmission_ws=transmission_workspace,
        )
        wavelength_adjustment = returned_dict["wavelength_adj"]
        pixel_adjustment = returned_dict["pixel_adj"]
        wavelength_and_pixel_adjustment = returned_dict["wavelength_pixel_adj"]

        return wavelength_adjustment, pixel_adjustment, wavelength_and_pixel_adjustment

    def _convert_to_histogram(self, workspace):
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

        return workspace

    def _convert_to_q(
        self, state, workspace, wavelength_adjustment_workspace, pixel_adjustment_workspace, wavelength_and_pixel_adjustment_workspace
    ):
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
        output_dict = convert_workspace(
            workspace=workspace,
            state_convert_to_q=state.convert_to_q,
            wavelength_adj_workspace=wavelength_adjustment_workspace,
            pixel_adj_workspace=pixel_adjustment_workspace,
            wavelength_and_pixel_adj_workspace=wavelength_and_pixel_adjustment_workspace,
            output_summed_parts=True,
        )

        output_workspace = output_dict["output"]
        sum_of_counts = output_dict["counts_summed"]
        sum_of_norms = output_dict["norm_summed"]

        return output_workspace, sum_of_counts, sum_of_norms

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            state = self._get_state()
            state.validate()
        except ValueError as err:
            errors.update({"SANSSingleReduction": str(err)})
        return errors

    def _get_state(self):
        state_json = self.getProperty("SANSState").value
        state = Serializer.from_json(state_json)

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

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSBeamCentreFinderCore)
