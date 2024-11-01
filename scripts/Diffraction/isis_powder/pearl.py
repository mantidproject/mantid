# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from contextlib import contextmanager
import os

import mantid.simpleapi as mantid
from mantid.kernel import logger

from isis_powder.routines import common, instrument_settings
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import pearl_advanced_config, pearl_algs, pearl_calibration_algs, pearl_output, pearl_param_mapping

import copy


class Pearl(AbstractInst):
    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=pearl_param_mapping.attr_mapping, adv_conf_dict=pearl_advanced_config.get_all_adv_variables(), kwargs=kwargs
        )
        self._default_inst_settings = copy.deepcopy(self._inst_settings)

        super(Pearl, self).__init__(
            user_name=self._inst_settings.user_name,
            calibration_dir=self._inst_settings.calibration_dir,
            output_dir=self._inst_settings.output_dir,
            inst_prefix="PEARL",
        )

        self._cached_run_details = {}

    def focus(self, **kwargs):
        with self._apply_temporary_inst_settings(kwargs, kwargs.get("run_number")):
            if self._inst_settings.perform_atten:
                if not hasattr(self._inst_settings, "attenuation_file"):
                    raise RuntimeError("Attenuation cannot be applied because attenuation_file not specified")
            return self._focus(
                run_number_string=self._inst_settings.run_number,
                do_absorb_corrections=self._inst_settings.absorb_corrections,
                do_van_normalisation=self._inst_settings.van_norm,
            )

    def create_vanadium(self, **kwargs):
        kwargs["perform_attenuation"] = None  # Hard code this off as we do not need an attenuation file

        with self._apply_temporary_inst_settings(kwargs, kwargs.get("run_in_cycle")):
            if str(self._inst_settings.tt_mode).lower() == "all":
                for new_tt_mode in ["tt35", "tt70", "tt88"]:
                    self._inst_settings.tt_mode = new_tt_mode
                    self._run_create_vanadium()
            else:
                self._run_create_vanadium()

    def create_cal(self, **kwargs):
        with self._apply_temporary_inst_settings(kwargs, kwargs.get("run_number")):
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
            output_file_paths = self._generate_out_file_paths(run_details)
            return pearl_calibration_algs.create_calibration(
                calibration_runs=self._inst_settings.run_number,
                instrument=self,
                offset_file_name=run_details.offset_file_path,
                grouping_file_name=run_details.grouping_file_path,
                calibration_dir=self._inst_settings.calibration_dir,
                rebin_1_params=self._inst_settings.cal_rebin_1,
                rebin_2_params=self._inst_settings.cal_rebin_2,
                cross_correlate_params=cross_correlate_params,
                get_det_offset_params=get_detector_offsets_params,
                output_name=output_file_paths["output_name"] + "_grouped",
            )

    def should_subtract_empty_inst(self):
        return self._inst_settings.subtract_empty_inst

    def _generate_out_file_paths(self, run_details):
        output_file_paths = super()._generate_out_file_paths(run_details)
        file_ext = run_details.file_extension
        if file_ext and self._inst_settings.incl_file_ext_in_wsname:
            output_file_paths["output_name"] = output_file_paths["output_name"] + file_ext.replace(".", "_")
        return output_file_paths

    def get_trans_module_indices(self):
        default_imods = list(range(9))  # all modules 1-9 in transverse banks (tth~90 deg)
        default_mod_nums_str = "1-9"
        if self._inst_settings.trans_mod_nums and self._inst_settings.focus_mode == "trans_subset":
            mod_nums = common.generate_run_numbers(run_number_string=self._inst_settings.trans_mod_nums)
            imods = [int(mod_num - 1) for mod_num in set(mod_nums) if 0 < mod_num < 10]  # remove invalid/duplicates
            if len(imods) < len(mod_nums):
                # catches case where no indices in correct range as len(mod_nums) > 1 in this branch
                logger.warning("Invalid or duplicate modules in trans_mod_nums - using all modules 1-9")
                return default_imods, default_mod_nums_str
            return imods, mod_nums
        else:
            return default_imods, default_mod_nums_str

    def _get_output_formats(self, output_directory, xye_files_directory):
        return {
            "nxs_filename": output_directory,
            "gss_filename": os.path.join(output_directory, "GSAS"),
            "tof_xye_filename": os.path.join(xye_files_directory, "ToF"),
            "dspacing_xye_filename": os.path.join(xye_files_directory, "dSpacing"),
        }

    @contextmanager
    def _apply_temporary_inst_settings(self, kwargs, run):
        self._inst_settings.update_attributes(kwargs=kwargs)
        self._switch_long_mode_inst_settings(self._inst_settings.long_mode)

        yield
        # reset instrument settings
        self._inst_settings = copy.deepcopy(self._default_inst_settings)

    def _run_create_vanadium(self):
        # Provides a minimal wrapper so if we have tt_mode 'all' we can loop round
        return self._create_vanadium(
            run_number_string=self._inst_settings.run_in_range, do_absorb_corrections=self._inst_settings.absorb_corrections
        )

    def _get_run_details(self, run_number_string):
        tt_mode_string = self._inst_settings.tt_mode
        if self._inst_settings.tt_mode == "custom":
            grouping_file_name = pearl_algs._pearl_get_tt_grouping_file_name(self._inst_settings)
            tt_mode_string += os.path.splitext(os.path.basename(grouping_file_name))[0]
        run_number_string_key = self._generate_run_details_fingerprint(
            run_number_string, self._inst_settings.file_extension, tt_mode_string, self._inst_settings.long_mode
        )
        if run_number_string_key in self._cached_run_details:
            return self._cached_run_details[run_number_string_key]

        self._cached_run_details[run_number_string_key] = pearl_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium
        )
        return self._cached_run_details[run_number_string_key]

    def _add_formatting_options(self, format_options):
        """
        Add any instrument-specific format options to the given
        list
        :param format_options: A dictionary of string format keys mapped to their expansions
        :return: format_options as it is passed in
        """
        inst = self._inst_settings
        format_options.update({"tt_mode": str(inst.tt_mode), "_long_mode": "_long" if inst.long_mode else ""})
        return format_options

    def _normalise_ws_current(self, ws_to_correct):
        monitor_spectra = self._inst_settings.monitor_spec_no

        monitor_ws = common.extract_single_spectrum(ws_to_process=ws_to_correct, spectrum_number_to_extract=monitor_spectra)

        normalised_ws = pearl_algs.normalise_ws_current(
            ws_to_correct=ws_to_correct,
            monitor_ws=monitor_ws,
            spline_coeff=self._inst_settings.monitor_spline,
            integration_range=self._inst_settings.monitor_integration_range,
            lambda_values=self._inst_settings.monitor_lambda,
            ex_regions=self._inst_settings.monitor_mask_regions,
        )
        common.remove_intermediate_workspace(monitor_ws)
        return normalised_ws

    def _get_current_tt_mode(self):
        return self._inst_settings.tt_mode

    def _spline_vanadium_ws(self, focused_vanadium_spectra):
        focused_vanadium_spectra = pearl_algs.strip_bragg_peaks(focused_vanadium_spectra)
        splined_list = common.spline_workspaces(
            focused_vanadium_spectra=focused_vanadium_spectra, num_splines=self._inst_settings.spline_coefficient
        )
        # Ensure the name is unique if we are in tt_mode all
        new_workspace_names = []
        for ws in splined_list:
            new_name = ws.name() + "_" + self._inst_settings.tt_mode
            new_workspace_names.append(mantid.RenameWorkspace(InputWorkspace=ws, OutputWorkspace=new_name))

        return new_workspace_names

    def _get_instrument_bin_widths(self):
        if self._inst_settings.tt_mode == "custom":
            return self._inst_settings.custom_focused_bin_widths
        else:
            return self._inst_settings.focused_bin_widths

    def _output_focused_ws(self, processed_spectra, run_details, output_mode=None):
        if not output_mode:
            output_mode = self._inst_settings.focus_mode

        attenuation_path = None
        if self._inst_settings.perform_atten:
            name_key = "name"
            path_key = "path"
            if isinstance(self._inst_settings.attenuation_files, str):
                self._inst_settings.attenuation_files = eval(self._inst_settings.attenuation_files)
            atten_file_found = False
            for atten_file in self._inst_settings.attenuation_files:
                if any(required_key not in atten_file for required_key in [name_key, path_key]):
                    logger.warning(
                        "A dictionary in attenuation_files has been ignored because "
                        f"it doesn't contain both {name_key} and {path_key} entries"
                    )
                elif atten_file[name_key] == self._inst_settings.attenuation_file:
                    if atten_file_found:
                        raise RuntimeError(f"Duplicate name {self._inst_settings.attenuation_file} found in attenuation_files")
                    attenuation_path = atten_file[path_key]
                    atten_file_found = True
            if attenuation_path is None:
                raise RuntimeError(f"Unknown attenuation_file {self._inst_settings.attenuation_file} specified for attenuation")

        output_spectra = pearl_output.generate_and_save_focus_output(
            self,
            processed_spectra=processed_spectra,
            run_details=run_details,
            focus_mode=output_mode,
            attenuation_filepath=attenuation_path,
        )
        group_name = self._generate_out_file_paths(run_details)["output_name"] + "-d"
        grouped_d_spacing = mantid.GroupWorkspaces(InputWorkspaces=output_spectra, OutputWorkspace=group_name)
        return grouped_d_spacing, None

    def _crop_banks_to_user_tof(self, focused_banks):
        if self._inst_settings.tt_mode == "custom":
            return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.custom_tof_cropping_values)
        else:
            return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.tof_cropping_values)

    def _crop_raw_to_expected_tof_range(self, ws_to_crop):
        out_ws = common.crop_in_tof(
            ws_to_crop=ws_to_crop, x_min=self._inst_settings.raw_data_crop_vals[0], x_max=self._inst_settings.raw_data_crop_vals[-1]
        )
        return out_ws

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        cropped_ws = common.crop_in_tof(
            ws_to_crop=van_ws_to_crop, x_min=self._inst_settings.van_tof_cropping[0], x_max=self._inst_settings.van_tof_cropping[-1]
        )
        return cropped_ws

    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        if self._inst_settings.gen_absorb:
            absorb_file_name = self._inst_settings.absorb_out_file
            if not absorb_file_name:
                raise RuntimeError('"absorb_corrections_out_filename" must be supplied when generating absorption ' "corrections")
            absorb_corrections = pearl_algs.generate_vanadium_absorb_corrections(van_ws=ws_to_correct, output_filename=absorb_file_name)
        else:
            absorb_corrections = None

        return pearl_algs.apply_vanadium_absorb_corrections(van_ws=ws_to_correct, run_details=run_details, absorb_ws=absorb_corrections)

    def _switch_long_mode_inst_settings(self, long_mode_on):
        self._inst_settings.update_attributes(advanced_config=pearl_advanced_config.get_long_mode_dict(long_mode_on))
        if long_mode_on:
            self._inst_settings.update_attributes(kwargs={"perform_attenuation": False})
