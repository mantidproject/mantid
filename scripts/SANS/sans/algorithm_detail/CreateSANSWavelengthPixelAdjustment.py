# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""CreateSANSWavelengthPixelAdjustment class creates workspaces for pixel adjustment
and wavelength adjustment.
"""

from sans.algorithm_detail.crop_helper import get_component_name
from SANS.sans.common.constants import EMPTY_NAME
from SANS.sans.common.enums import DetectorType
from SANS.sans.common.enums import RangeStepType
from SANS.sans.common.general_functions import create_unmanaged_algorithm
from SANS.sans.state.StateObjects.wavelength_interval import WavRange


class CreateSANSWavelengthPixelAdjustment(object):
    def __init__(self, state_adjustment_wavelength_and_pixel, component):
        """
        :param state_adjustment_wavelength_and_pixel: The state.adjustment.wavelength_and_pixel state
        :param component: The component of the instrument which is currently being investigated.
                          Allowed values: ['HAB', 'LAB']
        """
        self._component = component
        self._state = state_adjustment_wavelength_and_pixel

    def create_sans_wavelength_and_pixel_adjustment(self, transmission_ws, monitor_norm_ws, wav_range):
        """
        Calculates wavelength adjustment and pixel adjustment workspaces.
        :param transmission_ws: The calculated transmission workspace in wavelength units.
        :param monitor_norm_ws: The monitor normalization workspace in wavelength units.
        :return: A tuple containing: Wavelength Adjustment workspace and Pixel Adjustment workspace
        """

        # Read the state
        wavelength_and_pixel_adjustment_state = self._state

        # Get the wavelength adjustment workspace
        component = self._component
        adj_file = wavelength_and_pixel_adjustment_state.adjustment_files[component].wavelength_adjustment_file

        rebin_string = self._get_rebin_string(wavelength_and_pixel_adjustment_state, wav_range=wav_range)
        wavelength_adjustment_workspace = self._get_wavelength_adjustment_workspace(
            adj_file, transmission_ws, monitor_norm_ws, rebin_string
        )

        # Get the pixel adjustment workspace
        pixel_adjustment_file = wavelength_and_pixel_adjustment_state.adjustment_files[component].pixel_adjustment_file
        idf_path = wavelength_and_pixel_adjustment_state.idf_path
        pixel_adjustment_workspace = self._get_pixel_adjustment_workspace(pixel_adjustment_file, component, idf_path)

        return wavelength_adjustment_workspace, pixel_adjustment_workspace

    def _get_wavelength_adjustment_workspace(
        self, wavelength_adjustment_file, transmission_workspace, monitor_normalization_workspace, rebin_string
    ):
        """
        This creates a workspace with wavelength adjustments, ie this will be a correction for the bins, but it will
        be the same for all pixels. This is essentially the product of several workspaces.
        The participating workspaces are:
        1. A workspace loaded from a calibration file
        2.. The workspace resulting from the monitor normalization
        3. The workspace resulting from the transmission calculation (using CalculateSANSTransmission) if applicable

        :param wavelength_adjustment_file: the file path to the wavelength adjustment file
        :param transmission_workspace: the calculated transmission workspace (which can be None)
        :param monitor_normalization_workspace: the monitor normalization workspace
        :param rebin_string: the parameters for rebinning
        :return: a general wavelength adjustment workspace
        """
        # 1. Get the wavelength correction workspace from the file
        wavelength_adjustment_workspaces = []
        if wavelength_adjustment_file:
            wavelength_correction_workspace_from_file = self._load_wavelength_correction_file(wavelength_adjustment_file)
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
            rebin_options = {"InputWorkspace": workspace, "Params": rebin_string, "OutputWorkspace": EMPTY_NAME}
            rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_options)
            rebin_alg.execute()
            rebinned_workspace = rebin_alg.getProperty("OutputWorkspace").value
            if wavelength_adjustment_workspace is None:
                wavelength_adjustment_workspace = rebinned_workspace
            else:
                multiply_name = "Multiply"
                multiply_options = {
                    "LHSWorkspace": rebinned_workspace,
                    "RHSWorkspace": wavelength_adjustment_workspace,
                    "OutputWorkspace": EMPTY_NAME,
                }
                multiply_alg = create_unmanaged_algorithm(multiply_name, **multiply_options)
                multiply_alg.execute()
                wavelength_adjustment_workspace = multiply_alg.getProperty("OutputWorkspace").value
        return wavelength_adjustment_workspace

    @staticmethod
    def _load_wavelength_correction_file(file_name):
        correction_workspace = None
        if file_name:
            load_name = "LoadRKH"
            load_option = {"Filename": file_name, "OutputWorkspace": EMPTY_NAME, "FirstColumnValue": "Wavelength"}
            load_alg = create_unmanaged_algorithm(load_name, **load_option)
            load_alg.execute()
            output_workspace = load_alg.getProperty("OutputWorkspace").value
            # We require HistogramData and not PointData
            if not output_workspace.isHistogramData():
                convert_name = "ConvertToHistogram"
                convert_options = {"InputWorkspace": output_workspace, "OutputWorkspace": EMPTY_NAME}
                convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
                convert_alg.execute()
                output_workspace = convert_alg.getProperty("OutputWorkspace").value
            correction_workspace = output_workspace
        return correction_workspace

    @staticmethod
    def _get_pixel_adjustment_workspace(pixel_adjustment_file, component, idf_path):
        """
        This get the pixel-by-pixel adjustment of the workspace

        :param pixel_adjustment_file: full file path to the pixel adjustment file
        :param component: the component which is currently being investigated
        :param idf_path: the idf path
        :return: the pixel adjustment workspace
        """
        if pixel_adjustment_file:
            load_name = "LoadRKH"
            load_options = {"Filename": pixel_adjustment_file, "OutputWorkspace": EMPTY_NAME, "FirstColumnValue": "SpectrumNumber"}
            load_alg = create_unmanaged_algorithm(load_name, **load_options)
            load_alg.execute()
            output_workspace = load_alg.getProperty("OutputWorkspace").value

            if not idf_path:
                raise ValueError("No IDF path was found in the provided state")

            # Add an instrument to the workspace
            instrument_name = "LoadInstrument"
            instrument_options = {"Workspace": output_workspace, "Filename": idf_path, "RewriteSpectraMap": False}
            instrument_alg = create_unmanaged_algorithm(instrument_name, **instrument_options)
            instrument_alg.execute()

            # Crop to the required detector
            crop_name = "CropToComponent"
            component_to_crop = DetectorType(component)
            component_to_crop = get_component_name(output_workspace, component_to_crop)
            crop_options = {"InputWorkspace": output_workspace, "OutputWorkspace": EMPTY_NAME, "ComponentNames": component_to_crop}

            crop_alg = create_unmanaged_algorithm(crop_name, **crop_options)
            crop_alg.execute()
            pixel_adjustment_workspace = crop_alg.getProperty("OutputWorkspace").value
        else:
            pixel_adjustment_workspace = None
        return pixel_adjustment_workspace

    @staticmethod
    def _get_rebin_string(wavelength_and_pixel_adjustment_state, wav_range: WavRange):
        wavelength_step = wavelength_and_pixel_adjustment_state.wavelength_interval.wavelength_step
        wavelength_step_type = -1.0 if wavelength_and_pixel_adjustment_state.wavelength_step_type_lin_log is RangeStepType.LOG else 1.0

        # Create a rebin string from the wavelength information
        wavelength_step *= wavelength_step_type
        return f"{wav_range[0]}, {wavelength_step}, {wav_range[1]}"
