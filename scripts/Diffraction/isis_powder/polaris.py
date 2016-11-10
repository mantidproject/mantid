from __future__ import (absolute_import, division, print_function)

import math
import mantid.simpleapi as mantid

from isis_powder.abstract_inst import AbstractInst
from isis_powder.polaris_routines import polaris_calib_factory
from isis_powder.polaris_routines import polaris_calib_parser

import isis_powder.common as common


class Polaris(AbstractInst):

    _lower_lambda_range = 0.25
    _upper_lambda_range = 2.50  # TODO populate this

    _focus_crop_start = 2  # These are used when calculating binning range
    _focus_crop_end = 0.95
    _focus_bin_widths = [-0.0050, -0.0010, -0.0010, -0.0010, -0.00050]

    _calibration_grouping_names = None

    _number_of_banks = 5

    def __init__(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None,
                 input_file_ext=".raw", sample_empty_name=None):  # TODO move TT_mode PEARL specific

        super(Polaris, self).__init__(user_name=user_name, calibration_dir=calibration_dir, raw_data_dir=raw_data_dir,
                                      output_dir=output_dir, default_input_ext=input_file_ext)

        self._masking_file_name = "VanaPeaks.dat"
        self._sample_empty = sample_empty_name

    # Abstract implementation
    def _get_lambda_range(self):
        return self._lower_lambda_range, self._upper_lambda_range

    def _get_create_van_tof_binning(self):
        return self._create_van_calib_tof_binning

    def _get_default_group_names(self):
        return self._calibration_grouping_names

    def _get_calibration_full_paths(self, cycle):
        # TODO implement this properly
        # offset_file_name, grouping_file_name, vanadium_file_name = polaris_calib_factory.get_calibration_file(cycle)

        configuration = polaris_calib_parser.get_calibration_dict(cycle)
        calibration_dir = self.calibration_dir

        # Assume the raw vanadium is with other raw files
        vanadium_full_path = self.raw_data_dir + configuration["vanadium_file_name"]

        calibration_full_path = calibration_dir + configuration["offset_file_name"]
        grouping_full_path = calibration_dir + configuration["grouping_file_name"]

        calibrated_full_path = calibration_dir + configuration["calibrated_vanadium_file_name"]
        solid_angle_file_path = calibration_dir + configuration["solid_angle_file_name"]

        calibration_details = {"calibration": calibration_full_path,
                               "grouping": grouping_full_path,
                               "vanadium": vanadium_full_path,
                               "calibrated_vanadium": calibrated_full_path,
                               "solid_angle_corr": solid_angle_file_path}

        return calibration_details

    @staticmethod
    def _generate_inst_file_name(run_number):
        return "POL" + str(run_number)  # TODO check this is correct

    @staticmethod
    def _get_instrument_alg_save_ranges(instrument=''):
        alg_range = 5
        save_range = 0  # TODO set save range
        return alg_range, save_range

    @staticmethod
    def _get_cycle_information(run_number):
        return {"cycle": "test",  # TODO implement properly
                "instrument_version": ""}

    def _normalise_ws(self, ws_to_correct, monitor_ws=None, spline_terms=20):
        normalised_ws = mantid.NormaliseByCurrent(InputWorkspace=ws_to_correct)
        return normalised_ws

    def _mask_noisy_detectors(self, vanadium_ws):
        summed_van_ws = mantid.Integration(InputWorkspace=vanadium_ws)
        # TODO do they want this masking detectors with too high a contribution?
        mantid.MaskDetectorsIf(InputWorkspace=summed_van_ws, InputCalFile=self._grouping_file_path,
                               OutputCalFile=self._cal_file_path, Mode="DeselectIf", Operator="LessEqual", Value=10)

    def _calculate_solid_angle_efficiency_corrections(self, vanadium_ws):
        solid_angle_ws = mantid.SolidAngle(InputWorkspace=vanadium_ws)
        solid_angle_multiplicand = mantid.CreateSingleValuedWorkspace(DataValue=str(100))
        solid_angle_ws = mantid.Multiply(LHSWorkspace=solid_angle_ws, RHSWorkspace=solid_angle_multiplicand)
        common.remove_intermediate_workspace(solid_angle_multiplicand)

        efficiency_ws = mantid.Divide(LHSWorkspace=vanadium_ws, RHSWorkspace=solid_angle_ws)
        efficiency_ws = mantid.ConvertUnits(InputWorkspace=efficiency_ws, Target="Wavelength")
        efficiency_ws = mantid.Integration(InputWorkspace=efficiency_ws,
                                           RangeLower=self._lower_lambda_range, RangeUpper=self._upper_lambda_range)

        corrections_ws = mantid.Multiply(LHSWorkspace=solid_angle_ws, RHSWorkspace=efficiency_ws)
        corrections_divisor_ws = mantid.CreateSingleValuedWorkspace(DataValue=str(100000))
        corrections_ws = mantid.Divide(LHSWorkspace=corrections_ws, RHSWorkspace=corrections_divisor_ws)

        common.remove_intermediate_workspace(corrections_divisor_ws)
        common.remove_intermediate_workspace(solid_angle_ws)
        common.remove_intermediate_workspace(efficiency_ws)

        return corrections_ws

    def _subtract_sample_empty(self, input_sample):
        # TODO move this to be generated by calibration factory so we don't have to use the full fname
        if self._sample_empty is not None:
            empty_sample_path = self.calibration_dir + self._sample_empty
            empty_sample = mantid.Load(Filename=empty_sample_path)
            empty_sample = self._normalise_ws(empty_sample)
            input_sample = mantid.Minus(LHSWorkspace=input_sample, RHSWorkspace=empty_sample)
            common.remove_intermediate_workspace(empty_sample)
        return input_sample

    def _apply_solid_angle_efficiency_corr(self, ws_to_correct, vanadium_number=None, vanadium_path=None):
        assert(vanadium_number or vanadium_path)

        if vanadium_number:
            solid_angle_vanadium_ws = common._load_raw_files(run_number=vanadium_number, instrument=self)
        else:
            solid_angle_vanadium_ws = mantid.Load(Filename=vanadium_path)

        solid_angle_vanadium_ws = self._normalise_ws(solid_angle_vanadium_ws)
        corrections = self._calculate_solid_angle_efficiency_corrections(solid_angle_vanadium_ws)

        corrected_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=corrections)
        common.remove_intermediate_workspace(solid_angle_vanadium_ws)
        common.remove_intermediate_workspace(corrections)
        common.remove_intermediate_workspace(ws_to_correct)
        ws_to_correct = corrected_ws
        return ws_to_correct

    def correct_sample_vanadium(self, focused_ws, index, vanadium_ws=None):
        spectra_name = "sample_ws-" + str(index + 1)
        sample = mantid.CropWorkspace(InputWorkspace=focused_ws, OutputWorkspace=spectra_name,
                                      StartWorkspaceIndex=index, EndWorkspaceIndex=index)

        if vanadium_ws:
            van_rebinned = mantid.RebinToWorkspace(WorkspaceToRebin=vanadium_ws, WorkspaceToMatch=spectra_name)
            mantid.Divide(LHSWorkspace=spectra_name, RHSWorkspace=van_rebinned, OutputWorkspace=spectra_name)
            common.remove_intermediate_workspace(van_rebinned)

        return spectra_name

    def _spline_background(self, focused_vanadium_ws, spline_number, instrument_version=''):

        if spline_number is None:
            spline_number = 100

        mode = "spline"  # TODO support spline modes for all instruments
        extracted_spectra = _extract_bank_spectra(focused_vanadium_ws, self._number_of_banks)

        if mode == "spline":
            output = self._mask_spline_vanadium_ws(vanadium_spectra_list=extracted_spectra,
                                                   spline_coefficient=spline_number)

        for ws in extracted_spectra:
            common.remove_intermediate_workspace(ws)

        return output

    def _generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        absorb_ws = mantid.CloneWorkspace(InputWorkspace=ws_to_match)

        # TODO move all of this into defaults
        cylinder_sample_height = str(4)
        cylinder_sample_radius = str(0.4)

        attenuation_cross_section = str(4.88350)
        scattering_cross_section = str(5.15775)
        sample_number_density = str(0.0718956)

        number_of_slices = str(10)
        number_of_annuli = str(10)
        number_of_wavelength_points = str(100)

        exp_method = "Normal"
        # TODO move all of the above into defaults

        absorb_ws = mantid.CylinderAbsorption(InputWorkspace=absorb_ws,
                                              CylinderSampleHeight=cylinder_sample_height,
                                              CylinderSampleRadius=cylinder_sample_radius,
                                              AttenuationXSection=attenuation_cross_section,
                                              ScatteringXSection=scattering_cross_section,
                                              SampleNumberDensity=sample_number_density,
                                              NumberOfSlices=number_of_slices,
                                              NumberOfAnnuli=number_of_annuli,
                                              NumberOfWavelengthPoints=number_of_wavelength_points,
                                              ExpMethod=exp_method)
        return absorb_ws

    def calculate_focus_binning_params(self, sample):
        calculated_binning_params = []

        for i in range(0, self._number_of_banks):
            sample_data = sample.readX(i)
            starting_bin = _calculate_focus_bin_first_edge(bin_value=sample_data[0], crop_value=self._focus_crop_start)
            ending_bin = _calculate_focus_bin_last_edge(bin_value=sample_data[-1], crop_value=self._focus_crop_end)
            bin_width = _calculate_focus_bin_width(sample_data)

            if bin_width > self._focus_bin_widths[i]:
                bin_width = self._focus_bin_widths[i]

            bank_binning_params = [str(starting_bin), str(bin_width), str(ending_bin)]
            calculated_binning_params.append(bank_binning_params)

        return calculated_binning_params

    def _read_masking_file(self):
        all_banks_masking_list = []
        bank_masking_list = []
        mask_path = self.raw_data_dir + self._masking_file_name

        ignore_line_prefixes = (' ', '\n', '\t', '#')  # Matches whitespace or # symbol

        with open(mask_path) as mask_file:
            for line in mask_file:
                if line.startswith(ignore_line_prefixes):
                    # Push back onto new bank
                    all_banks_masking_list.append(bank_masking_list)
                    bank_masking_list = []
                else:
                    line.rstrip()
                    bank_masking_list.append(line.split())

        return all_banks_masking_list

    def _mask_spline_vanadium_ws(self, vanadium_spectra_list, spline_coefficient):
        masked_workspace = _apply_masking(workspaces_to_mask=vanadium_spectra_list, mask_list=self._read_masking_file())

        index = 0
        output_list = []
        for ws in masked_workspace:
            index += 1
            output_ws_name = "splined_vanadium_ws-" + str(index)
            splined_ws = mantid.SplineBackground(InputWorkspace=ws, OutputWorkspace=output_ws_name,
                                                 WorkspaceIndex=0, NCoeff=spline_coefficient)
            output_list.append(splined_ws)

        return output_list


