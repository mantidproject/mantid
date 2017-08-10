from __future__ import (absolute_import, division, print_function)

import os

from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines import instrument_settings
from isis_powder.hrpd_routines import hrpd_advanced_config, hrpd_algs, hrpd_param_mapping


class HRPD(AbstractInst):

    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=hrpd_param_mapping.attr_mapping, adv_conf_dict=hrpd_advanced_config.get_all_adv_variables(),
            kwargs=kwargs)

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
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(
            run_number_string=self._inst_settings.run_number, do_van_normalisation=self._inst_settings.do_van_norm,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def create_vanadium(self, **kwargs):
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
