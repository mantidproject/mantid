from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import isis_powder.routines.common as common
from isis_powder.pearl_routines import pearl_algs


def spline_vanadium_for_focusing(focused_vanadium_ws, spline_number, instrument_version):
    if instrument_version == "new2" or instrument_version == "new":
        out_list = _van_spline_new_instruments(in_workspace=focused_vanadium_ws, num_splines=spline_number,
                                               instrument_version=instrument_version)
    elif instrument_version == "old":
        out_list = _van_spline_old_instrument(in_workspace=focused_vanadium_ws)
    else:
        raise ValueError("Spline Background - PEARL: Instrument version unknown")

    return out_list


def _perform_spline_range(num_of_banks, num_splines, stripped_ws):
    splined_ws_list = []
    for i in range(0, num_of_banks):
        out_ws_name = "spline" + str(i + 1)
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=stripped_ws, OutputWorkspace=out_ws_name,
                                                       WorkspaceIndex=i, NCoeff=num_splines))
    return splined_ws_list


def _strip_peaks_new_inst(input_ws, alg_range):
    van_stripped_ws = mantid.StripPeaks(InputWorkspace=input_ws, FWHM=15, Tolerance=8, WorkspaceIndex=0)
    for i in range(1, alg_range):
        van_stripped_ws = mantid.StripPeaks(InputWorkspace=van_stripped_ws, FWHM=15, Tolerance=8,
                                            WorkspaceIndex=i)

    return van_stripped_ws


def _van_spline_new_instruments(in_workspace, num_splines, instrument_version):
    # remove bragg peaks before spline
    alg_range, unused = pearl_algs.get_instrument_ranges(instrument_version)
    van_stripped_ws = _strip_peaks_new_inst(in_workspace, alg_range)

    if instrument_version == "new2":
        # run twice on low angle as peaks are very broad
        for i in range(0, 2):
            van_stripped_ws = mantid.StripPeaks(InputWorkspace=van_stripped_ws, FWHM=100, Tolerance=10,
                                                WorkspaceIndex=12)
            van_stripped_ws = mantid.StripPeaks(InputWorkspace=van_stripped_ws, FWHM=60, Tolerance=10,
                                                WorkspaceIndex=13)

    van_stripped_ws = mantid.ConvertUnits(InputWorkspace=van_stripped_ws, Target="TOF")

    splined_ws_list = _perform_spline_range(alg_range, num_splines, van_stripped_ws)
    common.remove_intermediate_workspace(van_stripped_ws)
    return splined_ws_list


def _van_spline_old_instrument(in_workspace):
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
