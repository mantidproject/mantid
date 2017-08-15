from __future__ import (absolute_import, division, print_function)

import os

from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines import common, instrument_settings
from isis_powder.hrpd_routines import hrpd_advanced_config, hrpd_algs, hrpd_param_mapping


class HRPD(AbstractInst):

    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=hrpd_param_mapping.attr_mapping, kwargs=kwargs,
            adv_conf_dict=hrpd_advanced_config.get_all_adv_variables())

        super(HRPD, self).__init__(user_name=self._inst_settings.user_name,
                                   calibration_dir=self._inst_settings.calibration_dir,
                                   output_dir=self._inst_settings.output_dir,
                                   inst_prefix="HRPD")

        # Cannot load older .nxs files into Mantid from HRPD
        # because of a long-term bug which was not reported.
        # Instead, ask Mantid to use .raw files in this case
        if not self._inst_settings.file_extension:
            self._inst_settings.file_extension = ".raw"

        self._cached_run_details = {}

    def focus(self, **kwargs):
        self._switch_decoupled_mode_inst_settings(kwargs.get("decoupled_mode"))
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(
            run_number_string=self._inst_settings.run_number, do_van_normalisation=self._inst_settings.do_van_norm,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def create_vanadium(self, **kwargs):
        self._switch_decoupled_mode_inst_settings(kwargs.get("decoupled_mode"))
        self._inst_settings.update_attributes(kwargs=kwargs)

        return self._create_vanadium(run_number_string=self._inst_settings.run_in_range,
                                     do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string,
                                                                       self._inst_settings.file_extension)

        if run_number_string_key in self._cached_run_details:
            return self._cached_run_details[run_number_string_key]

        self._cached_run_details[run_number_string_key] = hrpd_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium=self._is_vanadium)

        return self._cached_run_details[run_number_string_key]

    def _spline_vanadium_ws(self, focused_vanadium_banks, instrument_version=''):
        spline_coeff = self._inst_settings.spline_coeff
        output = hrpd_algs.process_vanadium_for_focusing(bank_spectra=focused_vanadium_banks,
                                                         spline_number=spline_coeff)
        return output

    def _switch_decoupled_mode_inst_settings(self, decoupled_mode):
        self._inst_settings.update_attributes(
            advanced_config=hrpd_advanced_config.get_decoupled_mode_dict(decoupled_mode), suppress_warnings=True)

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.tof_cropping_values)

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return common.crop_in_tof(ws_to_crop=van_ws_to_crop, x_min=self._inst_settings.van_tof_cropping[0],
                                  x_max=self._inst_settings.van_tof_cropping[-1])

    def _get_instrument_bin_widths(self):
        return self._inst_settings.focused_bin_widths