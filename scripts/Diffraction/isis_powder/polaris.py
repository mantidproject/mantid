from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.abstract_inst import AbstractInst
from isis_powder.polaris_routines import polaris_algs, polaris_config_parser, polaris_output


class Polaris(AbstractInst):
    # Instrument specific properties
    _masking_file_name = "VanaPeaks.dat"
    _number_of_banks = 5

    def __init__(self, chopper_on, config_file=None, **kwargs):

        _set_kwargs_from_basic_config_file(config_path=config_file, kwargs=kwargs)

        # Have to pass in everything through named types until abstract_inst takes kwargs
        super(Polaris, self).__init__(user_name=kwargs["user_name"], calibration_dir=kwargs["calibration_directory"],
                                      output_dir=kwargs["output_directory"], kwargs=kwargs)

        self._chopper_on = chopper_on
        self._apply_solid_angle = kwargs["apply_solid_angle"]
        self._calibration_mapping_path = kwargs["calibration_mapping_file"]

        self._spline_coeff = 100  # TODO move this out into advanced config

        # Hold the last dictionary later to avoid us having to keep parsing the YAML
        self._run_details_last_run_number = None
        self._run_details_cached_obj = None

        self._ads_workaround = 0

    def focus(self, run_number, input_mode, do_attenuation=True, do_van_normalisation=True):
        return self._focus(run_number=run_number, input_batching=input_mode, do_attenuation=do_attenuation,
                           do_van_normalisation=do_van_normalisation)

    def create_calibration_vanadium(self, run_in_range, do_absorb_corrections=True, gen_absorb_correction=False):
        run_details = self.get_run_details(run_number=int(run_in_range))
        return self._create_calibration_vanadium(vanadium_runs=run_details.vanadium,
                                                 empty_runs=run_details.sample_empty,
                                                 do_absorb_corrections=do_absorb_corrections,
                                                 gen_absorb_correction=gen_absorb_correction)

    def get_default_group_names(self):
        return self._calibration_grouping_names

    # Abstract implementation

    def get_run_details(self, run_number):
        if self._run_details_last_run_number == run_number:
            return self._run_details_cached_obj

        run_details = polaris_algs.get_run_details(chopper_on=self._chopper_on, sac_on=self._apply_solid_angle,
                                                   run_number_string=run_number, calibration_dir=self._calibration_dir,
                                                   mapping_path=self._calibration_mapping_path)

        # Hold obj in case same run range is requested
        self._run_details_last_run_number = run_number
        self._run_details_cached_obj = run_details

        return run_details

    @staticmethod
    def generate_inst_file_name(run_number):
        if isinstance(run_number, list):
            updated_list = ["POL" + str(val) for val in run_number]
            return updated_list
        else:
            return "POL" + str(run_number)

    def get_num_of_banks(self, instrument_version=''):
        return self._number_of_banks

    def normalise_ws(self, ws_to_correct, run_details=None):
        normalised_ws = mantid.NormaliseByCurrent(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct)
        return normalised_ws

    def apply_solid_angle_efficiency_corr(self, ws_to_correct, run_details):
        if not self._apply_solid_angle:
            return ws_to_correct

        if not run_details or not os.path.isfile(run_details.solid_angle_corr):
            corrections = \
                polaris_algs.generate_solid_angle_corrections(run_details=run_details, instrument=self)
        else:
            corrections = mantid.Load(Filename=run_details.solid_angle_corr)

        corrected_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=corrections)
        common.remove_intermediate_workspace(corrections)
        common.remove_intermediate_workspace(ws_to_correct)
        ws_to_correct = corrected_ws
        return ws_to_correct

    def correct_sample_vanadium(self, focus_spectra, vanadium_spectra=None):
        spectra_name = "sample_ws-" + str(self._ads_workaround + 1)
        self._ads_workaround += 1

        if vanadium_spectra:
            van_rebinned = mantid.RebinToWorkspace(WorkspaceToRebin=vanadium_spectra, WorkspaceToMatch=focus_spectra)
            mantid.Divide(LHSWorkspace=focus_spectra, RHSWorkspace=van_rebinned, OutputWorkspace=spectra_name)
            common.remove_intermediate_workspace(van_rebinned)

        return spectra_name

    def spline_vanadium_ws(self, focused_vanadium_spectra, instrument_version=''):
        mode = "spline"

        masking_file_path = os.path.join(self.calibration_dir, self._masking_file_name)
        output = polaris_algs.process_vanadium_for_focusing(bank_spectra=focused_vanadium_spectra,
                                                            spline_number=self._spline_coeff,
                                                            mode=mode, mask_path=masking_file_path)

        return output

    def generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        return polaris_algs.generate_absorb_corrections(ws_to_match=ws_to_match)

    def vanadium_calibration_rebinning(self, vanadium_ws):
        common.crop_in_tof(ws_to_rebin=vanadium_ws, x_max=19900, is_mixed_binning=True)
        # TODO find out maximum TOF value for POLARIS

    def extract_and_crop_spectra(self, focused_ws):
        ws_spectra = common.extract_ws_spectra(ws_to_split=focused_ws)
        ws_spectra = common.crop_in_tof(ws_to_rebin=ws_spectra, x_min=800, x_max=20000)
        return ws_spectra

    def calculate_focus_binning_params(self, sample):
        calculated_binning_params = polaris_algs.calculate_focus_binning_params(sample_ws=sample,
                                                                                num_of_banks=self._number_of_banks)
        return calculated_binning_params

    def output_focused_ws(self, processed_spectra, run_details, output_mode=None, attenuate=False):
        d_spacing_group, tof_group = polaris_algs.split_into_tof_d_spacing_groups(processed_spectra)
        output_paths = self._generate_out_file_paths(run_details=run_details)

        polaris_output.save_polaris_focused_data(d_spacing_group=d_spacing_group, tof_group=tof_group,
                                                 output_paths=output_paths, run_number=run_details.run_number)

        return d_spacing_group


def _set_kwargs_from_basic_config_file(config_path, kwargs):
    if config_path:
        basic_config_dict = polaris_config_parser.get_basic_config(file_path=config_path)
    else:
        # Create an empty dictionary so we still get error checking below and nicer error messages
        basic_config_dict = {}

    # Set any unset properties:
    keys = ["user_name", "calibration_directory", "output_directory", "apply_solid_angle", "calibration_mapping_file"]
    for key in keys:
        _set_from_config_kwargs_helper(config_dictionary=basic_config_dict, kwargs=kwargs, key=key)


def _set_from_config_kwargs_helper(config_dictionary, kwargs, key):
    error_first = "Setting with name: '"
    error_last = "' was not passed in the call or set in the basic config."
    kwarg_value = kwargs.get(key, None)
    if not kwarg_value:
        # Only try to parse it if it wasn't passed
        value = common.dictionary_key_helper(dictionary=config_dictionary, key=key, throws=True,
                                             exception_msg=(error_first + key + error_last))
        kwargs[key] = value
