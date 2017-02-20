from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.abstract_inst import AbstractInst
from isis_powder.polaris_routines import polaris_calib_parser
from isis_powder.routines.RunDetails import RunDetails


class Polaris(AbstractInst):

    _focus_crop_start = 2  # These are used when calculating binning range
    _focus_crop_end = 0.95
    _focus_bin_widths = [-0.0050, -0.0010, -0.0010, -0.0010, -0.00050]

    _calibration_grouping_names = None
    _masking_file_name = "VanaPeaks.dat"

    _number_of_banks = 5

    def __init__(self, user_name, chopper_on, apply_solid_angle=True,
                 calibration_dir=None, output_dir=None, **kwargs):

        super(Polaris, self).__init__(user_name=user_name, calibration_dir=calibration_dir,
                                      output_dir=output_dir, kwargs=kwargs)

        self._chopper_on = chopper_on

        # Caches the last dictionary to avoid us having to keep parsing the YAML
        self._run_details_last_run_number = None
        self._run_details_cached_obj = None

        # Properties set in later calls:
        self._apply_solid_angle = apply_solid_angle

    def focus(self, run_number, do_attenuation=True, do_van_normalisation=True):
        return self._focus(run_number=run_number, do_attenuation=do_attenuation,
                           do_van_normalisation=do_van_normalisation)

    def create_calibration_vanadium(self, run_in_range, do_absorb_corrections=True, gen_absorb_correction=False):
        run_details = self._get_run_details(run_number=int(run_in_range))
        return self._create_calibration_vanadium(vanadium_runs=run_details.vanadium,
                                                 empty_runs=run_details.sample_empty,
                                                 do_absorb_corrections=do_absorb_corrections,
                                                 gen_absorb_correction=gen_absorb_correction)

    # Abstract implementation
    def _get_lambda_range(self):
        return self._lower_lambda_range, self._upper_lambda_range

    def _get_create_van_tof_binning(self):
        return self._create_van_calib_tof_binning

    def _get_default_group_names(self):
        return self._calibration_grouping_names

    def _get_run_details(self, run_number):
        if self._run_details_last_run_number == run_number:
            return self._run_details_cached_obj

        input_run_number_list = common.generate_run_numbers(run_number_string=run_number)
        configuration = polaris_calib_parser.get_calibration_dict(run_number=input_run_number_list[0])

        if self._chopper_on:
            chopper_config = configuration["chopper_on"]
        else:
            chopper_config = configuration["chopper_off"]

        empty_runs = chopper_config["empty_run_numbers"]
        vanadium_runs = chopper_config["vanadium_run_numbers"]
        solid_angle_file_name = self._generate_solid_angle_file_name(vanadium_run_string=vanadium_runs)
        splined_vanadium_name = self._generate_splined_van_name(vanadium_run_string=vanadium_runs)
        cycle = configuration["label"]

        calibration_dir = os.path.join(self.calibration_dir, cycle)
        calibration_full_path = os.path.join(calibration_dir, configuration["offset_file_name"])
        grouping_full_path = os.path.join(calibration_dir, configuration["offset_file_name"])
        solid_angle_file_path = os.path.join(calibration_dir, solid_angle_file_name)
        splined_vanadium = os.path.join(calibration_dir, splined_vanadium_name)

        calibration_details = RunDetails(calibration_path=calibration_full_path, grouping_path=grouping_full_path,
                                         vanadium_runs=vanadium_runs, run_number=run_number)
        calibration_details.label = cycle
        calibration_details.sample_empty = empty_runs
        calibration_details.splined_vanadium = splined_vanadium
        calibration_details.solid_angle_corr = solid_angle_file_path

        # Hold obj in case same run range is requested
        self._run_details_last_run_number = run_number
        self._run_details_cached_obj = calibration_details

        return calibration_details

    @staticmethod
    def _generate_inst_file_name(run_number):
        if isinstance(run_number, list):
            updated_list = ["POL" + str(val) for val in run_number]
            return updated_list
        else:
            return "POL" + str(run_number)

    @staticmethod
    def _get_instrument_alg_save_ranges(instrument=''):
        alg_range = 5
        return alg_range, None

    def _normalise_ws(self, ws_to_correct, run_details=None):
        normalised_ws = mantid.NormaliseByCurrent(InputWorkspace=ws_to_correct)
        return normalised_ws

    def apply_solid_angle_efficiency_corr(self, ws_to_correct, run_details):
        if not self._apply_solid_angle:
            return ws_to_correct

        if not run_details or not os.path.isfile(run_details.solid_angle_corr):
            corrections = self.generate_solid_angle_corrections(run_details)
        else:
            corrections = mantid.Load(Filename=run_details.solid_angle_corr)

        corrected_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=corrections)
        common.remove_intermediate_workspace(corrections)
        common.remove_intermediate_workspace(ws_to_correct)
        ws_to_correct = corrected_ws
        return ws_to_correct

    def generate_solid_angle_corrections(self, run_details):
        solid_angle_vanadium_ws = common.load_current_normalised_ws(run_number_string=run_details.vanadium,
                                                                    instrument=self)

        corrections = _calculate_solid_angle_efficiency_corrections(solid_angle_vanadium_ws)
        mantid.SaveNexusProcessed(InputWorkspace=corrections, Filename=run_details.solid_angle_corr)

        common.remove_intermediate_workspace(solid_angle_vanadium_ws)
        return corrections

    def correct_sample_vanadium(self, focused_ws, index, vanadium_ws=None):
        spectra_name = "sample_ws-" + str(index + 1)
        mantid.CropWorkspace(InputWorkspace=focused_ws, OutputWorkspace=spectra_name,
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
        else:
            raise NotImplementedError("Other vanadium processing methods not yet implemented")

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
            bin_width = self._focus_bin_widths[i]

            bank_binning_params = [str(starting_bin), str(bin_width), str(ending_bin)]
            calculated_binning_params.append(bank_binning_params)

        return calculated_binning_params

    def _process_focus_output(self, processed_spectra, run_details, attenuate=False):
        d_spacing_group, tof_group = _create_d_spacing_tof_output(processed_spectra)
        output_paths = self._generate_out_file_paths(run_details=run_details)

        mantid.SaveGSS(InputWorkspace=tof_group, Filename=output_paths["gss_filename"], SplitFiles=False, Append=False)
        mantid.SaveNexusProcessed(InputWorkspace=tof_group, Filename=output_paths["nxs_filename"], Append=False)

        _save_xye(ws_group=d_spacing_group, ws_units="d_spacing", run_number=run_details.run_number,
                  output_folder=output_paths["output_folder"])
        _save_xye(ws_group=tof_group, ws_units="TOF", run_number=run_details.run_number,
                  output_folder=output_paths["output_folder"])

        return d_spacing_group, tof_group

    def _read_masking_file(self):
        all_banks_masking_list = []
        bank_masking_list = []
        mask_path = os.path.join(self.calibration_dir, self._masking_file_name)

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

    def _generate_solid_angle_file_name(self, vanadium_run_string):
        if self._chopper_on:
            return "SAC_chopperOn_" + vanadium_run_string + ".nxs"
        else:
            return "SAC_chopperOff_" + vanadium_run_string + ".nxs"

    def _generate_splined_van_name(self, vanadium_run_string):
        output_string = "SVan_" + str(vanadium_run_string) + "_chopper"
        if self._chopper_on:
            output_string += "On"
        else:
            output_string += "Off"

        if self._apply_solid_angle:
            output_string += "_SAC"
        else:
            output_string += "_noSAC"

        return output_string


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


def _create_d_spacing_tof_output(processed_spectra):
    name_index = 1
    d_spacing_output = []
    tof_output = []
    for ws in processed_spectra:
        d_spacing_out_name = "ResultD-" + str(name_index)
        tof_out_name = "ResultTOF-" + str(name_index)
        name_index += 1
        # Rename d-spacing workspaces
        d_spacing_output.append(mantid.CloneWorkspace(InputWorkspace=ws, OutputWorkspace=d_spacing_out_name))
        # Convert to TOF
        tof_output.append(mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=tof_out_name, Target="TOF"))

    # Group the outputs
    d_spacing_group_name = "Results-D-Grp"
    d_spacing_group = mantid.GroupWorkspaces(InputWorkspaces=d_spacing_output, OutputWorkspace=d_spacing_group_name)
    tof_group_name = "Results-TOF-Grp"
    tof_group = mantid.GroupWorkspaces(InputWorkspaces=tof_output, OutputWorkspace=tof_group_name)

    return d_spacing_group, tof_group


def _calculate_solid_angle_efficiency_corrections(vanadium_ws):
    solid_angle_ws = mantid.SolidAngle(InputWorkspace=vanadium_ws)
    solid_angle_multiplicand = mantid.CreateSingleValuedWorkspace(DataValue=str(100))
    solid_angle_ws = mantid.Multiply(LHSWorkspace=solid_angle_ws, RHSWorkspace=solid_angle_multiplicand)
    common.remove_intermediate_workspace(solid_angle_multiplicand)

    efficiency_ws = mantid.Divide(LHSWorkspace=vanadium_ws, RHSWorkspace=solid_angle_ws)
    efficiency_ws = mantid.ConvertUnits(InputWorkspace=efficiency_ws, Target="Wavelength")
    efficiency_ws = mantid.Integration(InputWorkspace=efficiency_ws)

    corrections_ws = mantid.Multiply(LHSWorkspace=solid_angle_ws, RHSWorkspace=efficiency_ws)
    corrections_divisor_ws = mantid.CreateSingleValuedWorkspace(DataValue=str(100000))
    corrections_ws = mantid.Divide(LHSWorkspace=corrections_ws, RHSWorkspace=corrections_divisor_ws)

    common.remove_intermediate_workspace(corrections_divisor_ws)
    common.remove_intermediate_workspace(solid_angle_ws)
    common.remove_intermediate_workspace(efficiency_ws)

    return corrections_ws


def _save_xye(ws_group, ws_units, run_number, output_folder):
    bank_index = 1
    for ws in ws_group:
        outfile_name = str(run_number) + "-b_" + str(bank_index) + "-" + ws_units + ".dat"
        bank_index += 1
        full_file_path = os.path.join(output_folder, outfile_name)

        mantid.SaveFocusedXYE(InputWorkspace=ws, Filename=full_file_path, SplitFiles=False, IncludeHeader=False)
