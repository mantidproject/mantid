from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import numpy as numpy

from PearlPowder_AbstractInst import AbstractInst
import PearlPowder_common as Common
import pearl_calib_factory
import pearl_cycle_factory
# TODO create static wrapper called start


class Pearl(AbstractInst):

    # # Instrument default settings
    _default_input_ext = '.raw'
    _lambda_lower = 0.03
    _lambda_upper = 6.00
    _tof_binning = "1500,-0.0006,19900"

    def __init__(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None):
        if user_name is None:
            raise ValueError("A username must be provided in the startup script")

        super(Pearl, self).__init__(calibration_dir=calibration_dir, raw_data_dir=raw_data_dir, output_dir=output_dir)
        self.user_name = user_name

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

        self.mode = None  # For later callers to set

    # --- Abstract Implementation ---- #

    def get_input_extension(self):
        return self._default_input_ext  # TODO allow user override

    def get_lambda_range(self):
        return self._lambda_lower, self._lambda_upper

    def get_tof_binning(self):
        return self._tof_binning

    def get_calibration_full_paths(self, cycle, tt_mode=''):
        calibration_file, grouping_file, van_absorb, van_file =\
            pearl_calib_factory.get_calibration_filename(cycle=cycle, tt_mode=tt_mode)

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

    # Implementation of instrument specific steps

    def _attenuate_workspace(self, input_workspace):
        wc_attenuated = mantid.PearlMCAbsorption(self.attenuation_full_path)
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
