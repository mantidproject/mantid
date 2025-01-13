# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

from isis_powder.routines import absorb_corrections, common, instrument_settings
from isis_powder.abstract_inst import AbstractInst
from isis_powder.polaris_routines import polaris_advanced_config, polaris_algs, polaris_param_mapping
from mantid.kernel import logger
from mantid.api import MatrixWorkspace, WorkspaceGroup


class Polaris(AbstractInst):
    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=polaris_param_mapping.attr_mapping, adv_conf_dict=polaris_advanced_config.get_all_adv_variables(), kwargs=kwargs
        )

        super(Polaris, self).__init__(
            user_name=self._inst_settings.user_name,
            calibration_dir=self._inst_settings.calibration_dir,
            output_dir=self._inst_settings.output_dir,
            inst_prefix="POLARIS",
        )

        # Hold the last dictionary later to avoid us having to keep parsing the YAML
        self._run_details_cached_obj = {}
        self._sample_details = None

    # Public API

    def focus(self, **kwargs):
        self._switch_mode_specific_inst_settings(kwargs.get("mode"))
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(
            run_number_string=self._inst_settings.run_number,
            do_van_normalisation=self._inst_settings.do_van_normalisation,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections,
            sample_details=self._sample_details,
            empty_can_subtraction_method=self._inst_settings.empty_can_subtraction_method,
            paalman_pings_events_per_point=self._inst_settings.paalman_pings_events_per_point,
        )

    def create_vanadium(self, **kwargs):
        self._switch_mode_specific_inst_settings(kwargs.get("mode"))
        self._inst_settings.update_attributes(kwargs=kwargs)
        if not self._inst_settings.multiple_scattering or not self._inst_settings.do_absorb_corrections:
            raise ValueError("You must set multiple_scattering=True and do_absorb_corrections=True when creating the vanadium run.")

        per_detector = False
        if self._inst_settings.per_detector_vanadium:
            per_detector = bool(self._inst_settings.per_detector_vanadium)

        vanadium_d = self._create_vanadium(
            run_number_string=self._inst_settings.run_in_range,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections,
            per_detector=per_detector,
        )
        self.ensure_per_detector_and_vanadium_output_are_in_sync(vanadium_d, per_detector)

        run_details = self._get_run_details(run_number_string=self._inst_settings.run_in_range)
        common.save_unsplined_vanadium(vanadium_ws=vanadium_d, output_path=run_details.unsplined_vanadium_file_path)
        return vanadium_d

    def ensure_per_detector_and_vanadium_output_are_in_sync(self, vanadium_d, per_detector):
        correct_per_detector_condition = isinstance(vanadium_d, MatrixWorkspace) and per_detector
        correct_per_bank_condition = isinstance(vanadium_d, WorkspaceGroup) and not per_detector
        if not correct_per_detector_condition and not correct_per_bank_condition:
            raise ValueError(
                f"The output from polaris._create_vanadium must be a WorkspaceGroup in the per_bank "
                f"routine (default) and must be MatrixWorkspace in the per_detector routine. In this case,"
                f"the output was type {type(vanadium_d)} and in the "
                f"{'per_detector' if per_detector else 'per_bank'} routine"
            )

    def create_total_scattering_pdf(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        if not hasattr(self._inst_settings, "pdf_type") or self._inst_settings.pdf_type not in ["G(r)", "g(r)", "RDF(r)", "G_k(r)"]:
            self._inst_settings.pdf_type = "G(r)"
            logger.warning("PDF type not specified or is invalid, defaulting to G(r)")
        if not hasattr(self._inst_settings, "placzek_order") or self._inst_settings.placzek_order not in [1, 2]:
            self._inst_settings.placzek_order = 1
            logger.warning("Placzek correction order not specified or is invalid, defaulting to 1")
        # Generate pdf
        run_details = self._get_run_details(self._inst_settings.run_number)
        focus_file_path = self._generate_out_file_paths(run_details)["nxs_filename"]
        cal_file_name = os.path.join(self._inst_settings.calibration_dir, self._inst_settings.grouping_file_name)
        if self._inst_settings.sample_temp is None:
            sample_temperature = None
        else:
            sample_temperature = str(self._inst_settings.sample_temp)
        pdf_output = polaris_algs.generate_ts_pdf(
            run_number=self._inst_settings.run_number,
            focus_file_path=focus_file_path,
            placzek_order=self._inst_settings.placzek_order,
            sample_temp=sample_temperature,
            merge_banks=self._inst_settings.merge_banks,
            q_lims=self._inst_settings.q_lims,
            cal_file_name=cal_file_name,
            sample_details=self._sample_details,
            delta_r=self._inst_settings.delta_r,
            delta_q=self._inst_settings.delta_q,
            pdf_type=self._inst_settings.pdf_type,
            lorch_filter=self._inst_settings.lorch_filter,
            freq_params=self._inst_settings.freq_params,
            per_detector=self._inst_settings.per_detector_vanadium,
            debug=self._inst_settings.debug,
            pdf_output_name=self._inst_settings.pdf_output_name,
        )
        return pdf_output

    def set_sample_details(self, **kwargs):
        self._switch_mode_specific_inst_settings(kwargs.get("mode"))
        kwarg_name = "sample"
        sample_details_obj = common.dictionary_key_helper(
            dictionary=kwargs,
            key=kwarg_name,
            exception_msg="The argument containing sample details was not found. Please set the following argument: " + kwarg_name,
        )
        self._sample_details = sample_details_obj

    # Overrides
    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        self._check_sample_details()
        if self._is_vanadium:
            return polaris_algs.calculate_van_absorb_corrections(
                ws_to_correct=ws_to_correct,
                multiple_scattering=self._inst_settings.multiple_scattering,
                is_vanadium=self._is_vanadium,
                msevents=self._inst_settings.mayers_mult_scat_events,
            )
        else:
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
        cropped_ws = common.crop_in_tof(
            ws_to_crop=ws_to_crop, x_min=self._inst_settings.raw_data_crop_values[0], x_max=self._inst_settings.raw_data_crop_values[1]
        )
        return cropped_ws

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        cropped_ws = common.crop_banks_using_crop_list(bank_list=van_ws_to_crop, crop_values_list=self._inst_settings.van_crop_values)
        return cropped_ws

    @staticmethod
    def _generate_input_file_name(run_number, file_ext=""):
        polaris_old_name = "POL"
        polaris_new_name = "POLARIS"
        first_run_new_name = 96912

        if isinstance(run_number, list):
            # Lists use recursion to deal with individual entries
            updated_list = []
            for run in run_number:
                updated_list.append(Polaris._generate_input_file_name(run_number=run))
            return updated_list
        else:
            # Select between old and new prefix
            # Test if it can be converted to an int or if we need to ask Mantid to do it for us
            if isinstance(run_number, str) and not run_number.isdigit():
                # Convert using Mantid and take the first element which is most likely to be the lowest digit
                use_new_name = int(common.generate_run_numbers(run_number)[0]) >= first_run_new_name
            else:
                use_new_name = int(run_number) >= first_run_new_name

            prefix = polaris_new_name if use_new_name else polaris_old_name

            return prefix + str(run_number) + file_ext

    def _get_input_batching_mode(self):
        return self._inst_settings.input_mode

    def _get_instrument_bin_widths(self):
        return self._inst_settings.focused_bin_widths

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string, self._inst_settings.file_extension)

        if run_number_string_key in self._run_details_cached_obj:
            return self._run_details_cached_obj[run_number_string_key]

        self._run_details_cached_obj[run_number_string_key] = polaris_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium_run=self._is_vanadium
        )

        return self._run_details_cached_obj[run_number_string_key]

    def _switch_mode_specific_inst_settings(self, mode):
        if mode is None and hasattr(self._inst_settings, "mode"):
            mode = self._inst_settings.mode

        self._inst_settings.update_attributes(advanced_config=polaris_advanced_config.get_mode_specific_dict(mode), suppress_warnings=True)

    def _apply_paalmanpings_absorb_and_subtract_empty(self, workspace, summed_empty, sample_details, paalman_pings_events_per_point=None):
        return absorb_corrections.apply_paalmanpings_absorb_and_subtract_empty(
            workspace=workspace,
            summed_empty=summed_empty,
            sample_details=sample_details,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
        )

    def perform_abs_vanadium_norm(self):
        return self._inst_settings.van_normalisation_method == "Absolute"

    def apply_additional_per_detector_corrections(self, input_workspace, sample_details, run_details):
        if self._inst_settings.mode.lower() == "pdf":
            if not hasattr(self._inst_settings, "placzek_order") or self._inst_settings.placzek_order not in [1, 2]:
                self._inst_settings.placzek_order = 1
                logger.warning("Placzek correction order not specified or is invalid, defaulting to 1")
            if not hasattr(self._inst_settings, "sample_temp"):
                sample_temperature = None
            else:
                sample_temperature = str(self._inst_settings.sample_temp)
            return polaris_algs.apply_placzek_correction_per_detector(
                input_workspace, sample_details, run_details, self._inst_settings.placzek_order, sample_temperature
            )
        else:
            return input_workspace