# Class private implementation


def _extract_bank_spectra(ws_to_split, num_banks):
    spectra_bank_list = []
    for i in range(0, num_banks):
        output_name = "bank-" + str(i + 1)
        # Have to use crop workspace as extract single spectrum struggles with the variable bin widths
        spectra_bank_list.append(mantid.CropWorkspace(InputWorkspace=ws_to_split, OutputWorkspace=output_name,
                                                      StartWorkspaceIndex=i, EndWorkspaceIndex=i))
    return spectra_bank_list


def _apply_masking(workspaces_to_mask, mask_list):

    index = 0
    output_workspaces = []
    for ws in workspaces_to_mask:
        output_workspaces.append(ws)

    for bank_mask_list in mask_list:
        if not bank_mask_list:
            continue

        output_name = "masked_vanadium-" + str(index + 1)

        for mask_params in bank_mask_list:
            out_workspace = mantid.MaskBins(InputWorkspace=output_workspaces[index], OutputWorkspace=output_name,
                                            XMin=mask_params[0], XMax=mask_params[1])
            output_workspaces[index] = out_workspace

        index += 1

    return output_workspaces


def _calculate_focus_bin_first_edge(bin_value, crop_value):
    return bin_value * (1 + crop_value)


def _calculate_focus_bin_last_edge(bin_value, crop_value):
    return bin_value * crop_value


def _calculate_focus_bin_width(bin_data):
    first_val = bin_data[0]
    last_val = bin_data[-1]
    number_of_bins = len(bin_data) - 1

    bin_delta = last_val / first_val
    delta_logarithm = math.log(bin_delta)
    avg_delta = delta_logarithm / number_of_bins

    rebin_width = math.exp(avg_delta) - 1
    rebin_width = -1 * math.fabs(rebin_width)
    return rebin_width
