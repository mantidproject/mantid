from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import numpy as numpy

import os

from PearlPowder_AbstractInst import AbstractInst
import PearlPowder_common as Common
import pearl_calib_factory
import pearl_cycle_factory
# TODO create static wrapper called start


class Pearl(AbstractInst):

    # # Instrument default settings
    _default_input_ext = '.raw'
    _default_group_names = "bank1,bank2,bank3,bank4"
    _lambda_lower = 0.03
    _lambda_upper = 6.00
    _focus_tof_binning = "1500,-0.0006,19900"
    _create_van_tof_binning = "100,-0.0006,19990"

    def __init__(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None,
                 input_file_ext=".raw", tt_mode="TT88"):
        if user_name is None:
            raise ValueError("A username must be provided in the startup script")

        super(Pearl, self).__init__(calibration_dir=calibration_dir, raw_data_dir=raw_data_dir, output_dir=output_dir,
                                    default_input_ext=input_file_ext, tt_mode=tt_mode)
        self._user_name = user_name

        # This advanced option disables appending the current cycle to the
        # path given for raw files.
        self._disable_appending_cycle_to_raw_dir = False

        # live_data_directory = None  # TODO deal with this

        # Old API support
        self._old_atten_file = None

        # File names
        pearl_mc_absorption_file_name = "PRL112_DC25_10MM_FF.OUT" # TODO
        self._attenuation_full_path = calibration_dir + pearl_mc_absorption_file_name # TODO
        self.mode = None  # For later callers to set TODO

    # --- Abstract Implementation ---- #

    # Params #
    def get_default_group_names(self):
        return self._default_group_names

    def get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def get_focus_tof_binning(self):
        return self._focus_tof_binning

    def get_create_van_tof_binning(self):
        return self._create_van_tof_binning

    # Methods #

    def get_calibration_full_paths(self, cycle):

        calibration_file, grouping_file, van_absorb, van_file =\
            pearl_calib_factory.get_calibration_filename(cycle=cycle, tt_mode=self.tt_mode)

        calibration_dir = self.calibration_dir

        calibration_full_path = calibration_dir + calibration_file
        grouping_full_path = calibration_dir + grouping_file
        van_absorb_full_path = calibration_dir + van_absorb
        van_file_full_path = calibration_dir + van_file

        calibration_details = {"calibration": calibration_full_path,
                               "grouping": grouping_full_path,
                               "vanadium_absorption": van_absorb_full_path,
                               "vanadium": van_file_full_path}

        return calibration_details

    @staticmethod
    def get_cycle_information(run_number):
        cycle, instrument_version = pearl_cycle_factory.get_cycle_dir(run_number)

        cycle_information = {'cycle': cycle,
                             'instrument_version': instrument_version}
        return cycle_information

    @staticmethod
    def get_instrument_alg_save_ranges(instrument_version):
        return _get_instrument_ranges(instrument_version=instrument_version)

    @staticmethod
    def generate_inst_file_name(run_number):
        return _gen_file_name(run_number=run_number)

    # Hook overrides

    def attenuate_workspace(self, input_workspace):
        return self._attenuate_workspace(input_workspace=input_workspace)

    def get_monitor(self, run_number, input_dir, spline_terms=20):
        return self._get_monitor(run_number=run_number, input_dir=input_dir, spline_terms=spline_terms)

    def get_monitor_spectra(self, run_number):
        return self._get_monitor_spectrum(run_number=run_number)

    def _skip_appending_cycle_to_raw_dir(self):
        return self._disable_appending_cycle_to_raw_dir

    def spline_background(self, focused_vanadium_ws, spline_number, instrument_version=''):
        if instrument_version == "new2":
            out_list = _spline_new2_background(in_workspace=focused_vanadium_ws, num_splines=spline_number,
                                               instrument_version=instrument_version)
        elif instrument_version == "new":
            out_list = _spline_new_background(in_workspace=focused_vanadium_ws, num_splines=spline_number,
                                              instrument_version=instrument_version)
        elif instrument_version == "old":
            out_list = _spline_old_background(in_workspace=focused_vanadium_ws, num_splines=spline_number)
        else:
            raise ValueError("Spline Background - PEARL: Instrument version unknown")
        return out_list

    # Implementation of instrument specific steps

    def _attenuate_workspace(self, input_workspace):
        if self._old_atten_file is None:  # For old API support
            attenuation_path = self._attenuation_full_path
        else:
            attenuation_path = self._old_atten_file

        wc_attenuated = mantid.PearlMCAbsorption(attenuation_path)
        wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
        wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=input_workspace,
                                                OutputWorkspace=wc_attenuated)
        output_workspace = mantid.Divide(LHSWorkspace=input_workspace, RHSWorkspace=wc_attenuated)
        Common.remove_intermediate_workspace(workspace_name="wc_attenuated")
        return output_workspace

    def _get_monitor(self, run_number, input_dir, spline_terms):
        get_monitor_ws = Common._load_monitor(run_number, input_dir=input_dir, instrument=self)
        get_monitor_ws = mantid.ConvertUnits(InputWorkspace=get_monitor_ws, Target="Wavelength")
        lmin, lmax = self.get_lambda_range()
        get_monitor_ws = mantid.CropWorkspace(InputWorkspace=get_monitor_ws, XMin=lmin, XMax=lmax)
        ex_regions = numpy.zeros((2, 4))
        ex_regions[:, 0] = [3.45, 3.7]
        ex_regions[:, 1] = [2.96, 3.2]
        ex_regions[:, 2] = [2.1, 2.26]
        ex_regions[:, 3] = [1.73, 1.98]
        # ConvertToDistribution(works)

        for reg in range(0, 4):
            get_monitor_ws = mantid.MaskBins(InputWorkspace=get_monitor_ws, XMin=ex_regions[0, reg],
                                             XMax=ex_regions[1, reg])

        get_monitor_ws = mantid.SplineBackground(InputWorkspace=get_monitor_ws, WorkspaceIndex=0, NCoeff=spline_terms)

        return get_monitor_ws

    def _get_monitor_spectrum(self, run_number):
        if run_number < 71009:
            if self.mode == "trans":
                mspectra = 1081
            elif self.mode == "all":
               mspectra = 2721
            elif self.mode == "novan":
               mspectra = 2721
            else:
                raise ValueError("Mode not set or supported")
        else:
            mspectra = 1
        return mspectra

    # Support for old API
    def _old_api_constructor_set(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None,
                                 input_file_ext=None, tt_mode=None):
        # Any param can be set so check each individually
        if user_name is not None:
            self._user_name = user_name
        if calibration_dir is not None:
            self._calibration_dir = calibration_dir
        if raw_data_dir is not None:
            self._raw_data_dir = raw_data_dir
        if output_dir is not None:
            self._output_dir = output_dir
        if input_file_ext is not None:
            self._default_input_ext = input_file_ext
        if tt_mode is not None:
            self._tt_mode = tt_mode

    def _old_api_set_tt_mode(self, tt_mode):
        self._tt_mode = tt_mode

    def _old_api_set_calib_dir(self, calib_dir):
        self._calibration_dir = calib_dir

    def _old_api_set_raw_data_dir(self, raw_data_dir):
        self._disable_appending_cycle_to_raw_dir = True
        self._raw_data_dir = raw_data_dir

    def _old_api_set_output_dir(self, output_dir):
        self._output_dir = output_dir

    def _old_api_set_ext(self, ext):
        self._default_input_ext = ext

    def _old_api_set_atten(self, atten_file):
        self._old_atten_file = _old_api_strip_file_path(atten_file)


