from __future__ import (absolute_import, division, print_function)
import os
import numpy as numpy
import mantid.simpleapi as mantid

from isis_powder.pearl_routines import pearl_calib_factory, pearl_cycle_factory
import isis_powder.routines.common as common
from isis_powder.routines.RunDetails import RunDetails


def apply_tof_rebinning(ws_to_rebin, tof_params, return_units=None):
    rebinned_ws = mantid.ConvertUnits(InputWorkspace=ws_to_rebin, Target="TOF")
    rebinned_ws = mantid.Rebin(InputWorkspace=rebinned_ws, Params=tof_params)
    if return_units:
        rebinned_ws = mantid.ConvertUnits(InputWorkspace=rebinned_ws, Target=return_units)
    return rebinned_ws


def get_instrument_ranges(instrument_version):
    # TODO rename this to get number of banks/save range
    if instrument_version == "new" or instrument_version == "old":  # New and old have identical ranges
        alg_range = 12
        save_range = 3
    elif instrument_version == "new2":
        alg_range = 14
        save_range = 5
    else:
        raise ValueError("Instrument version unknown")

    return alg_range, save_range


def get_run_details(tt_mode, run_number_string, label, calibration_dir):
    calibration_file, grouping_file, van_absorb, van_file = \
        pearl_calib_factory.get_calibration_filename(cycle=label, tt_mode=tt_mode)
    cycle, instrument_version = pearl_cycle_factory.get_cycle_dir(run_number_string)

    calibration_full_path = os.path.join(calibration_dir, calibration_file)
    grouping_full_path = os.path.join(calibration_dir, grouping_file)
    van_absorb_full_path = os.path.join(calibration_dir, van_absorb)
    van_file_full_path = os.path.join(calibration_dir, van_file)

    run_details = RunDetails(calibration_path=calibration_full_path, grouping_path=grouping_full_path,
                             vanadium_runs=van_file_full_path, run_number=run_number_string)
    run_details.vanadium_absorption = van_absorb_full_path
    run_details.label = cycle
    run_details.instrument_version = instrument_version

    # TODO remove this when we move to saving splined van ws on PEARL
    run_details.splined_vanadium = run_details.vanadium

    return run_details


def normalise_ws_current(ws_to_correct, monitor_ws, spline_coeff):
    lambda_lower = 0.03  # TODO move these into config
    lambda_upper = 6.00

    processed_monitor_ws = mantid.ConvertUnits(InputWorkspace=monitor_ws, Target="Wavelength")
    processed_monitor_ws = mantid.CropWorkspace(InputWorkspace=processed_monitor_ws,
                                                XMin=lambda_lower, XMax=lambda_upper)
    ex_regions = numpy.zeros((2, 4))
    ex_regions[:, 0] = [3.45, 3.7]
    ex_regions[:, 1] = [2.96, 3.2]
    ex_regions[:, 2] = [2.1, 2.26]
    ex_regions[:, 3] = [1.73, 1.98]

    for reg in range(0, 4):
        processed_monitor_ws = mantid.MaskBins(InputWorkspace=processed_monitor_ws, XMin=ex_regions[0, reg],
                                               XMax=ex_regions[1, reg])

    splined_monitor_ws = mantid.SplineBackground(InputWorkspace=processed_monitor_ws,
                                                 WorkspaceIndex=0, NCoeff=spline_coeff)

    normalised_ws = mantid.ConvertUnits(InputWorkspace=ws_to_correct, Target="Wavelength")
    normalised_ws = mantid.NormaliseToMonitor(InputWorkspace=normalised_ws, MonitorWorkspace=splined_monitor_ws,
                                              IntegrationRangeMin=0.6, IntegrationRangeMax=5.0)
    normalised_ws = mantid.ConvertUnits(InputWorkspace=normalised_ws, Target="TOF")

    common.remove_intermediate_workspace(processed_monitor_ws)
    common.remove_intermediate_workspace(splined_monitor_ws)

    return normalised_ws
