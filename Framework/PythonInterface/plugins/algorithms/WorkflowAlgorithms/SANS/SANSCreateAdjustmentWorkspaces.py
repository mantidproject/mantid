# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSCreateAdjustmentWorkspaces algorithm creates workspaces for pixel adjustment
    , wavelength adjustment and pixel-and-wavelength adjustment workspaces.
"""

from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        WorkspaceUnitValidator)
from mantid.kernel import (Direction, StringListValidator, CompositeValidator)
from sans.algorithm_detail.calculate_sans_transmission import calculate_transmission
from sans.algorithm_detail.normalize_to_sans_monitor import normalize_to_monitor
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (DataType, DetectorType)
from sans.common.general_functions import create_unmanaged_algorithm
from sans.state.Serializer import Serializer


class SANSCreateAdjustmentWorkspaces(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Adjust'

    def summary(self):
        return 'Calculates wavelength adjustment, pixel adjustment workspaces and wavelength-and-pixel ' \
               'adjustment workspaces.'

    def PyInit(self):
        # ---------------
        # INPUT
        # ---------------
        # State
        self.declareProperty('SANSState', '',
                             doc='A JSON string which fulfills the SANSState contract.')
        self.declareProperty('WavelengthMin', '', doc='Minimum Wavelength of associated workspace')
        self.declareProperty('WavelengthMax', '', doc='Maximum Wavelength of associated workspace')

        # Input workspaces
        self.declareProperty(MatrixWorkspaceProperty('TransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The transmission workspace.')
        self.declareProperty(MatrixWorkspaceProperty('DirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The direct workspace.')
        self.declareProperty(MatrixWorkspaceProperty('MonitorWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The scatter monitor workspace. This workspace only contains monitors.')

        workspace_validator = CompositeValidator()
        workspace_validator.add(WorkspaceUnitValidator("Wavelength"))
        self.declareProperty(MatrixWorkspaceProperty('SampleData', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input,
                                                     validator=workspace_validator),
                             doc='A workspace cropped to the detector to be reduced (the SAME as the input to Q1D). '
                                 'This used to verify the solid angle. The workspace is not modified, just inspected.')

        # The component
        allowed_detector_types = StringListValidator([DetectorType.HAB.value,
                                                      DetectorType.LAB.value])
        self.declareProperty("Component", DetectorType.LAB.value,
                             validator=allowed_detector_types, direction=Direction.Input,
                             doc="The component of the instrument which is currently being investigated.")

        # The data type
        allowed_data = StringListValidator([DataType.SAMPLE.value,
                                            DataType.CAN.value])
        self.declareProperty("DataType", DataType.SAMPLE.value,
                             validator=allowed_data, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        # Slice factor for monitor
        self.declareProperty('SliceEventFactor', 1.0, direction=Direction.Input, doc='The slice factor for the monitor '
                                                                                     'normalization. This factor is the'
                                                                                     ' one obtained from event '
                                                                                     'slicing.')

        # ---------------
        # Output
        # ---------------
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceWavelengthAdjustment', '',
                                                     direction=Direction.Output),
                             doc='The workspace for wavelength-based adjustments.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspacePixelAdjustment', '',
                                                     direction=Direction.Output),
                             doc='The workspace for wavelength-based adjustments.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspaceWavelengthAndPixelAdjustment', '',
                                                     direction=Direction.Output),
                             doc='The workspace for, both, wavelength- and pixel-based adjustments.')

        self.declareProperty(MatrixWorkspaceProperty('CalculatedTransmissionWorkspace', ''
                                                     , optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The calculated transmission workspace')

        self.declareProperty(MatrixWorkspaceProperty('UnfittedTransmissionWorkspace', ''
                                                     , optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The unfitted transmission workspace')

    def PyExec(self):
        # Read the state
        state_json = self.getProperty("SANSState").value
        state = Serializer.from_json(state_json)

        # --------------------------------------
        # Get the monitor normalization workspace
        # --------------------------------------
        monitor_normalization_workspace = self._get_monitor_normalization_workspace(state)

        # --------------------------------------
        # Get the calculated transmission
        # --------------------------------------
        calculated_transmission_workspace, unfitted_transmission_workspace = \
            self._get_calculated_transmission_workspace(state)

        # --------------------------------------
        # Get the wide angle correction workspace
        # --------------------------------------
        wave_length_and_pixel_adjustment_workspace = self._get_wide_angle_correction_workspace(state,
                                                                                               calculated_transmission_workspace)  # noqa

        # --------------------------------------------
        # Get the full wavelength and pixel adjustment
        # --------------------------------------------
        wave_length_adjustment_workspace, \
        pixel_length_adjustment_workspace = self._get_wavelength_and_pixel_adjustment_workspaces(monitor_normalization_workspace,
                                                                                                 # noqa
                                                                                                 calculated_transmission_workspace)  # noqa

        if wave_length_adjustment_workspace:
            self.setProperty("OutputWorkspaceWavelengthAdjustment", wave_length_adjustment_workspace)
        if pixel_length_adjustment_workspace:
            self.setProperty("OutputWorkspacePixelAdjustment", pixel_length_adjustment_workspace)
        if wave_length_and_pixel_adjustment_workspace:
            self.setProperty("OutputWorkspaceWavelengthAndPixelAdjustment", wave_length_and_pixel_adjustment_workspace)

        self.setProperty("CalculatedTransmissionWorkspace", calculated_transmission_workspace)
        self.setProperty("UnfittedTransmissionWorkspace", unfitted_transmission_workspace)

    def _get_wavelength_and_pixel_adjustment_workspaces(self,
                                                        monitor_normalization_workspace,
                                                        calculated_transmission_workspace):
        component = self.getProperty("Component").value

        wave_pixel_adjustment_name = "SANSCreateWavelengthAndPixelAdjustment"
        serialized_state = self.getProperty("SANSState").value
        wave_pixel_adjustment_options = {"SANSState": serialized_state,
                                         "NormalizeToMonitorWorkspace": monitor_normalization_workspace,
                                         "OutputWorkspaceWavelengthAdjustment": EMPTY_NAME,
                                         "OutputWorkspacePixelAdjustment": EMPTY_NAME,
                                         "Component": component}
        if calculated_transmission_workspace:
            wave_pixel_adjustment_options.update({"TransmissionWorkspace": calculated_transmission_workspace})
        wave_pixel_adjustment_alg = create_unmanaged_algorithm(wave_pixel_adjustment_name,
                                                               **wave_pixel_adjustment_options)

        wave_pixel_adjustment_alg.execute()
        wavelength_out = wave_pixel_adjustment_alg.getProperty("OutputWorkspaceWavelengthAdjustment").value
        pixel_out = wave_pixel_adjustment_alg.getProperty("OutputWorkspacePixelAdjustment").value
        return wavelength_out, pixel_out

    def _get_monitor_normalization_workspace(self, state, wav_range):
        """
        Gets the monitor normalization workspace via the NormalizeToSansMonitor algorithm

        :param state: a SANSState object.
        :return: the normalization workspace.
        """
        monitor_workspace = self.getProperty("MonitorWorkspace").value
        scale_factor = self.getProperty("SliceEventFactor").value

        ws = normalize_to_monitor(workspace=monitor_workspace, scale_factor=scale_factor,
                                  state_adjustment_normalize_to_monitor=state.adjustment.normalize_to_monitor,
                                  wav_range=wav_range)

        return ws

    def _get_calculated_transmission_workspace(self, state, wav_range):
        """
        Creates the fitted transmission workspace.

        Note that this step is not mandatory. If no transmission and direct workspaces are provided, then we
        don't have to do anything here.
        :param state: a SANSState object.
        :return: a fitted transmission workspace and the unfitted data.
        """
        transmission_workspace = self.getProperty("TransmissionWorkspace").value
        direct_workspace = self.getProperty("DirectWorkspace").value
        if transmission_workspace and direct_workspace:
            data_type = self.getProperty("DataType").value

            fitted_data, unfitted_data = \
                calculate_transmission(direct_ws=direct_workspace, data_type_str=data_type,
                                       transmission_ws=transmission_workspace,
                                       state_adjustment_calculate_transmission=state.adjustment.calculate_transmission,
                                       wav_range=wav_range)

        else:
            fitted_data = None
            unfitted_data = None
        return fitted_data, unfitted_data

    def _get_wide_angle_correction_workspace(self, state, calculated_transmission_workspace):
        wide_angle_correction = state.adjustment.wide_angle_correction
        sample_data = self.getProperty("SampleData").value
        workspace = None
        if wide_angle_correction and sample_data and calculated_transmission_workspace:
            wide_angle_name = "SANSWideAngleCorrection"
            wide_angle_options = {"SampleData": sample_data,
                                  "TransmissionData": calculated_transmission_workspace,
                                  "OutputWorkspace": EMPTY_NAME}
            wide_angle_alg = create_unmanaged_algorithm(wide_angle_name, **wide_angle_options)
            wide_angle_alg.execute()
            workspace = wide_angle_alg.getProperty("OutputWorkspace").value
        return workspace

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = Serializer.from_json(state_property_manager)
            state.validate()
        except ValueError as err:
            errors.update({"SANSCreateAdjustmentWorkspaces": str(err)})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSCreateAdjustmentWorkspaces)
