# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSCreateAdjustmentWorkspaces algorithm creates workspaces for pixel adjustment
    , wavelength adjustment and pixel-and-wavelength adjustment workspaces.
"""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator, CompositeValidator)
from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        WorkspaceUnitValidator)

from sans.common.constants import EMPTY_NAME
from sans.common.enums import (DataType, DetectorType)
from sans.common.general_functions import create_unmanaged_algorithm
from sans.state.state_base import create_deserialized_sans_state_from_property_manager


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
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

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
        allowed_detector_types = StringListValidator([DetectorType.to_string(DetectorType.HAB),
                                                      DetectorType.to_string(DetectorType.LAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB),
                             validator=allowed_detector_types, direction=Direction.Input,
                             doc="The component of the instrument which is currently being investigated.")

        # The data type
        allowed_data = StringListValidator([DataType.to_string(DataType.Sample),
                                            DataType.to_string(DataType.Can)])
        self.declareProperty("DataType", DataType.to_string(DataType.Sample),
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
                                                     ,optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The calculated transmission workspace')

        self.declareProperty(MatrixWorkspaceProperty('UnfittedTransmissionWorkspace', ''
                                                     ,optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The unfitted transmission workspace')

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        # --------------------------------------
        # Get the monitor normalization workspace
        # --------------------------------------
        monitor_normalization_workspace = self._get_monitor_normalization_workspace(state)

        # --------------------------------------
        # Get the calculated transmission
        # --------------------------------------
        calculated_transmission_workspace, unfitted_transmission_workspace =\
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
        pixel_length_adjustment_workspace = self._get_wavelength_and_pixel_adjustment_workspaces(state,
                                                                            monitor_normalization_workspace,  # noqa
                                                                            calculated_transmission_workspace)  # noqa

        if wave_length_adjustment_workspace:
            self.setProperty("OutputWorkspaceWavelengthAdjustment", wave_length_adjustment_workspace)
        if pixel_length_adjustment_workspace:
            self.setProperty("OutputWorkspacePixelAdjustment", pixel_length_adjustment_workspace)
        if wave_length_and_pixel_adjustment_workspace:
            self.setProperty("OutputWorkspaceWavelengthAndPixelAdjustment", wave_length_and_pixel_adjustment_workspace)

        self.setProperty("CalculatedTransmissionWorkspace", calculated_transmission_workspace)
        self.setProperty("UnfittedTransmissionWorkspace", unfitted_transmission_workspace)

    def _get_wavelength_and_pixel_adjustment_workspaces(self, state,
                                                        monitor_normalization_workspace,
                                                        calculated_transmission_workspace):
        component = self.getProperty("Component").value

        wave_pixel_adjustment_name = "SANSCreateWavelengthAndPixelAdjustment"
        serialized_state = state.property_manager
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

    def _get_monitor_normalization_workspace(self, state):
        """
        Gets the monitor normalization workspace via the SANSNormalizeToMonitor algorithm

        :param state: a SANSState object.
        :return: the normalization workspace.
        """
        monitor_workspace = self.getProperty("MonitorWorkspace").value
        scale_factor = self.getProperty("SliceEventFactor").value

        normalize_name = "SANSNormalizeToMonitor"
        serialized_state = state.property_manager
        normalize_option = {"InputWorkspace": monitor_workspace,
                            "OutputWorkspace": EMPTY_NAME,
                            "SANSState": serialized_state,
                            "ScaleFactor": scale_factor}
        normalize_alg = create_unmanaged_algorithm(normalize_name, **normalize_option)
        normalize_alg.execute()
        ws = normalize_alg.getProperty("OutputWorkspace").value
        return ws

    def _get_calculated_transmission_workspace(self, state):
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
            transmission_name = "SANSCalculateTransmission"
            serialized_state = state.property_manager
            transmission_options = {"TransmissionWorkspace": transmission_workspace,
                                    "DirectWorkspace": direct_workspace,
                                    "SANSState": serialized_state,
                                    "DataType": data_type,
                                    "OutputWorkspace": EMPTY_NAME,
                                    "UnfittedData": EMPTY_NAME}
            transmission_alg = create_unmanaged_algorithm(transmission_name, **transmission_options)
            transmission_alg.execute()
            fitted_data = transmission_alg.getProperty("OutputWorkspace").value
            unfitted_data = transmission_alg.getProperty("UnfittedData").value
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
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.property_manager = state_property_manager
            state.validate()
        except ValueError as err:
            errors.update({"SANSCreateAdjustmentWorkspaces": str(err)})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSCreateAdjustmentWorkspaces)
