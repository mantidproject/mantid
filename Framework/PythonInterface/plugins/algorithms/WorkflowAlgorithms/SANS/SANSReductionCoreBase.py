# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""A base class to share functionality between SANSReductionCore algorithms."""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode, IEventWorkspace)

from sans.state.state_base import create_deserialized_sans_state_from_property_manager
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import (create_child_algorithm, append_to_sans_file_tag)
from sans.common.enums import (DetectorType, DataType)


class SANSReductionCoreBase(DistributedDataProcessorAlgorithm):
    def _pyinit_input(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # WORKSPACES
        # Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('ScatterWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('ScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The scatter monitor workspace. This workspace only contains monitors.')

        # Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('TransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The transmission workspace.')

        # Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('DirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The direct workspace.')

        self.setPropertyGroup("ScatterWorkspace", 'Data')
        self.setPropertyGroup("ScatterMonitorWorkspace", 'Data')
        self.setPropertyGroup("TransmissionWorkspace", 'Data')
        self.setPropertyGroup("DirectWorkspace", 'Data')

        # The component
        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB),
                             validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        # The data type
        allowed_data = StringListValidator([DataType.to_string(DataType.Sample),
                                            DataType.to_string(DataType.Can)])
        self.declareProperty("DataType", DataType.to_string(DataType.Sample),
                             validator=allowed_data, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

    def _pyinit_output(self):
        # ----------
        # OUTPUT
        # ----------
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", '', direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(MatrixWorkspaceProperty('SumOfCounts', '', optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc='The sum of the counts of the output workspace.')

        self.declareProperty(MatrixWorkspaceProperty('SumOfNormFactors', '', optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc='The sum of the counts of the output workspace.')

        self.declareProperty(MatrixWorkspaceProperty('CalculatedTransmissionWorkspace', '', optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc='The calculated transmission workspace')

        self.declareProperty(MatrixWorkspaceProperty('UnfittedTransmissionWorkspace', '', optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc='The unfitted transmission workspace')

    def _get_cropped_workspace(self, component):
        scatter_workspace = self.getProperty("ScatterWorkspace").value
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

    def _adjustment(self, state_serialized, workspace, monitor_workspace, component_as_string, data_type):
        transmission_workspace = self._get_transmission_workspace()
        direct_workspace = self._get_direct_workspace()

        adjustment_name = "SANSCreateAdjustmentWorkspaces"
        adjustment_options = {"SANSState": state_serialized,
                              "Component": component_as_string,
                              "DataType": data_type,
                              "MonitorWorkspace": monitor_workspace,
                              "SampleData": workspace,
                              "OutputWorkspaceWavelengthAdjustment": EMPTY_NAME,
                              "OutputWorkspacePixelAdjustment": EMPTY_NAME,
                              "OutputWorkspaceWavelengthAndPixelAdjustment": EMPTY_NAME}
        if transmission_workspace:
            transmission_workspace = self._move(state_serialized, transmission_workspace, component_as_string,
                                                is_transmission=True)
            adjustment_options.update({"TransmissionWorkspace": transmission_workspace})

        if direct_workspace:
            direct_workspace = self._move(state_serialized, direct_workspace, component_as_string, is_transmission=True)
            adjustment_options.update({"DirectWorkspace": direct_workspace})

        adjustment_alg = create_child_algorithm(self, adjustment_name, **adjustment_options)
        adjustment_alg.execute()

        wavelength_adjustment = adjustment_alg.getProperty("OutputWorkspaceWavelengthAdjustment").value
        pixel_adjustment = adjustment_alg.getProperty("OutputWorkspacePixelAdjustment").value
        wavelength_and_pixel_adjustment = adjustment_alg.getProperty(
                                           "OutputWorkspaceWavelengthAndPixelAdjustment").value
        calculated_transmission_workspace = adjustment_alg.getProperty("CalculatedTransmissionWorkspace").value
        unfitted_transmission_workspace = adjustment_alg.getProperty("UnfittedTransmissionWorkspace").value
        return wavelength_adjustment, pixel_adjustment, wavelength_and_pixel_adjustment, \
            calculated_transmission_workspace, unfitted_transmission_workspace

    def _copy_bin_masks(self, workspace, dummy_workspace):
        mask_options = {"InputWorkspace": workspace,
                        "MaskedWorkspace": dummy_workspace,
                        "OutputWorkspace": EMPTY_NAME}
        mask_alg = create_child_algorithm(self, "MaskBinsFromWorkspace", **mask_options)
        mask_alg.execute()
        return mask_alg.getProperty("OutputWorkspace").value

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

    def _convert_to_q(self, state_serialized, workspace, wavelength_adjustment_workspace, pixel_adjustment_workspace,
                      wavelength_and_pixel_adjustment_workspace):
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
        convert_name = "SANSConvertToQ"
        convert_options = {"InputWorkspace": workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "SANSState": state_serialized,
                           "OutputParts": True}
        if wavelength_adjustment_workspace:
            convert_options.update({"InputWorkspaceWavelengthAdjustment": wavelength_adjustment_workspace})
        if pixel_adjustment_workspace:
            convert_options.update({"InputWorkspacePixelAdjustment": pixel_adjustment_workspace})
        if wavelength_and_pixel_adjustment_workspace:
            convert_options.update({"InputWorkspaceWavelengthAndPixelAdjustment":
                                    wavelength_and_pixel_adjustment_workspace})
        convert_alg = create_child_algorithm(self, convert_name, **convert_options)
        convert_alg.execute()
        data_workspace = convert_alg.getProperty("OutputWorkspace").value
        sum_of_counts = convert_alg.getProperty("SumOfCounts").value
        sum_of_norms = convert_alg.getProperty("SumOfNormFactors").value
        return data_workspace, sum_of_counts, sum_of_norms

    def _get_state(self):
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        state.property_manager = state_property_manager
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
        clone_options = {"InputWorkspace": workspace,
                         "OutputWorkspace": EMPTY_NAME}
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
                    rebin_option = {"InputWorkspace": workspace,
                                    "Params": compatibility.time_rebin_string,
                                    "OutputWorkspace": EMPTY_NAME,
                                    "PreserveEvents": False}
                    rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                    rebin_alg.execute()
                    workspace = rebin_alg.getProperty("OutputWorkspace").value
                else:
                    rebin_name = "RebinToWorkspace"
                    rebin_option = {"WorkspaceToRebin": workspace,
                                    "WorkspaceToMatch": monitor_workspace,
                                    "OutputWorkspace": EMPTY_NAME,
                                    "PreserveEvents": False}
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
                extract_spectrum_option = {"InputWorkspace": workspace,
                                           "OutputWorkspace": "dummy_mask_workspace",
                                           "WorkspaceIndex": 0}
                extract_spectrum_alg = create_child_algorithm(self, extract_spectrum_name, **extract_spectrum_option)
                extract_spectrum_alg.execute()
                dummy_mask_workspace = extract_spectrum_alg.getProperty("OutputWorkspace").value

                rebin_name = "RebinToWorkspace"
                rebin_option = {"WorkspaceToRebin": dummy_mask_workspace,
                                "WorkspaceToMatch": monitor_workspace,
                                "OutputWorkspace": "dummy_mask_workspace",
                                "PreserveEvents": False}
                rebin_alg = create_child_algorithm(self, rebin_name, **rebin_option)
                rebin_alg.execute()
                dummy_mask_workspace = rebin_alg.getProperty("OutputWorkspace").value
        return workspace, dummy_mask_workspace, use_dummy_workspace
