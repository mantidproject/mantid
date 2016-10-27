from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import numpy as numpy

import os

from PearlPowder_AbstractInst import AbstractInst
import PearlPowder_common as Common
import pearl_calib_factory
import pearl_cycle_factory


class Pearl(AbstractInst):

    # # Instrument default settings
    _default_input_ext = '.raw'
    _default_group_names = "bank1,bank2,bank3,bank4"

    _lambda_lower = 0.03
    _lambda_upper = 6.00

    _focus_tof_binning = "1500,-0.0006,19900"

    _create_van_first_tof_binning = "100,-0.0006,19990"
    _create_van_second_tof_binning = "150,-0.0006,19900"

    def __init__(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None,
                 input_file_ext=".raw", tt_mode="TT88"):

        super(Pearl, self).__init__(user_name=user_name, calibration_dir=calibration_dir, raw_data_dir=raw_data_dir,
                                    output_dir=output_dir, default_input_ext=input_file_ext, tt_mode=tt_mode)

        # This advanced option disables appending the current cycle to the
        # path given for raw files.
        self._disable_appending_cycle_to_raw_dir = False

        # live_data_directory = None  # TODO deal with this

        # Old API support
        self._old_atten_file = None
        self._old_api_uses_full_paths = False

        # File names
        pearl_mc_absorption_file_name = "PRL112_DC25_10MM_FF.OUT"  # TODO
        self._attenuation_full_path = calibration_dir + pearl_mc_absorption_file_name  # TODO

    # --- Abstract Implementation ---- #

    # Params #
    def _get_default_group_names(self):
        return self._default_group_names

    def _get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def _get_focus_tof_binning(self):
        return self._focus_tof_binning

    def _get_create_van_tof_binning(self):
        return_dict = {"1": self._create_van_first_tof_binning,
                       "2": self._create_van_second_tof_binning}
        return return_dict

    # Methods #

    def _get_calibration_full_paths(self, cycle):

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
    def _get_cycle_information(run_number):
        cycle, instrument_version = pearl_cycle_factory.get_cycle_dir(run_number)

        cycle_information = {'cycle': cycle,
                             'instrument_version': instrument_version}
        return cycle_information

    @staticmethod
    def _get_instrument_alg_save_ranges(instrument=''):
        return _get_instrument_ranges(instrument_version=instrument)

    @staticmethod
    def _generate_inst_file_name(run_number):
        return _gen_file_name(run_number=run_number)

    # Hook overrides

    def _attenuate_workspace(self, input_workspace):
        return self._run_attenuate_workspace(input_workspace=input_workspace)

    def _create_calibration(self, calibration_runs, offset_file_name, grouping_file_name):
        input_ws = Common._read_ws(number=calibration_runs, instrument=self)
        cycle_information = self._get_cycle_information(calibration_runs)

        # TODO move these hard coded params to instrument specific
        if cycle_information["instrument_version"] == "new" or cycle_information["instrument_version"] == "new2":
            input_ws = mantid.Rebin(InputWorkspace=input_ws, Params="100,-0.0006,19950")

        d_spacing_cal = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
        d_spacing_cal = mantid.Rebin(InputWorkspace=d_spacing_cal, Params="1.8,0.002,2.1")

        if cycle_information["instrument_version"] == "new2":
            cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                                 WorkspaceIndexMin=9, WorkspaceIndexMax=1063, XMin=1.8, XMax=2.1)

        elif cycle_information["instrument_version"] == "new":
            cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=20,
                                                 WorkspaceIndexMin=9, WorkspaceIndexMax=943, XMin=1.8, XMax=2.1)
        else:
            cross_cor_ws = mantid.CrossCorrelate(InputWorkspace=d_spacing_cal, ReferenceSpectra=500,
                                                 WorkspaceIndexMin=1, WorkspaceIndexMax=1440, XMin=1.8, XMax=2.1)
        if self._old_api_uses_full_paths:  # Workaround for old API setting full paths
            grouping_file_path = grouping_file_name
            offset_file_path = offset_file_name
        else:
            offset_file_path = self.calibration_dir + offset_file_name
            grouping_file_path = self.calibration_dir + grouping_file_name

        # Ceo Cell refined to 5.4102(3) so 220 is 1.912795
        offset_output_path = mantid.GetDetectorOffsets(InputWorkspace=cross_cor_ws, Step=0.002, DReference=1.912795,
                                                       XMin=-200, XMax=200, GroupingFileName=offset_file_path)
        del offset_output_path  # This isn't used so delete it to keep linters happy
        aligned_ws = mantid.AlignDetectors(InputWorkspace=input_ws, CalibrationFile=offset_file_path)
        cal_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws, GroupingFileName=grouping_file_path)

        Common.remove_intermediate_workspace(d_spacing_cal)
        Common.remove_intermediate_workspace(cross_cor_ws)
        Common.remove_intermediate_workspace(aligned_ws)
        Common.remove_intermediate_workspace(cal_grouped_ws)

    def _create_calibration_silicon(self, calibration_runs, cal_file_name, grouping_file_name):
        self._do_silicon_calibration(calibration_runs, cal_file_name, grouping_file_name)

    def _get_monitor(self, run_number, input_dir, spline_terms=20):
        return self._run_get_monitor(run_number=run_number, input_dir=input_dir, spline_terms=spline_terms)

    def _get_monitor_spectra(self, run_number):
        return self._get_monitor_spectrum(run_number=run_number)

    def _skip_appending_cycle_to_raw_dir(self):
        return self._disable_appending_cycle_to_raw_dir

    def _spline_background(self, focused_vanadium_ws, spline_number, instrument_version=''):
        if instrument_version == "new2":
            out_list = _spline_new2_background(in_workspace=focused_vanadium_ws, num_splines=spline_number,
                                               instrument_version=instrument_version)
        elif instrument_version == "new":
            out_list = _spline_new_background(in_workspace=focused_vanadium_ws, num_splines=spline_number,
                                              instrument_version=instrument_version)
        elif instrument_version == "old":
            out_list = _spline_old_background(in_workspace=focused_vanadium_ws)
        else:
            raise ValueError("Spline Background - PEARL: Instrument version unknown")
        return out_list

    # Implementation of instrument specific steps

    def _run_attenuate_workspace(self, input_workspace):
        if self._old_atten_file is None:  # For old API support
            attenuation_path = self._attenuation_full_path
        else:
            attenuation_path = self._old_atten_file

        wc_attenuated = mantid.PearlMCAbsorption(attenuation_path)
        wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
        wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=input_workspace,
                                                OutputWorkspace=wc_attenuated)
        pearl_attenuated_ws = mantid.Divide(LHSWorkspace=input_workspace, RHSWorkspace=wc_attenuated)
        Common.remove_intermediate_workspace(workspace_name=wc_attenuated)
        return pearl_attenuated_ws

    def _do_silicon_calibration(self, runs_to_process, cal_file_name, grouping_file_name):
        create_si_ws = Common._read_ws(number=runs_to_process, instrument=self)
        cycle_details = self._get_cycle_information(runs_to_process)
        instrument_version = cycle_details["instrument_version"]

        if instrument_version == "new" or instrument_version == "new2":
            create_si_ws = mantid.Rebin(InputWorkspace=create_si_ws, Params="100,-0.0006,19950")

        create_si_d_spacing_ws = mantid.ConvertUnits(InputWorkspace=create_si_ws, Target="dSpacing")

        if instrument_version == "new2":
            create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="1.71,0.002,2.1")
            create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                            ReferenceSpectra=20, WorkspaceIndexMin=9,
                                                            WorkspaceIndexMax=1063, XMin=1.71, XMax=2.1)
        elif instrument_version == "new":
            create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="1.85,0.002,2.05")
            create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                            ReferenceSpectra=20, WorkspaceIndexMin=9,
                                                            WorkspaceIndexMax=943, XMin=1.85, XMax=2.05)
        elif instrument_version == "old":
            create_si_d_spacing_rebin_ws = mantid.Rebin(InputWorkspace=create_si_d_spacing_ws, Params="3,0.002,3.2")
            create_si_cross_corr_ws = mantid.CrossCorrelate(InputWorkspace=create_si_d_spacing_rebin_ws,
                                                            ReferenceSpectra=500, WorkspaceIndexMin=1,
                                                            WorkspaceIndexMax=1440, XMin=3, XMax=3.2)
        else:
            raise NotImplementedError("The instrument version is not supported for creating a silicon calibration")

        Common.remove_intermediate_workspace(create_si_d_spacing_ws)
        Common.remove_intermediate_workspace(create_si_d_spacing_rebin_ws)

        calibration_output_path = self.calibration_dir + cal_file_name
        create_si_offsets_ws = mantid.GetDetectorOffsets(InputWorkspace=create_si_cross_corr_ws,
                                                         Step=0.002, DReference=1.920127251, XMin=-200, XMax=200,
                                                         GroupingFileName=calibration_output_path)
        create_si_aligned_ws = mantid.AlignDetectors(InputWorkspace=create_si_ws,
                                                     CalibrationFile=calibration_output_path)
        grouping_output_path = self.calibration_dir + grouping_file_name
        create_si_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=create_si_aligned_ws,
                                                           GroupingFileName=grouping_output_path)
        del create_si_offsets_ws, create_si_grouped_ws

    def _run_get_monitor(self, run_number, input_dir, spline_terms):
        load_monitor_ws = Common._load_monitor(run_number, input_dir=input_dir, instrument=self)
        get_monitor_ws = mantid.ConvertUnits(InputWorkspace=load_monitor_ws, Target="Wavelength")
        Common.remove_intermediate_workspace(load_monitor_ws)
        lmin, lmax = self._get_lambda_range()
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

        monitor_ws = mantid.SplineBackground(InputWorkspace=get_monitor_ws, WorkspaceIndex=0, NCoeff=spline_terms)
        Common.remove_intermediate_workspace(get_monitor_ws)
        return monitor_ws

    def _get_monitor_spectrum(self, run_number):
        if run_number < 71009:
            if self._focus_mode == "trans":
                mspectra = 1081
            elif self._focus_mode == "all":
                mspectra = 2721
            elif self._focus_mode == "novan":
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
            self.default_input_ext = input_file_ext
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
        self.default_input_ext = ext

    def _old_api_set_atten(self, atten_file):
        self._old_atten_file = _old_api_get_file_name(atten_file)

    def _old_api_set_full_paths(self, val):
        self._old_api_uses_full_paths = val

    def _PEARL_use_full_path(self):
        return self._old_api_uses_full_paths


