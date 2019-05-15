# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSReductionCoreEventSlice algorithm runs the sequence of reduction steps which are necessary to reduce a data set,
for which data must be event sliced. These steps are: slicing, adjustment, convert to q."""

from __future__ import (absolute_import, division, print_function)

from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        IEventWorkspace, Progress)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (DetectorType, DataType)
from sans.common.general_functions import (create_child_algorithm, append_to_sans_file_tag)
from sans.state.state_base import create_deserialized_sans_state_from_property_manager


class SANSReductionCoreEventSlice(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return 'Runs the the core reduction elements which need to be carried out ' \
               'on individual event slices.'

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
        # Wavelength Adjustment
        self.declareProperty(MatrixWorkspaceProperty('WavelengthAdjustmentWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The direct workspace.')
        # Pixel Adjustment
        self.declareProperty(MatrixWorkspaceProperty('PixelAdjustmentWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The direct workspace.')
        # Wavelength and Pixel Adjustment
        self.declareProperty(MatrixWorkspaceProperty('WavelengthAndPixelAdjustmentWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The direct workspace.')

        self.setPropertyGroup("ScatterWorkspace", 'Data')
        self.setPropertyGroup("ScatterMonitorWorkspace", 'Data')
        self.setPropertyGroup("WavelengthAdjustmentWorkspace", 'Data')
        self.setPropertyGroup("PixelAdjustmentWorkspace", 'Data')
        self.setPropertyGroup("WavelengthAndPixelAdjustmentWorkspace", 'Data')

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

    def PyExec(self):
        # Get the input
        state = self._get_state()
        state_serialized = state.property_manager
        progress = self._get_progress()

        workspace = self.getProperty("ScatterWorkspace").value
        # --------------------------------------------------------------------------------------------------------------
        # 1. Create event slice
        #    If we are dealing with an event workspace as input, this will cut out a time-based (user-defined) slice.
        #    In case of a histogram workspace, nothing happens.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Event slicing ...")
        data_type_as_string = self.getProperty("DataType").value
        monitor_workspace = self._get_monitor_workspace()
        workspace, monitor_workspace, slice_event_factor = self._slice(state_serialized, workspace, monitor_workspace,
                                                                       data_type_as_string)

        # ------------------------------------------------------------
        # 2. Convert event workspaces to histogram workspaces
        # ------------------------------------------------------------
        progress.report("Converting to histogram mode ...")
        workspace = self._convert_to_histogram(workspace)

        # ------------------------------------------------------------
        # 3. Re-mask. We need to bin mask in histogram mode in order
        #    to have knowledge of masked regions: masking
        #    EventWorkspaces simply removes their events
        # ------------------------------------------------------------
        progress.report("Masking bin ...")
        workspace = self._mask_bins(state, workspace, component_as_string)

        # ------------------------------------------------------------
        # 4. Convert to Q
        # -----------------------------------------------------------
        progress.report("Converting to q ...")
        wavelength_adjustment_workspace = self.getProperty("WavelengthAdjustmentWorkspace").value
        pixel_adjustment_workspace = self.getProperty("PixelAdjustmentWorkspace").value
        wavelength_and_pixel_adjustment_workspace = self.getProperty("WavelengthAndPixelAdjustmentWorkspace").value
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

    def _mask_bins(self, state, workspace, component):
        instrument = workspace.getInstrument().getName()
        mask_info = state.mask

        bin_mask_general_start = mask_info.bin_mask_general_start
        bin_mask_general_stop = mask_info.bin_mask_general_stop

        # Convert the bins with the detector-specific setting
        bin_mask_start = mask_info.detectors[component].bin_mask_start
        bin_mask_stop = mask_info.detectors[component].bin_mask_stop

        if bin_mask_general_start and bin_mask_general_stop:
            bin_mask_general_start, \
                bin_mask_general_stop = self._convert_mask_values(bin_mask_general_start,
                                                                  bin_mask_general_stop,
                                                                  instrument,
                                                                  state.property_manager)
            mask_info.bin_mask_general_start = bin_mask_general_start
            mask_info.bin_mask_general_stop = bin_mask_general_stop

        if bin_mask_start and bin_mask_stop:
            bin_mask_start, bin_mask_stop = self._convert_mask_values(bin_mask_start, bin_mask_stop,
                                                                      instrument,
                                                                      state.property_manager)
            mask_info.detectors[component].bin_mask_start = bin_mask_start
            mask_info.detectors[component].bin_mask_stop = bin_mask_stop

        component = DetectorType.from_string(component)
        return mask_bins(mask_info, workspace, component)

    def _convert_mask_values(self, mask_start, mask_stop, instrument, state_serialized):
        """
        Convert the mask boundaries from TOF to wavelength and then scale.
        To convert units, we create a workspace with the boundaries as bins,
        ConvertUnits on the workspace, and extract the new bins.

        :param mask_start: the start of the masking region, in TOF
        :param mask_stop: the end of the masking region, in TOF
        :param instrument: the instrument of the workspace, necessary for
                           converting units on the dummy workspace
        :param state_serialized: a SANS state object
        :return: mask start in wavelength, mask stop in wavelength
        """
        # Create the workspace
        create_workspace_options = {"OutputWorkspace": "TemporaryWorkspace",
                                    "DataX": [mask_start[0], mask_stop[0]],
                                    "DataY": [0.],
                                    "UnitX": "TOF"}
        create_workspace_alg = create_managed_non_child_algorithm("CreateWorkspace", **create_workspace_options)
        create_workspace_alg.execute()

        # Give it an instrument
        load_instrument_options = {"Workspace": "TemporaryWorkspace",
                                   "RewriteSpectraMap": True,
                                   "InstrumentName": instrument}
        load_instrument_alg = create_managed_non_child_algorithm("LoadInstrument", **load_instrument_options)
        load_instrument_alg.execute()

        # Convert units to wavelength
        convert_units_options = {"InputWorkspace": "TemporaryWorkspace",
                                 "OutputWorkspace": "TemporaryWorkspace",
                                 "Target": "Wavelength"}
        # Child so we can get workspace directly from the algorithm
        convert_units_alg = create_child_algorithm(self, "ConvertUnits", **convert_units_options)
        convert_units_alg.execute()
        temp_workspace = convert_units_alg.getProperty("OutputWorkspace").value

        scale_name = "SANSScale"
        scale_options = {"SANSState": state_serialized,
                         "InputWorkspace": temp_workspace,
                         "OutputWorkspace": "TemporaryWorkspace"}
        scale_alg = create_child_algorithm(self, scale_name, **scale_options)
        scale_alg.execute()
        temp_workspace = scale_alg.getProperty("OutputWorkspace").value

        # Get the new values
        new_boundaries = temp_workspace.extractX()[0]  # extractX returns a 2d numpy array
        new_mask_start = [float(new_boundaries[0])]
        new_mask_stop = [float(new_boundaries[1])]

        # Delete the workspace
        delete_alg = create_managed_non_child_algorithm("DeleteWorkspace", **{"Workspace": "TemporaryWorkspace"})
        delete_alg.execute()

        return new_mask_start, new_mask_stop

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
            errors.update({"SANSSingleReductionEventSlice": str(err)})
        return errors

    def _get_state(self):
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        state.property_manager = state_property_manager
        return state

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
        return Progress(self, start=0.0, end=1.0, nreports=5)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSReductionCoreEventSlice)
