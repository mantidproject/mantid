# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSCreateWavelengthAndPixelAdjustment algorithm creates workspaces for pixel adjustment
    and wavelength adjustment.
"""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, StringListValidator, PropertyManagerProperty, CompositeValidator)

from mantid.api import (ParallelDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        WorkspaceUnitValidator)

from sans.state.state_base import create_deserialized_sans_state_from_property_manager
from sans.common.enums import (RangeStepType, DetectorType)
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_unmanaged_algorithm


class SANSCreateWavelengthAndPixelAdjustment(ParallelDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Adjust'

    def summary(self):
        return 'Calculates wavelength adjustment and pixel adjustment workspaces.'

    def PyInit(self):
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')
        # Input workspaces
        workspace_validator = CompositeValidator()
        workspace_validator.add(WorkspaceUnitValidator("Wavelength"))

        self.declareProperty(MatrixWorkspaceProperty("TransmissionWorkspace", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input,
                                                     validator=workspace_validator),
                             doc='The calculated transmission workspace in wavelength units.')
        self.declareProperty(MatrixWorkspaceProperty("NormalizeToMonitorWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input,
                                                     validator=workspace_validator),
                             doc='The monitor normalization workspace in wavelength units.')
        allowed_detector_types = StringListValidator([DetectorType.HAB.name,
                                                      DetectorType.LAB.name])
        self.declareProperty("Component", DetectorType.LAB.name,
                             validator=allowed_detector_types, direction=Direction.Input,
                             doc="The component of the instrument which is currently being investigated.")

        # Output workspace
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspaceWavelengthAdjustment", '',
                                                     direction=Direction.Output),
                             doc='A wavelength adjustment output workspace.')
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspacePixelAdjustment", '',
                                                     direction=Direction.Output),
                             doc='A pixel adjustment output workspace.')

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        wavelength_and_pixel_adjustment_state = state.adjustment.wavelength_and_pixel_adjustment

        # Get the wavelength adjustment workspace
        transmission_workspace = self.getProperty("TransmissionWorkspace").value
        monitor_normalization_workspace = self.getProperty("NormalizeToMonitorWorkspace").value

        component = self.getProperty("Component").value
        wavelength_adjustment_file = wavelength_and_pixel_adjustment_state.adjustment_files[component].wavelength_adjustment_file

        rebin_string = self._get_rebin_string(wavelength_and_pixel_adjustment_state)
        wavelength_adjustment_workspace = self._get_wavelength_adjustment_workspace(wavelength_adjustment_file,
                                                                                    transmission_workspace,
                                                                                    monitor_normalization_workspace,
                                                                                    rebin_string)

        # Get the pixel adjustment workspace
        pixel_adjustment_file = wavelength_and_pixel_adjustment_state.adjustment_files[component].pixel_adjustment_file
        idf_path = wavelength_and_pixel_adjustment_state.idf_path
        pixel_adjustment_workspace = self._get_pixel_adjustment_workspace(pixel_adjustment_file, component, idf_path)

        # Set the output
        if wavelength_adjustment_workspace:
            self.setProperty("OutputWorkspaceWavelengthAdjustment", wavelength_adjustment_workspace)
        if pixel_adjustment_workspace:
            self.setProperty("OutputWorkspacePixelAdjustment", pixel_adjustment_workspace)

    def _get_wavelength_adjustment_workspace(self, wavelength_adjustment_file, transmission_workspace,
                                             monitor_normalization_workspace, rebin_string):
        """
        This creates a workspace with wavelength adjustments, ie this will be a correction for the bins, but it will
        be the same for all pixels. This is essentially the product of several workspaces.
        The participating workspaces are:
        1. A workspace loaded from a calibration file
        2.. The workspace resulting from the monitor normalization
        3. The workspace resulting from the transmission calculation (using SANSCalculateTransmission) if applicable

        :param wavelength_adjustment_file: the file path to the wavelength adjustment file
        :param transmission_workspace: the calculated transmission workspace (which can be None)
        :param monitor_normalization_workspace: the monitor normalization workspace
        :param rebin_string: the parameters for rebinning
        :return: a general wavelength adjustment workspace
        """
        # 1. Get the wavelength correction workspace from the file
        wavelength_adjustment_workspaces = []
        if wavelength_adjustment_file:
            wavelength_correction_workspace_from_file = \
                self._load_wavelength_correction_file(wavelength_adjustment_file)
            wavelength_adjustment_workspaces.append(wavelength_correction_workspace_from_file)

        # 2. Normalization
        wavelength_adjustment_workspaces.append(monitor_normalization_workspace)

        # 3. Transmission Calculation
        if transmission_workspace:
            wavelength_adjustment_workspaces.append(transmission_workspace)

        # Multiply all workspaces
        wavelength_adjustment_workspace = None
        for workspace in wavelength_adjustment_workspaces:
            # First we need to change the binning such that is matches the binning of the main data workspace
            rebin_name = "Rebin"
            rebin_options = {"InputWorkspace": workspace,
                             "Params": rebin_string,
                             "OutputWorkspace": EMPTY_NAME}
            rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_options)
            rebin_alg.execute()
            rebinned_workspace = rebin_alg.getProperty("OutputWorkspace").value
            if wavelength_adjustment_workspace is None:
                wavelength_adjustment_workspace = rebinned_workspace
            else:
                multiply_name = "Multiply"
                multiply_options = {"LHSWorkspace": rebinned_workspace,
                                    "RHSWorkspace": wavelength_adjustment_workspace,
                                    "OutputWorkspace": EMPTY_NAME}
                multiply_alg = create_unmanaged_algorithm(multiply_name, **multiply_options)
                multiply_alg.execute()
                wavelength_adjustment_workspace = multiply_alg.getProperty("OutputWorkspace").value
        return wavelength_adjustment_workspace

    def _load_wavelength_correction_file(self, file_name):
        correction_workspace = None
        if file_name:
            load_name = "LoadRKH"
            load_option = {"Filename": file_name,
                           "OutputWorkspace": EMPTY_NAME,
                           "FirstColumnValue": "Wavelength"}
            load_alg = create_unmanaged_algorithm(load_name, **load_option)
            load_alg.execute()
            output_workspace = load_alg.getProperty("OutputWorkspace").value
            # We require HistogramData and not PointData
            if not output_workspace.isHistogramData():
                convert_name = "ConvertToHistogram"
                convert_options = {"InputWorkspace": output_workspace,
                                   "OutputWorkspace": EMPTY_NAME}
                convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
                convert_alg.execute()
                output_workspace = convert_alg.getProperty("OutputWorkspace").value
            correction_workspace = output_workspace
        return correction_workspace

    def _get_pixel_adjustment_workspace(self, pixel_adjustment_file, component, idf_path):
        """
        This get the pixel-by-pixel adjustment of the workspace

        :param pixel_adjustment_file: full file path to the pixel adjustment file
        :param component: the component which is currently being investigated
        :param idf_path: the idf path
        :return: the pixel adjustment workspace
        """
        if pixel_adjustment_file:
            load_name = "LoadRKH"
            load_options = {"Filename": pixel_adjustment_file,
                            "OutputWorkspace": EMPTY_NAME,
                            "FirstColumnValue": "SpectrumNumber"}
            load_alg = create_unmanaged_algorithm(load_name, **load_options)
            load_alg.execute()
            output_workspace = load_alg.getProperty("OutputWorkspace").value

            # Add an instrument to the workspace
            instrument_name = "LoadInstrument"
            instrument_options = {"Workspace": output_workspace,
                                  "Filename": idf_path,
                                  "RewriteSpectraMap": False}
            instrument_alg = create_unmanaged_algorithm(instrument_name, **instrument_options)
            instrument_alg.execute()

            # Crop to the required detector
            crop_name = "SANSCrop"
            crop_options = {"InputWorkspace": output_workspace,
                            "OutputWorkspace": EMPTY_NAME,
                            "Component": component}
            crop_alg = create_unmanaged_algorithm(crop_name, **crop_options)
            crop_alg.execute()
            pixel_adjustment_workspace = crop_alg.getProperty("OutputWorkspace").value
        else:
            pixel_adjustment_workspace = None
        return pixel_adjustment_workspace

    def _get_rebin_string(self, wavelength_and_pixel_adjustment_state):
        wavelength_low = wavelength_and_pixel_adjustment_state.wavelength_low[0]
        wavelength_high = wavelength_and_pixel_adjustment_state.wavelength_high[0]
        wavelength_step = wavelength_and_pixel_adjustment_state.wavelength_step
        wavelength_step_type = -1.0 if wavelength_and_pixel_adjustment_state.wavelength_step_type \
                                       is RangeStepType.Log else 1.0  # noqa

        # Create a rebin string from the wavelength information
        wavelength_step *= wavelength_step_type
        return str(wavelength_low) + "," + str(wavelength_step) + "," + str(wavelength_high)

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.validate()
        except ValueError as err:
            errors.update({"SANSCreateWavelengthAndPixelAdjustment": str(err)})

        # The transmission and the normalize to monitor workspace must have exactly one histogram present
        transmission_workspace = self.getProperty("TransmissionWorkspace").value
        normalize_to_monitor = self.getProperty("NormalizeToMonitorWorkspace").value
        if transmission_workspace and transmission_workspace.getNumberHistograms() != 1:
            errors.update({"TransmissionWorkspace": "The transmission workspace can have only one histogram."})
        if normalize_to_monitor.getNumberHistograms() != 1:
            errors.update({"NormalizeToMonitorWorkspace": "The monitor normalization workspace can have"
                                                          " only one histogram."})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSCreateWavelengthAndPixelAdjustment)