def _old_api_get_file_name(in_path):
    # Gets the filename from a full path
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
    alg_range, unused = _get_instrument_ranges(instrument_version)
    van_stripped_ws = _strip_peaks_new_inst(in_workspace, alg_range)

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
    # Remove bragg peaks before spline
    alg_range, unused = _get_instrument_ranges(instrument_version)
    van_stripped = _strip_peaks_new_inst(in_workspace, alg_range)

    van_stripped = mantid.ConvertUnits(InputWorkspace=van_stripped, Target="TOF")

    splined_ws_list = _perform_spline_range(instrument_version, num_splines, van_stripped)
    Common.remove_intermediate_workspace(van_stripped)
    return splined_ws_list


def _strip_peaks_new_inst(input_ws, alg_range):
    van_stripped_ws = mantid.StripPeaks(InputWorkspace=input_ws, FWHM=15, Tolerance=8, WorkspaceIndex=0)
    for i in range(1, alg_range):
        van_stripped_ws = mantid.StripPeaks(InputWorkspace=van_stripped_ws, FWHM=15, Tolerance=8,
                                            WorkspaceIndex=i)

    return van_stripped_ws


def _perform_spline_range(instrument_version, num_splines, stripped_ws):
    ws_range, unused = _get_instrument_ranges(instrument_version)
    splined_ws_list = []
    for i in range(0, ws_range):
        out_ws_name = "spline" + str(i + 1)
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=stripped_ws, OutputWorkspace=out_ws_name,
                                                       WorkspaceIndex=i, NCoeff=num_splines))
    return splined_ws_list


def _spline_old_background(in_workspace):
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
        out_ws_name = "spline" + str(i+1)
        if i == 1:
            coeff = 80
        else:
            coeff = 100
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=van_stripped, OutputWorkspace=out_ws_name,
                                                       WorkspaceIndex=i, NCoeff=coeff))
    Common.remove_intermediate_workspace(van_stripped)
    return splined_ws_list
