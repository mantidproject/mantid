# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os

from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines import absorb_corrections, common, instrument_settings
from isis_powder.hrpd_routines import hrpd_advanced_config, hrpd_algs, hrpd_param_mapping

import mantid.simpleapi as mantid

# A bug on the instrument when recording historic NeXus files (< 2015) caused
# corrupted data. Use raw files for now until sufficient time has past and old
# data is unlikely to be reanalysed.
RAW_DATA_EXT = '.raw'

# Constants
PROMPT_PULSE_INTERVAL = 20000.0
PROMPT_PULSE_RIGHT_WIDTH = 140.0
PROMPT_PULSE_LEFT_WIDTH = 30.0


class HRPD(AbstractInst):

    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=hrpd_param_mapping.attr_mapping, kwargs=kwargs,
            adv_conf_dict=hrpd_advanced_config.get_all_adv_variables())

        super(HRPD, self).__init__(user_name=self._inst_settings.user_name,
                                   calibration_dir=self._inst_settings.calibration_dir,
                                   output_dir=self._inst_settings.output_dir,
                                   inst_prefix="HRPD")

        self._cached_run_details = {}
        self._sample_details = None

    def focus(self, **kwargs):
        self._switch_tof_window_inst_settings(kwargs.get("window"))
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(
            run_number_string=self._inst_settings.run_number, do_van_normalisation=self._inst_settings.do_van_norm,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def create_vanadium(self, **kwargs):
        self._switch_tof_window_inst_settings(kwargs.get("window"))
        self._inst_settings.update_attributes(kwargs=kwargs)

        return self._create_vanadium(run_number_string=self._inst_settings.run_in_range,
                                     do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def set_sample_details(self, **kwargs):
        kwarg_name = "sample"
        sample_details_obj = common.dictionary_key_helper(
            dictionary=kwargs, key=kwarg_name,
            exception_msg="The argument containing sample details was not found. Please"
                          " set the following argument: {}".format(kwarg_name))
        self._sample_details = sample_details_obj

    def mask_prompt_pulses_if_necessary(self, ws_list):
        for ws in ws_list:
            self._mask_prompt_pulses(ws)

    def should_subtract_empty_inst(self):
        return self._inst_settings.subtract_empty_inst

    def create_solid_angle_corrections(self, vanadium, run_details):
        """
        Creates the solid angle corrections from a vanadium run, only applicable on HRPD otherwise return None
        :param vanadium: The vanadium used to create this
        :param run_details: the run details of to use
        """
        settings = self._inst_settings
        if not settings.do_solid_angle:
            return
        solid_angle = mantid.SolidAngle(InputWorkspace=vanadium)
        solid_angle = mantid.Scale(InputWorkspace=solid_angle, Factor=100, Operation='Multiply')

        eff = mantid.Divide(LHSWorkspace=vanadium, RHSWorkspace=solid_angle)
        eff = mantid.ConvertUnits(InputWorkspace=eff, Target='Wavelength')
        integration_range = settings.eff_integration_range
        # use full range if no range is supplied
        integration_range = integration_range if integration_range is not None else (None, None)
        eff = mantid.Integration(InputWorkspace=eff,
                                 RangeLower=integration_range[0],
                                 RangeUpper=integration_range[1])

        correction = mantid.Multiply(LHSWorkspace=solid_angle, RHSWorkspace=eff)
        correction = mantid.Scale(InputWorkspace=correction, Factor=1e-5,
                                  Operation='Multiply')
        name = "sac" + common.generate_splined_name(run_details.run_number, [])
        path = run_details.van_paths

        mantid.SaveNexus(InputWorkspace=correction, Filename=os.path.join(path, name))
        common.remove_intermediate_workspace(eff)
        common.remove_intermediate_workspace(correction)

    def get_solid_angle_corrections(self, vanadium, run_details):
        if not self._inst_settings.do_solid_angle:
            return
        name = "sac" + common.generate_splined_name(vanadium, [])
        path = run_details.van_paths
        try:
            solid_angle = mantid.Load(Filename=os.path.join(path,name))
            return solid_angle
        except ValueError:
            raise RuntimeError("Could not find " + os.path.join(path, name)+" please run create_vanadium with "
                                                                            "\"do_solid_angle_corrections=True\"")

    def _generate_input_file_name(self, run_number, file_ext=""):
        """
        Generates a name which Mantid uses within Load to find the file.
        :param run_number: The run number to convert into a valid format for Mantid
        :param file_ext: An optional file extension to add to force a particular format
        :return: A filename that will allow Mantid to find the correct run for that instrument.
        """
        if not file_ext:
            file_ext = RAW_DATA_EXT
        return self._generate_inst_filename(run_number=run_number, file_ext=file_ext)

    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        if self._is_vanadium:
            return hrpd_algs.calculate_van_absorb_corrections(
                ws_to_correct=ws_to_correct, multiple_scattering=self._inst_settings.multiple_scattering)
        elif self._sample_details is None:
            raise RuntimeError("Absorption corrections cannot be run without sample details."
                               " Please set sample details using set_sample before running absorption corrections.")
        elif self._sample_details.shape_type() == "slab":
            return hrpd_algs.calculate_slab_absorb_corrections(ws_to_correct=ws_to_correct,
                                                               sample_details_obj=self._sample_details)
        else:
            return absorb_corrections.run_cylinder_absorb_corrections(
                ws_to_correct=ws_to_correct, multiple_scattering=self._inst_settings.multiple_scattering,
                sample_details_obj=self._sample_details, is_vanadium=self._is_vanadium)

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.tof_cropping_values)

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return common.crop_in_tof(ws_to_crop=van_ws_to_crop, x_min=self._inst_settings.van_tof_cropping[0],
                                  x_max=self._inst_settings.van_tof_cropping[-1])

    def _get_instrument_bin_widths(self):
        return self._inst_settings.focused_bin_widths

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string,
                                                                       self._inst_settings.file_extension)

        if run_number_string_key in self._cached_run_details:
            return self._cached_run_details[run_number_string_key]

        self._cached_run_details[run_number_string_key] = hrpd_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium=self._is_vanadium)

        return self._cached_run_details[run_number_string_key]

    def _mask_prompt_pulses(self, ws):
        """
        HRPD has a long flight path from the moderator resulting
        in sharp peaks from the proton pulse that maintain their
        sharp resolution. Here we mask these pulses out that occur
        at 20ms intervals.

        :param ws: The workspace containing the pulses. It is
        masked in place.
        """
        # The number of pulse can vary depending on the data range
        # Compute number of pulses that occur at each 20ms interval.
        x_data = ws.readX(0)
        pulse_min = int(round(x_data[0]) / PROMPT_PULSE_INTERVAL) + 1
        pulse_max = int(round(x_data[-1]) / PROMPT_PULSE_INTERVAL) + 1
        for i in range(pulse_min, pulse_max):
            centre = PROMPT_PULSE_INTERVAL * float(i)
            mantid.MaskBins(InputWorkspace=ws, OutputWorkspace=ws,
                            XMin=centre - PROMPT_PULSE_LEFT_WIDTH,
                            XMax=centre + PROMPT_PULSE_RIGHT_WIDTH)

    def _switch_tof_window_inst_settings(self, tof_window):
        self._inst_settings.update_attributes(
            advanced_config=hrpd_advanced_config.get_tof_window_dict(tof_window=tof_window))
