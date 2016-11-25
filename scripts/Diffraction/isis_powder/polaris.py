from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.abstract_inst import AbstractInst
from isis_powder.polaris_routines import polaris_algs, polaris_output
from isis_powder.routines.RunDetails import RunDetails


class Polaris(AbstractInst):
    # Instrument specific properties
    _masking_file_name = "VanaPeaks.dat"
    _number_of_banks = 5

    def __init__(self, user_name, chopper_on, apply_solid_angle=True,
                 calibration_dir=None, output_dir=None, **kwargs):

        super(Polaris, self).__init__(user_name=user_name, calibration_dir=calibration_dir,
                                      output_dir=output_dir, kwargs=kwargs)
        self._chopper_on = chopper_on
        self._apply_solid_angle = apply_solid_angle

        self._spline_coeff = 100

        # Caches the last dictionary to avoid us having to keep parsing the YAML
        self._run_details_last_run_number = None
        self._run_details_cached_obj = None

    def focus(self, run_number, do_attenuation=True, do_van_normalisation=True):
        return self._focus(run_number=run_number, do_attenuation=do_attenuation,
                           do_van_normalisation=do_van_normalisation)

    def create_calibration_vanadium(self, run_in_range, do_absorb_corrections=True, gen_absorb_correction=False):
        run_details = self.get_run_details(run_number=int(run_in_range))
        return self._create_calibration_vanadium(vanadium_runs=run_details.vanadium,
                                                 empty_runs=run_details.sample_empty,
                                                 do_absorb_corrections=do_absorb_corrections,
                                                 gen_absorb_correction=gen_absorb_correction)

    # Abstract implementation

    def get_create_van_tof_binning(self):
        return self._create_van_calib_tof_binning

    def get_default_group_names(self):
        return self._calibration_grouping_names

    def get_run_details(self, run_number):
        if self._run_details_last_run_number == run_number:
            return self._run_details_cached_obj

        run_details = polaris_algs.get_run_details(chopper_on=self._chopper_on, sac_on=self._apply_solid_angle,
                                                   run_number=run_number, calibration_dir=self.calibration_dir)

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

    def _normalise_ws(self, ws_to_correct, run_details=None):
        normalised_ws = mantid.NormaliseByCurrent(InputWorkspace=ws_to_correct)
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

    def correct_sample_vanadium(self, focused_ws, index, vanadium_ws=None):
        spectra_name = "sample_ws-" + str(index + 1)
        mantid.CropWorkspace(InputWorkspace=focused_ws, OutputWorkspace=spectra_name,
                             StartWorkspaceIndex=index, EndWorkspaceIndex=index)

        if vanadium_ws:
            van_rebinned = mantid.RebinToWorkspace(WorkspaceToRebin=vanadium_ws, WorkspaceToMatch=spectra_name)
            mantid.Divide(LHSWorkspace=spectra_name, RHSWorkspace=van_rebinned, OutputWorkspace=spectra_name)
            common.remove_intermediate_workspace(van_rebinned)

        return spectra_name

    def spline_vanadium_ws(self, focused_vanadium_ws, instrument_version=''):
        extracted_spectra = common.extract_bank_spectra(focused_vanadium_ws, self._number_of_banks)
        mode = "spline"

        masking_file_path = os.path.join(self.calibration_dir, self._masking_file_name)
        output = polaris_algs.process_vanadium_for_focusing(bank_spectra=extracted_spectra,
                                                            spline_number=self._spline_coeff,
                                                            mode=mode, mask_path=masking_file_path)

        for ws in extracted_spectra:
            common.remove_intermediate_workspace(ws)

        return output

    def _generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        return polaris_algs.generate_absorb_corrections(ws_to_match=ws_to_match)

    def calculate_focus_binning_params(self, sample):
        calculated_binning_params = polaris_algs.calculate_focus_binning_params(sample_ws=sample,
                                                                                num_of_banks=self._number_of_banks)
        return calculated_binning_params

    def output_focused_ws(self, processed_spectra, run_details, attenuate=False):
        d_spacing_group, tof_group = polaris_algs.split_into_tof_d_spacing_groups(processed_spectra)
        output_paths = self._generate_out_file_paths(run_details=run_details)

        polaris_output.save_polaris_focused_data(d_spacing_group=d_spacing_group, tof_group=tof_group,
                                                 output_paths=output_paths, run_number=run_details.run_number)

        return d_spacing_group, tof_group