def _old_api_strip_file_path(in_path):
    return os.path.basename(in_path)

# Implementation of static methods


def _gen_file_name(run_number):

    digit = len(str(run_number))

    if run_number < 71009:
        number_of_digits = 5
        filename = "PRL"
    else:
        number_of_digits = 8
        filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number)

    return filename


def _get_instrument_ranges(instrument_version):
    if instrument_version == "new" or instrument_version == "old":  # New and old have identical ranges
        alg_range = 12
        save_range = 3
    elif instrument_version == "new2":
        alg_range = 14
        save_range = 5
    else:
        raise ValueError("Instrument version unknown")

    return alg_range, save_range


def _spline_new2_background(in_workspace, num_splines, instrument_version):
    # remove bragg peaks before spline

    for i in range(0, 12):  # TODO remove this hardcoded value if possible - this is 14 with the 2 below
        van_stripped_ws = mantid.StripPeaks(InputWorkspace=in_workspace, FWHM=15, Tolerance=8,
                                            WorkspaceIndex=i)

    # run twice on low angle as peaks are very broad
    for i in range(0, 2):
        van_stripped_ws = mantid.StripPeaks(InputWorkspace=van_stripped_ws, FWHM=100, Tolerance=10,
                                            WorkspaceIndex=12)
        van_stripped_ws = mantid.StripPeaks(InputWorkspace=van_stripped_ws, FWHM=60, Tolerance=10,
                                            WorkspaceIndex=13)

    van_stripped_ws = mantid.ConvertUnits(InputWorkspace=van_stripped_ws, Target="TOF")

    splined_ws_list = _perform_spline_range(instrument_version, num_splines, van_stripped_ws)
    Common.remove_intermediate_workspace(van_stripped_ws)
    return splined_ws_list


