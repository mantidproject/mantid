from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid
import numpy as numpy
from mantid import config

import isis_powder.routines.common as common
from isis_powder.abstract_inst import AbstractInst
from isis_powder.pearl_routines import fmode_output, pearl_calib_factory, pearl_cycle_factory
from isis_powder.routines.RunDetails import RunDetails


class Pearl(AbstractInst):

    # # Instrument default settings
    _default_input_ext = '.raw'
    _default_group_names = "bank1,bank2,bank3,bank4"

    _lambda_lower = 0.03
    _lambda_upper = 6.00

    _focus_tof_binning = "1500,-0.0006,19900"

    _create_van_first_tof_binning = "100,-0.0006,19990"
    _create_van_second_tof_binning = "150,-0.0006,19900"

    def __init__(self, user_name, tt_mode="TT88", calibration_dir=None, output_dir=None, **kwargs):

        super(Pearl, self).__init__(user_name=user_name, calibration_dir=calibration_dir,
                                    output_dir=output_dir, kwargs=kwargs)

        self._tt_mode = tt_mode
        self._focus_mode = None

        # This advanced option disables appending the current cycle to the
        # path given for raw files.
        self._disable_appending_cycle_to_raw_dir = False

        # Old API support
        self._old_atten_file = None
        self._existing_config = None

        # File names
        pearl_mc_absorption_file_name = "PRL112_DC25_10MM_FF.OUT"  # TODO how often does this change
        self._attenuation_full_path = os.path.join(calibration_dir, pearl_mc_absorption_file_name)

    # --- Abstract Implementation ---- #

    def focus(self, run_number, focus_mode, do_attenuation=True, do_van_normalisation=True):
        self._focus_mode = focus_mode
        return self._focus(run_number=run_number,
                           do_attenuation=do_attenuation, do_van_normalisation=do_van_normalisation)

    def create_calibration_vanadium(self, vanadium_runs, empty_runs, output_file_name=None, num_of_splines=60,
                                    do_absorb_corrections=True, gen_absorb_correction=False):

        self._create_calibration_vanadium(vanadium_runs=vanadium_runs, empty_runs=empty_runs,
                                          output_file_name=output_file_name, num_of_splines=num_of_splines,
                                          do_absorb_corrections=do_absorb_corrections,
                                          gen_absorb_correction=gen_absorb_correction)

    # Params #
    def _get_default_group_names(self):
        return self._default_group_names

    def _get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def _get_create_van_tof_binning(self):
        return_dict = {"1": self._create_van_first_tof_binning,
                       "2": self._create_van_second_tof_binning}
        return return_dict

    # Methods #

    def _get_run_details(self, run_number):
        # TODO once we migrate this to another format (i.e. not the if/elif/else) implement cached val
        cycle_dict = self._get_label_information(run_number=run_number)

        calibration_file, grouping_file, van_absorb, van_file =\
            pearl_calib_factory.get_calibration_filename(cycle=cycle_dict["cycle"], tt_mode=self._tt_mode)
        cycle, instrument_version = pearl_cycle_factory.get_cycle_dir(run_number)

        calibration_dir = self.calibration_dir

        calibration_full_path = os.path.join(calibration_dir, calibration_file)
        grouping_full_path = os.path.join(calibration_dir, grouping_file)
        van_absorb_full_path = os.path.join(calibration_dir, van_absorb)
        van_file_full_path = os.path.join(calibration_dir, van_file)

        run_details = RunDetails(calibration_path=calibration_full_path, grouping_path=grouping_full_path,
                                 vanadium_runs=van_file_full_path, run_number=run_number)
        run_details.vanadium_absorption = van_absorb_full_path
        run_details.label = cycle
        run_details.instrument_version = instrument_version

        # TODO remove this when we move to saving splined van ws on PEARL
        run_details.splined_vanadium = run_details.vanadium

        return run_details

    def _get_label_information(self, run_number):
        # TODO remove this when we move to combining CAL/RUN factories
        run_input = ""
        if isinstance(run_number, int) or run_number.isdigit():
            run_input = int(run_number)
        else:
            # Only take first valid number as it is probably of the form 12345_12350
            for character in run_number:
                if character.isdigit():
                    run_input += character
                else:
                    break

        cycle, instrument_version = pearl_cycle_factory.get_cycle_dir(run_input)

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
        input_ws = common.load_current_normalised_ws(run_number_string=calibration_runs, instrument=self)
        cycle_information = self._get_label_information(calibration_runs)

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
            offset_file_path = os.path.join(self.calibration_dir, offset_file_name)
            grouping_file_path = os.path.join(self.calibration_dir, grouping_file_name)

        # Ceo Cell refined to 5.4102(3) so 220 is 1.912795
        offset_output_path = mantid.GetDetectorOffsets(InputWorkspace=cross_cor_ws, Step=0.002, DReference=1.912795,
                                                       XMin=-200, XMax=200, GroupingFileName=offset_file_path)
        del offset_output_path  # This isn't used so delete it to keep linters happy
        aligned_ws = mantid.AlignDetectors(InputWorkspace=input_ws, CalibrationFile=offset_file_path)
        cal_grouped_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws, GroupingFileName=grouping_file_path)

        common.remove_intermediate_workspace(d_spacing_cal)
        common.remove_intermediate_workspace(cross_cor_ws)
        common.remove_intermediate_workspace(aligned_ws)
        common.remove_intermediate_workspace(cal_grouped_ws)

    def _create_calibration_silicon(self, calibration_runs, cal_file_name, grouping_file_name):
        self._do_silicon_calibration(calibration_runs, cal_file_name, grouping_file_name)

    def _normalise_ws(self, ws_to_correct, run_details=None):
        if not run_details:
            raise RuntimeError("Run details was not passed into PEARL: normalise_ws")
        monitor_ws = common.load_monitor(run_numbers=run_details.run_number, instrument=self)
        return self._normalise_current_ws(ws_to_correct=ws_to_correct, load_monitor_ws=monitor_ws,
                                          spline_terms=20)

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
            out_list = _spline_old_instrument_background(in_workspace=focused_vanadium_ws)
        else:
            raise ValueError("Spline Background - PEARL: Instrument version unknown")
        return out_list

    def _do_tof_rebinning_focus(self, input_workspace):
        input_workspace = mantid.Rebin(InputWorkspace=input_workspace, Params=self._focus_tof_binning)
        return input_workspace

    def _focus_processing(self, run_number, input_workspace, perform_vanadium_norm):
        return self._perform_focus_loading(run_number, input_workspace, perform_vanadium_norm)

    def _process_focus_output(self, processed_spectra, run_details, attenuate=False):
        return fmode_output.generate_and_save_focus_output(self, processed_spectra=processed_spectra,
                                                           run_details=run_details,
                                                           perform_attenuation=attenuate,
                                                           focus_mode=self._focus_mode)

    def _apply_van_calibration_tof_rebinning(self, vanadium_ws, tof_rebin_pass, return_units):
        tof_rebin_param_dict = self._get_create_van_tof_binning()
        tof_rebin_param = tof_rebin_param_dict[str(tof_rebin_pass)]

        rebinned_ws = mantid.ConvertUnits(InputWorkspace=vanadium_ws, Target="TOF")
        rebinned_ws = mantid.Rebin(InputWorkspace=rebinned_ws, Params=tof_rebin_param)

        rebinned_ws = mantid.ConvertUnits(InputWorkspace=rebinned_ws, Target=return_units)

        common.remove_intermediate_workspace(vanadium_ws)
        vanadium_ws = rebinned_ws
        return vanadium_ws

    def _generate_vanadium_absorb_corrections(self, calibration_full_paths, ws_to_match):
        raise NotImplementedError("Generating absorption corrections needs to be implemented correctly")

        # TODO are these values applicable to all instruments
        shape_ws = mantid.CloneWorkspace(InputWorkspace=ws_to_match)
        mantid.CreateSampleShape(InputWorkspace=shape_ws, ShapeXML='<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                                                          <radius val="0.005" /> </sphere>')

        absorb_ws = \
            mantid.AbsorptionCorrection(InputWorkspace=shape_ws, AttenuationXSection="5.08",
                                        ScatteringXSection="5.1", SampleNumberDensity="0.072",
                                        NumberOfWavelengthPoints="25", ElementSize="0.05")
        mantid.SaveNexus(Filename=calibration_full_paths["vanadium_absorption"],
                         InputWorkspace=absorb_ws, Append=False)
        common.remove_intermediate_workspace(shape_ws)
        return absorb_ws

    def _calibration_rebin_to_workspace(self, ws_to_rebin, ws_to_match):
        rebinned_ws = mantid.RebinToWorkspace(WorkspaceToRebin=ws_to_rebin, WorkspaceToMatch=ws_to_match)
        common.remove_intermediate_workspace(ws_to_rebin)
        ws_to_rebin = rebinned_ws
        return ws_to_rebin

    def correct_sample_vanadium(self, focused_ws, index, vanadium_ws=None):
        data_ws = mantid.ExtractSingleSpectrum(InputWorkspace=focused_ws, WorkspaceIndex=index)
        data_ws = mantid.ConvertUnits(InputWorkspace=data_ws, Target="TOF")
        data_ws = mantid.Rebin(InputWorkspace=data_ws, Params=self._focus_tof_binning)

        if vanadium_ws:
            data_processed = "van_processed" + str(index)  # Workaround for Mantid overwriting the WS in a loop
            vanadium_ws = mantid.Rebin(InputWorkspace=vanadium_ws, Params=self._focus_tof_binning)
            data_ws = mantid.Divide(LHSWorkspace=data_ws, RHSWorkspace=vanadium_ws, OutputWorkspace=data_processed)
        else:
            data_processed = "processed-" + str(index)

        mantid.CropWorkspace(InputWorkspace=data_ws, XMin=0.1, OutputWorkspace=data_processed)

        if vanadium_ws:
            mantid.Scale(InputWorkspace=data_processed, Factor=10, OutputWorkspace=data_processed)

        common.remove_intermediate_workspace(data_ws)

        return data_processed

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
        common.remove_intermediate_workspace(workspace_name=wc_attenuated)
        return pearl_attenuated_ws

    def _do_silicon_calibration(self, runs_to_process, cal_file_name, grouping_file_name):
        # TODO fix all of this as the script is too limited to be useful
        create_si_ws = common.load_current_normalised_ws(run_number_string=runs_to_process, instrument=self)
        cycle_details = self._get_label_information(runs_to_process)
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

        common.remove_intermediate_workspace(create_si_d_spacing_ws)
        common.remove_intermediate_workspace(create_si_d_spacing_rebin_ws)

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

    def _normalise_current_ws(self, ws_to_correct, load_monitor_ws, spline_terms):
        get_monitor_ws = mantid.ConvertUnits(InputWorkspace=load_monitor_ws, Target="Wavelength")
        common.remove_intermediate_workspace(load_monitor_ws)
        lmin, lmax = self._get_lambda_range()
        get_monitor_ws = mantid.CropWorkspace(InputWorkspace=get_monitor_ws, XMin=lmin, XMax=lmax)
        ex_regions = numpy.zeros((2, 4))
        ex_regions[:, 0] = [3.45, 3.7]
        ex_regions[:, 1] = [2.96, 3.2]
        ex_regions[:, 2] = [2.1, 2.26]
        ex_regions[:, 3] = [1.73, 1.98]

        for reg in range(0, 4):
            get_monitor_ws = mantid.MaskBins(InputWorkspace=get_monitor_ws, XMin=ex_regions[0, reg],
                                             XMax=ex_regions[1, reg])

        monitor_ws = mantid.SplineBackground(InputWorkspace=get_monitor_ws, WorkspaceIndex=0, NCoeff=spline_terms)

        normalised_ws = mantid.ConvertUnits(InputWorkspace=ws_to_correct, Target="Wavelength")
        normalised_ws = mantid.NormaliseToMonitor(InputWorkspace=normalised_ws, MonitorWorkspace=monitor_ws,
                                                  IntegrationRangeMin=0.6, IntegrationRangeMax=5.0)
        normalised_ws = mantid.ConvertUnits(InputWorkspace=normalised_ws, Target="TOF")

        common.remove_intermediate_workspace(get_monitor_ws)
        common.remove_intermediate_workspace(monitor_ws)

        return normalised_ws

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

    def _generate_raw_data_cycle_dir(self, run_cycle):
        if self._skip_appending_cycle_to_raw_dir():
            return self.raw_data_dir
        str_run_cycle = str(run_cycle)

        # Append current cycle to raw data directory
        generated_dir = os.path.join(self.raw_data_dir, str_run_cycle)
        generated_dir += '/'

        return generated_dir

    # Support for old API - This can be removed when PEARL_routines is removed
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
        self._old_api_uses_full_paths = True
        self._raw_data_dir = raw_data_dir

    def _old_api_set_output_dir(self, output_dir):
        self._output_dir = output_dir

    def _old_api_set_ext(self, ext):
        self.default_input_ext = ext

    def _old_api_set_atten(self, atten_file):
        self._old_atten_file = _old_api_get_file_name(atten_file)

    def _old_api_set_full_paths(self, val):
        self._old_api_uses_full_paths = val

    def _PEARL_filename_is_full_path(self):
        return self._old_api_uses_full_paths

    def PEARL_setup_input_directories(self, run_number):
        run_details = self._get_run_details(run_number=run_number)
        generated_path = self._generate_raw_data_cycle_dir(run_cycle=run_details.label)
        user_dirs = config['datasearch.directories']
        user_dirs_list = user_dirs.split(';')
        if generated_path not in user_dirs_list:
            config['datasearch.directories'] += ';' + generated_path

    def _get_focus_tof_binning(self):
        return self._focus_tof_binning


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
    common.remove_intermediate_workspace(van_stripped_ws)
    return splined_ws_list


def _spline_new_background(in_workspace, num_splines, instrument_version):
    # Remove bragg peaks before spline
    alg_range, unused = _get_instrument_ranges(instrument_version)
    van_stripped = _strip_peaks_new_inst(in_workspace, alg_range)

    van_stripped = mantid.ConvertUnits(InputWorkspace=van_stripped, Target="TOF")

    splined_ws_list = _perform_spline_range(instrument_version, num_splines, van_stripped)
    common.remove_intermediate_workspace(van_stripped)
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


def _spline_old_instrument_background(in_workspace):
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
    common.remove_intermediate_workspace(van_stripped)
    return splined_ws_list
