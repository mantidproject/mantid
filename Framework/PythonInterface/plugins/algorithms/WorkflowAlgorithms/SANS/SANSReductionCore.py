# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSReductionCore algorithm runs the sequence of reduction steps which are necessary to reduce a data set."""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        IEventWorkspace, Progress)

from sans.state.state_base import create_deserialized_sans_state_from_property_manager
from sans.algorithm_detail.mask_workspace import mask_bins
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import (create_child_algorithm, append_to_sans_file_tag)
from sans.common.enums import (DetectorType, DataType)


class SANSReductionCore(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return ' Runs the the core reduction elements.'

    def PyInit(self):
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

    def PyExec(self):
        # Get the input
        state = self._get_state()
        state_serialized = state.property_manager
        component_as_string = self.getProperty("Component").value
        progress = self._get_progress()

        # --------------------------------------------------------------------------------------------------------------
        # 1. Crop workspace by detector name
        #    This will create a reduced copy of the original workspace with only those spectra which are relevant
        #    for this particular reduction.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Cropping ...")
        workspace = self._get_cropped_workspace(component_as_string)

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
        monitor_workspace = self._get_monitor_workspace()
        workspace, monitor_workspace, slice_event_factor = self._slice(state_serialized, workspace, monitor_workspace,
                                                                       data_type_as_string)

        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN
        # The old reduction workflow converted the workspace to a histogram at this point.
        # A more recent workflow keeps the workspaces as Events for longer, to make use of cheap rebinning for
        # EventWorkspaces, and to optimise for event slicing.
        # However, in the new workflow ("non-compatibility mode") it is necessary to keep a workspace as a histogram
        # to keep track of the bin masking. These masks are lifted from the dummy workspace to the actual workspace
        # near the end of the reduction.
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        compatibility = state.compatibility
        is_event_workspace = isinstance(workspace, IEventWorkspace)
        use_dummy_workspace = False
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
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY END
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        # ------------------------------------------------------------
        # 4. Move the workspace into the correct position
        #    The detectors in the workspaces are set such that the beam centre is at (0,0). The position is
        #    a user-specified value which can be obtained with the help of the beam centre finder.
        # ------------------------------------------------------------
        progress.report("Moving ...")
        workspace = self._move(state_serialized, workspace, component_as_string)
        monitor_workspace = self._move(state_serialized, monitor_workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 5. Apply masking (pixel masking and time masking)
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Masking ...")
        workspace = self._mask(state_serialized, workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 6. Convert to Wavelength
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Converting to wavelength ...")
        workspace = self._convert_to_wavelength(state_serialized, workspace)
        # Convert and rebin the dummy workspace to get correct bin flags
        if use_dummy_workspace:
            dummy_mask_workspace = mask_bins(state.mask, dummy_mask_workspace, DetectorType.from_string(component_as_string))
            dummy_mask_workspace = self._convert_to_wavelength(state_serialized, dummy_mask_workspace)

        # --------------------------------------------------------------------------------------------------------------
        # 7. Multiply by volume and absolute scale
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Multiplying by volume and absolute scale ...")
        workspace = self._scale(state_serialized, workspace)

        # ------------------------------------------------------------
        # 8. Convert event workspaces to histogram workspaces
        # ------------------------------------------------------------
        progress.report("Converting to histogram mode ...")
        workspace = self._convert_to_histogram(workspace)

        # ------------------------------------------------------------
        # 9. ? Re-mask. We need to bin mask in histogram mode in order
        #      to have knowledge of masked regions: masking
        #      EventWorkspaces simply removes their events
        # ------------------------------------------------------------
        if use_dummy_workspace:
            workspace = self._copy_bin_masks(workspace, dummy_mask_workspace)

        # --------------------------------------------------------------------------------------------------------------
        # 9. Create adjustment workspaces, those are
        #     1. pixel-based adjustments
        #     2. wavelength-based adjustments
        #     3. pixel-and-wavelength-based adjustments
        # Note that steps 4 to 7 could run in parallel if we don't use wide angle correction. If we do then the
        # creation of the adjustment workspaces requires the sample workspace itself and we have to run it sequentially.
        # We could consider to have a serial and a parallel strategy here, depending on the wide angle correction
        # settings. On the other hand it is not clear that this would be an advantage with the GIL.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Creating adjustment workspaces ...")
        wavelength_adjustment_workspace, pixel_adjustment_workspace, wavelength_and_pixel_adjustment_workspace, \
            calculated_transmission_workspace, unfitted_transmission_workspace = \
            self._adjustment(state_serialized, workspace, monitor_workspace, component_as_string, data_type_as_string)

        # ------------------------------------------------------------
        # 9. Convert event workspaces to histogram workspaces
        # ------------------------------------------------------------
        progress.report("Converting to histogram mode ...")
        workspace = self._convert_to_histogram(workspace)

        # ------------------------------------------------------------
        # 10. Convert to Q
        # -----------------------------------------------------------
        progress.report("Converting to q ...")
        workspace, sum_of_counts, sum_of_norms = self._convert_to_q(state_serialized,
                                                                    workspace,
                                                                    wavelength_adjustment_workspace,
                                                                    pixel_adjustment_workspace,
                                                                    wavelength_and_pixel_adjustment_workspace)
        progress.report("Completed SANSReductionCore ...")

        # ------------------------------------------------------------
        # Populate the output
        # ------------------------------------------------------------
        self.setProperty("OutputWorkspace", workspace)

        # ------------------------------------------------------------
        # Diagnostic output
        # ------------------------------------------------------------
        if sum_of_counts:
            self.setProperty("SumOfCounts", sum_of_counts)
        if sum_of_norms:
            self.setProperty("SumOfNormFactors", sum_of_norms)

        self.setProperty("CalculatedTransmissionWorkspace", calculated_transmission_workspace)
        self.setProperty("UnfittedTransmissionWorkspace", unfitted_transmission_workspace)

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

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSReductionCore)