def _spline_new_background(in_workspace, num_splines, instrument_version):

    # remove bragg peaks before spline
    for i in range(0, 12):
        van_stripped = mantid.StripPeaks(InputWorkspace=in_workspace, FWHM=15, Tolerance=8, WorkspaceIndex=i)

    van_stripped = mantid.ConvertUnits(InputWorkspace=van_stripped, Target="TOF")

    splined_ws_list = _perform_spline_range(instrument_version, num_splines, van_stripped)
    Common.remove_intermediate_workspace(van_stripped)
    return splined_ws_list


def _perform_spline_range(instrument_version, num_splines, stripped_ws):
    ws_range, unused = _get_instrument_ranges(instrument_version)
    splined_ws_list = []
    for i in range(0, ws_range):
        out_ws_name = "_create_van_cal_spline-" + str(i)
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=stripped_ws, OutputWorkspace=out_ws_name,
                                                       WorkspaceIndex=i, NCoeff=num_splines))
    return splined_ws_list


def _spline_old_background(in_workspace, num_splines):
    van_stripped = mantid.ConvertUnits(InputWorkspace=in_workspace, Target="dSpacing")

    # remove bragg peaks before spline
    van_stripped = mantid.StripPeaks(InputWorkspace=van_stripped, FWHM=15, Tolerance=6, WorkspaceIndex=0)
    van_stripped = mantid.StripPeaks(InputWorkspace=van_stripped, FWHM=15, Tolerance=6, WorkspaceIndex=2)
    van_stripped = mantid.StripPeaks(InputWorkspace=van_stripped, FWHM=15, Tolerance=6, WorkspaceIndex=3)
    van_stripped = mantid.StripPeaks(InputWorkspace=van_stripped, FWHM=40, Tolerance=12, WorkspaceIndex=1)
    van_stripped = mantid.StripPeaks(InputWorkspace=van_stripped, FWHM=60, Tolerance=12, WorkspaceIndex=1)

    # Mask low d region that is zero before spline
    for reg in range(0, 4):
        if reg == 1:
            van_stripped = mantid.MaskBins(InputWorkspace=van_stripped, XMin=0, XMax=0.14, SpectraList=reg)
        else:
            van_stripped = mantid.MaskBins(InputWorkspace=van_stripped, XMin=0, XMax=0.06, SpectraList=reg)

    van_stripped = mantid.ConvertUnits(InputWorkspace=van_stripped, Target="TOF")

    splined_ws_list = []
    for i in range(0, 4):
        out_ws_name = "_create_van_calc_spline-" + str(i)
        if i == 1:
            coeff = 80
        else:
            coeff = 100
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=van_stripped, OutputWorkspace=out_ws_name,
                                                       WorkspaceIndex=i, NCoeff=coeff))
    Common.remove_intermediate_workspace(van_stripped)
    return splined_ws_list

