# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

from isis_powder.abstract_inst import AbstractInst
from isis_powder.gem_routines import gem_advanced_config, gem_algs, gem_param_mapping, gem_output, gem_calibration_algs
from isis_powder.routines import absorb_corrections, common, instrument_settings, common_output


class Gem(AbstractInst):
    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=gem_param_mapping.attr_mapping, adv_conf_dict=gem_advanced_config.get_all_adv_variables(), kwargs=kwargs
        )

        super(Gem, self).__init__(
            user_name=self._inst_settings.user_name,
            calibration_dir=self._inst_settings.calibration_dir,
            output_dir=self._inst_settings.output_dir,
            inst_prefix="GEM",
        )

        self._cached_run_details = {}
        self._sample_details = None

    # Public API

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        self._switch_texture_mode_specific_inst_settings(kwargs.get("texture_mode"))

        return self._focus(
            run_number_string=self._inst_settings.run_number,
            do_van_normalisation=self._inst_settings.do_van_norm,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections,
        )

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        self._switch_texture_mode_specific_inst_settings(kwargs.get("texture_mode"))

        return self._create_vanadium(
            run_number_string=self._inst_settings.run_in_range, do_absorb_corrections=self._inst_settings.do_absorb_corrections
        )

    def create_cal(self, **kwargs):
        self._switch_texture_mode_specific_inst_settings(kwargs.get("texture_mode"))
        self._inst_settings.update_attributes(kwargs=kwargs)
        self._inst_settings.update_attributes(advanced_config=gem_advanced_config.get_calibration_variables())
        run_details = self._get_run_details(self._inst_settings.run_number)

        cross_correlate_params = {
            "ReferenceSpectra": self._inst_settings.reference_spectra,
            "WorkspaceIndexMin": self._inst_settings.cross_corr_ws_min,
            "WorkspaceIndexMax": self._inst_settings.cross_corr_ws_max,
            "XMin": self._inst_settings.cross_corr_x_min,
            "XMax": self._inst_settings.cross_corr_x_max,
        }
        get_detector_offsets_params = {
            "DReference": self._inst_settings.d_reference,
            "Step": self._inst_settings.get_det_offsets_step,
            "XMin": self._inst_settings.get_det_offsets_x_min,
            "XMax": self._inst_settings.get_det_offsets_x_max,
        }
        if self._inst_settings.cal_adjust:
            return gem_calibration_algs.adjust_calibration(
                calibration_runs=self._inst_settings.run_number,
                instrument=self,
                offset_file_name=run_details.offset_file_path,
                grouping_file_name=run_details.grouping_file_path,
                calibration_dir=self._inst_settings.calibration_dir,
                rebin_1_params=self._inst_settings.cal_rebin_1,
                rebin_2_params=self._inst_settings.cal_rebin_2,
                cross_correlate_params=cross_correlate_params,
                get_det_offset_params=get_detector_offsets_params,
                original_cal=self._inst_settings.cal_adjust,
            )
        return gem_calibration_algs.create_calibration(
            calibration_runs=self._inst_settings.run_number,
            instrument=self,
            offset_file_name=run_details.offset_file_path,
            grouping_file_name=run_details.grouping_file_path,
            calibration_dir=self._inst_settings.calibration_dir,
            rebin_1_params=self._inst_settings.cal_rebin_1,
            rebin_2_params=self._inst_settings.cal_rebin_2,
            cross_correlate_params=cross_correlate_params,
            get_det_offset_params=get_detector_offsets_params,
        )

    def set_sample_details(self, **kwargs):
        kwarg_name = "sample"
        sample_details_obj = common.dictionary_key_helper(
            dictionary=kwargs,
            key=kwarg_name,
            exception_msg="The argument containing sample details was not found. Please set the following argument: " + kwarg_name,
        )
        self._sample_details = sample_details_obj

    def should_subtract_empty_inst(self):
        return self._inst_settings.subtract_empty_inst

    # Private methods

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string, self._inst_settings.file_extension)
        if run_number_string_key in self._cached_run_details:
            return self._cached_run_details[run_number_string_key]

        self._cached_run_details[run_number_string_key] = gem_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium
        )
        return self._cached_run_details[run_number_string_key]

    def _generate_output_file_name(self, run_number_string):
        return self._generate_input_file_name(run_number_string)

    def _generate_out_file_paths(self, run_details):
        out_file_names = super(Gem, self)._generate_out_file_paths(run_details)
        nxs_filename = out_file_names["nxs_filename"]
        filename_stub = ".".join(nxs_filename.split(".")[:-1])

        if self._inst_settings.save_maud:
            maud_filename = filename_stub + "_MAUD.gem"
            out_file_names["maud_filename"] = maud_filename

        if hasattr(self._inst_settings, "texture_mode") and self._inst_settings.texture_mode:
            angles_filename = filename_stub + "_grouping.new"
            out_file_names["angles_filename"] = angles_filename

        if self._inst_settings.save_maud_calib:
            maud_calib_filename = filename_stub + ".maud"
            out_file_names["maud_calib_filename"] = maud_calib_filename

        if self._inst_settings.save_gda:
            gda_filename = filename_stub + ".gda"
            out_file_names["gda_filename"] = gda_filename

        return out_file_names

    def _output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        """
        Takes a list of focused workspace banks and saves them out in an instrument appropriate format.
        :param processed_spectra: The list of workspace banks to save out
        :param run_details: The run details associated with this run
        :param output_mode: Optional - Sets additional saving/grouping behaviour depending on the instrument
        :return: d-spacing and TOF groups of the processed output workspaces
        """
        if self._inst_settings.save_all:
            d_spacing_group, tof_group = super(Gem, self)._output_focused_ws(
                processed_spectra=processed_spectra, run_details=run_details, output_mode=output_mode
            )
        else:
            d_spacing_group, tof_group = common_output.split_into_tof_d_spacing_groups(
                run_details=run_details, processed_spectra=processed_spectra
            )

        if self._is_vanadium:
            return d_spacing_group, tof_group

        output_paths = self._generate_out_file_paths(run_details=run_details)
        if "maud_filename" in output_paths:
            gem_output.save_maud(d_spacing_group, output_paths["maud_filename"])

        if "angles_filename" in output_paths:
            gem_output.save_angles(d_spacing_group, output_paths["angles_filename"])
        self._save_gsas_req_files(d_spacing_group, output_paths)

        return d_spacing_group, tof_group

    def _save_gsas_req_files(self, d_spacing_group, output_paths):
        gsas_calib_file_path = os.path.join(self._inst_settings.calibration_dir, self._inst_settings.gsas_calib_filename)
        raise_warning = False
        if not os.path.exists(gsas_calib_file_path):
            raise_warning = True
        if "maud_calib_filename" in output_paths:
            gem_output.save_maud_calib(
                d_spacing_group=d_spacing_group,
                output_path=output_paths["maud_calib_filename"],
                gsas_calib_filename=gsas_calib_file_path,
                grouping_scheme=self._inst_settings.maud_grouping_scheme,
                raise_warning=raise_warning,
            )

        if "gda_filename" in output_paths:
            gem_output.save_gda(
                d_spacing_group=d_spacing_group,
                output_path=output_paths["gda_filename"],
                gsas_calib_filename=gsas_calib_file_path,
                grouping_scheme=self._inst_settings.maud_grouping_scheme,
                raise_warning=raise_warning,
            )

    @staticmethod
    def _generate_input_file_name(run_number, file_ext=""):
        return _gem_generate_inst_name(run_number=run_number) + file_ext

    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        if self._is_vanadium:
            return gem_algs.calculate_van_absorb_corrections(
                ws_to_correct=ws_to_correct,
                multiple_scattering=self._inst_settings.multiple_scattering,
                is_vanadium=self._is_vanadium,
                msevents=self._inst_settings.mayers_mult_scat_events,
            )
        else:
            self._check_sample_details()
            return absorb_corrections.run_cylinder_absorb_corrections(
                ws_to_correct=ws_to_correct,
                multiple_scattering=self._inst_settings.multiple_scattering,
                sample_details_obj=self._sample_details,
                is_vanadium=self._is_vanadium,
                msevents=self._inst_settings.mayers_mult_scat_events,
            )

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.focused_cropping_values)

    def _crop_raw_to_expected_tof_range(self, ws_to_crop):
        raw_cropping_values = self._inst_settings.raw_tof_cropping_values
        return common.crop_in_tof(ws_to_crop, raw_cropping_values[0], raw_cropping_values[1])

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return common.crop_banks_using_crop_list(van_ws_to_crop, self._inst_settings.vanadium_cropping_values)

    def _get_input_batching_mode(self):
        return self._inst_settings.input_batching

    def _get_unit_to_keep(self):
        return self._inst_settings.unit_to_keep

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        """
        GEM uses a Vanadium-Niobium mix and doesn't need to strip any Vanadium Bragg peaks
        before splining
        """
        return common.spline_vanadium_workspaces(
            focused_vanadium_spectra=focused_vanadium_banks, spline_coefficient=self._inst_settings.spline_coeff
        )

    def _switch_texture_mode_specific_inst_settings(self, mode):
        if mode is None and hasattr(self._inst_settings, "texture_mode"):
            mode = self._inst_settings.texture_mode
        save_all = not hasattr(self._inst_settings, "save_all")
        adv_config_variables = gem_advanced_config.get_mode_specific_variables(mode, save_all)
        if self.should_subtract_empty_inst() is None:
            adv_config_variables.update({"subtract_empty_instrument": True})
        self._inst_settings.update_attributes(advanced_config=adv_config_variables, suppress_warnings=True)


def _gem_generate_inst_name(run_number):
    if isinstance(run_number, list):
        # Use recursion on lists
        updated_list = []
        for run in run_number:
            updated_list.append(_gem_generate_inst_name(run))
        return updated_list
    else:
        # Individual entry
        return "GEM" + str(run_number)
