# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" Finds the beam centre for SANS"""

from __future__ import (absolute_import, division, print_function)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress, IEventWorkspace)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_child_algorithm, append_to_sans_file_tag
from sans.state.state_base import create_deserialized_sans_state_from_property_manager
from sans.common.enums import (DetectorType)


class SANSBeamCentreFinderMassMethod(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\BeamCentreFinder'

    def summary(self):
        return 'Finds the position of the beam centre'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        # Workspace which is to be cropped
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty(MatrixWorkspaceProperty("SampleScatterWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample scatter data')

        self.declareProperty(MatrixWorkspaceProperty('SampleScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The scatter monitor workspace. This workspace only condtains monitors.')

        self.declareProperty("Centre1", 0.0, direction=Direction.InOut)
        self.declareProperty("Centre2", 0.0, direction=Direction.InOut)

        self.declareProperty("RMin", 0.06, direction=Direction.Input)

        self.declareProperty('Tolerance', 0.0001251, direction=Direction.Input)

        self.declareProperty('Iterations', 10, direction=Direction.Input)

        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB),
                             validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

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
        if state.mask.phi_min:
            state.mask.phi_min = 0.0
        if state.mask.phi_max:
            state.mask.phi_max = 0.0

        component = self.getProperty("Component").value
        component_as_string = DetectorType.to_string(component)

        # Set test centre
        state.move.detectors[component_as_string].sample_centre_pos1 = self.getProperty(
            "Centre1").value
        state.move.detectors[component_as_string].sample_centre_pos2 = self.getProperty(
            "Centre2").value

        state_serialized = state.property_manager

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
        monitor_scatter_date = self._get_monitor_workspace()
        scatter_data, monitor_scatter_date, slice_event_factor = self._slice(state_serialized, scatter_data,
                                                                             monitor_scatter_date,
                                                                             'Sample')

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
                rebin_option = {"InputWorkspace": scatter_data,
                                "Params": compatibility.time_rebin_string,
                                "OutputWorkspace": EMPTY_NAME,
                                "PreserveEvents": False}
                rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                rebin_alg.execute()
                scatter_data = rebin_alg.getProperty("OutputWorkspace").value
            else:
                rebin_name = "RebinToWorkspace"
                rebin_option = {"WorkspaceToRebin": scatter_data,
                                "WorkspaceToMatch": monitor_scatter_date,
                                "OutputWorkspace": EMPTY_NAME,
                                "PreserveEvents": False}
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
        scatter_data = self._move(state_serialized, scatter_data, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 5. Apply masking (pixel masking and time masking)
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Masking ...")
        scatter_data = self._mask(state_serialized, scatter_data, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 6. Convert to Wavelength
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Converting to wavelength ...")
        scatter_data = self._convert_to_wavelength(state_serialized, scatter_data)

        centre1 = self.getProperty("Centre1").value
        centre2 = self.getProperty("Centre2").value
        r_min = self.getProperty("RMin").value
        tolerance = self.getProperty("Tolerance").value
        output_table = self._run_center_of_mass_position(scatter_data, centre1, centre2, r_min, tolerance)

        centre1_out = output_table[0]
        centre2_out = output_table[1]

        self.setProperty("Centre1", centre1_out + centre1)
        self.setProperty("Centre2", centre2_out + centre2)

    def _run_center_of_mass_position(self, scatter_workspace, centre1, centre2, r_min, tolerance):
        algorithm_name = "FindCenterOfMassPosition"
        alg_options = {"InputWorkspace": scatter_workspace,
                       "CenterX": centre1,
                       "CenterY": centre2,
                       "BeamRadius": r_min,
                       "Tolerance": tolerance,
                       "DirectBeam": False}
        alg = create_child_algorithm(self, algorithm_name, **alg_options)
        alg.execute()

        return alg.getProperty("CenterOfMass").value

    def _get_cropped_workspace(self, component):
        scatter_workspace = self.getProperty("SampleScatterWorkspace").value
        crop_name = "SANSCrop"
        crop_options = {"InputWorkspace": scatter_workspace,
                        "OutputWorkspace": EMPTY_NAME,
                        "Component": component}
        crop_alg = create_child_algorithm(self, crop_name, **crop_options)
        crop_alg.execute()
        return crop_alg.getProperty("OutputWorkspace").value

    def _slice(self, state_serialized, workspace, monitor_workspace, data_type_as_string):
        slice_name = "SANSSliceEvent"
        slice_options = {"SANSState": state_serialized,
                         "InputWorkspace": workspace,
                         "InputWorkspaceMonitor": monitor_workspace,
                         "OutputWorkspace": EMPTY_NAME,
                         "OutputWorkspaceMonitor": "dummy2",
                         "DataType": data_type_as_string}
        slice_alg = create_child_algorithm(self, slice_name, **slice_options)
        slice_alg.execute()

        workspace = slice_alg.getProperty("OutputWorkspace").value
        monitor_workspace = slice_alg.getProperty("OutputWorkspaceMonitor").value
        slice_event_factor = slice_alg.getProperty("SliceEventFactor").value
        return workspace, monitor_workspace, slice_event_factor

    def _move(self, state_serialized, workspace, component, is_transmission=False):
        # First we set the workspace to zero, since it might have been moved around by the user in the ADS
        # Second we use the initial move to bring the workspace into the correct position
        move_name = "SANSMove"
        move_options = {"SANSState": state_serialized,
                        "Workspace": workspace,
                        "MoveType": "SetToZero",
                        "Component": ""}
        move_alg = create_child_algorithm(self, move_name, **move_options)
        move_alg.execute()
        workspace = move_alg.getProperty("Workspace").value

        # Do the initial move
        move_alg.setProperty("MoveType", "InitialMove")
        move_alg.setProperty("Component", component)
        move_alg.setProperty("Workspace", workspace)
        move_alg.setProperty("IsTransmissionWorkspace", is_transmission)
        move_alg.execute()
        return move_alg.getProperty("Workspace").value

    def _mask(self, state_serialized, workspace, component):
        mask_name = "SANSMaskWorkspace"
        mask_options = {"SANSState": state_serialized,
                        "Workspace": workspace,
                        "Component": component}
        mask_alg = create_child_algorithm(self, mask_name, **mask_options)
        mask_alg.execute()
        return mask_alg.getProperty("Workspace").value

    def _convert_to_wavelength(self, state_serialized, workspace):
        wavelength_name = "SANSConvertToWavelength"
        wavelength_options = {"SANSState": state_serialized,
                              "InputWorkspace": workspace}
        wavelength_alg = create_child_algorithm(self, wavelength_name, **wavelength_options)
        wavelength_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        wavelength_alg.setProperty("OutputWorkspace", workspace)
        wavelength_alg.execute()
        return wavelength_alg.getProperty("OutputWorkspace").value

    def _scale(self, state_serialized, workspace):
        scale_name = "SANSScale"
        scale_options = {"SANSState": state_serialized,
                         "InputWorkspace": workspace,
                         "OutputWorkspace": EMPTY_NAME}
        scale_alg = create_child_algorithm(self, scale_name, **scale_options)
        scale_alg.execute()
        return scale_alg.getProperty("OutputWorkspace").value

    def _convert_to_histogram(self, workspace):
        if isinstance(workspace, IEventWorkspace):
            convert_name = "RebinToWorkspace"
            convert_options = {"WorkspaceToRebin": workspace,
                               "WorkspaceToMatch": workspace,
                               "OutputWorkspace": "OutputWorkspace",
                               "PreserveEvents": False}
            convert_alg = create_child_algorithm(self, convert_name, **convert_options)
            convert_alg.execute()
            workspace = convert_alg.getProperty("OutputWorkspace").value
            append_to_sans_file_tag(workspace, "_histogram")

        return workspace

    def _get_state(self):
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        state.property_manager = state_property_manager
        return state

    def _get_monitor_workspace(self):
        monitor_workspace = self.getProperty("SampleScatterMonitorWorkspace").value
        return self._get_cloned_workspace(monitor_workspace)

    def _get_cloned_workspace(self, workspace):
        clone_name = "CloneWorkspace"
        clone_options = {"InputWorkspace": workspace,
                         "OutputWorkspace": EMPTY_NAME}
        clone_alg = create_child_algorithm(self, clone_name, **clone_options)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSBeamCentreFinderMassMethod)
