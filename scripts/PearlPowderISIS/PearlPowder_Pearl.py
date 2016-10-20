from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import numpy as numpy

import PearlPowder_common as Common
import pearl_calib_factory
import pearl_cycle_factory
# TODO create static wrapper called start


class Pearl:

    def __init__(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None):
        if user_name is None:
            raise ValueError("A username must be provided in the startup script")

        self.user_name = user_name
        self.calibration_dir = calibration_dir
        self.raw_data_dir = raw_data_dir
        self.output_dir = output_dir

        # This advanced option disables appending the current cycle to the
        # path given for raw files.
        self.disable_appending_cycle_to_raw_dir = False

        # live_data_directory = None  # TODO deal with this

        # File names # TODO remove this
        pearl_MC_absorption_file_name = "PRL112_DC25_10MM_FF.OUT"
        cal_file_name = "pearl_offset_11_2.cal"
        group_file_name = "pearl_group_11_2_TT88.cal"
        van_absorb_file_name = "van_spline_all_cycle_11_1.nxs"
        van_file_name = "van_spline_all_cycle_11_1.nxs"

        self.attenuation_full_path = calibration_dir + pearl_MC_absorption_file_name

        # Instrument defaults settings
        self.lambda_lower = 0.03
        self.lambda_upper = 6.00
        self.tof_binning = "1500,-0.0006,19900"

        # Instrument specific params
        self.mode = None # For later callers to set

    # ---- PEARL API ---- #

    def attenuate_workspace(self, input_workspace):
        return self._attenuate_workspace(input_workspace=input_workspace)

    def get_lambda_range(self):
        return self.lambda_lower, self.lambda_upper

    def get_monitor(self, run_number, file_extension, input_dir, spline_terms=20):
        return self._get_monitor(run_number=run_number, ext=file_extension,
                                 input_dir=input_dir, spline_terms=spline_terms)

    def get_monitor_spectra(self, run_number):
        return self._get_monitor_spectrum(run_number=run_number)

    @staticmethod
    def generate_out_file_paths(run_number, output_directory):
        return _generate_out_file_names(number=run_number, output_directory=output_directory)

    @staticmethod
    def generate_out_file_name(run_number, ext):
        return _gen_file_name(run_number=run_number, ext=ext)

    @staticmethod
    def get_instrument_alg_save_ranges(instrument_version):
        return _get_instrument_ranges(instrument_version=instrument_version)

    @staticmethod
    def get_calibration_file_names(cycle, tt_mode):
        return pearl_calib_factory.get_calibration_filename(cycle=cycle, tt_mode=tt_mode)

    @staticmethod
    def get_cycle_directory(cycle):
        return pearl_cycle_factory.get_cycle_dir(cycle)

    # PEARL implementation of instrument specific steps

    def _attenuate_workspace(self, input_workspace):
        wc_attenuated = mantid.PearlMCAbsorption(self.attenuation_full_path)
        wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
        wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=input_workspace,
                                                OutputWorkspace=wc_attenuated)
        output_workspace = mantid.Divide(LHSWorkspace=input_workspace, RHSWorkspace=wc_attenuated)
        Common.remove_intermediate_workspace(workspace_name="wc_attenuated")
        return output_workspace

    def _get_monitor(self, run_number, ext, input_dir, spline_terms):
        get_monitor_ws = Common._load_monitor(run_number, ext, input_dir=input_dir, instrument=self)
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


# Functions which only output instrument specific data

def _generate_out_file_names(number, output_directory):
    outfile = output_directory + "PRL" + str(number) + ".nxs"
    gssfile = output_directory + "PRL" + str(number) + ".gss"
    tof_xye_file = output_directory + "PRL" + str(number) + "_tof_xye.dat"
    d_xye_file = output_directory + "PRL" + str(number) + "_d_xye.dat"
    outwork = "PRL" + str(number)

    out_file_names = {"nxs_filename": outfile,
                      "gss_filename": gssfile,
                      "tof_xye_filename": tof_xye_file,
                      "dspacing_xye_filename": d_xye_file,
                      "output_name": outwork}

    return out_file_names


def _gen_file_name(run_number, ext):

    digit = len(str(run_number))

    if run_number < 71009:
        number_of_digits = 5
        filename = "PRL"
    else:
        number_of_digits = 8
        filename = "PEARL"

    for i in range(0, number_of_digits - digit):
        filename += "0"

    filename += str(run_number) + "." + ext

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
