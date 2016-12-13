from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import isis_powder.routines.common as common
from isis_powder.pearl_routines import pearl_algs


def spline_vanadium_for_focusing(focused_vanadium_spectra, num_splines):
    # remove bragg peaks before spline
    stripped_peaks_list = []
    for ws in focused_vanadium_spectra:
        out_name = ws.name() + "_stripped"
        # We have to clone the workspace - if no peaks are found it trashes the list
        # as it assigns to new name to the previous list
        stripped_ws = mantid.CloneWorkspace(InputWorkspace=ws, OutputWorkspace=out_name)
        stripped_peaks_list.append(mantid.StripPeaks(InputWorkspace=stripped_ws, FWHM=15, Tolerance=8,
                                                     OutputWorkspace=out_name))

    # run twice on low angle as peaks are very broad
    for i in range(0, 2):
        stripped_peaks_list[12] = mantid.StripPeaks(InputWorkspace=stripped_peaks_list[12], FWHM=100, Tolerance=10,
                                                    OutputWorkspace=stripped_peaks_list[12])

        stripped_peaks_list[13] = mantid.StripPeaks(InputWorkspace=stripped_peaks_list[13], FWHM=60, Tolerance=10,
                                                    OutputWorkspace=stripped_peaks_list[13])

    tof_ws_list = []
    for ws in stripped_peaks_list:
        out_name = ws.name() + "_TOF"
        tof_ws_list.append(mantid.ConvertUnits(InputWorkspace=ws, Target="TOF", OutputWorkspace=out_name))
        common.remove_intermediate_workspace(ws)

    splined_ws_list = []
    bank_index = 0
    for ws in tof_ws_list:
        bank_index += 1
        out_ws_name = "spline_bank_" + str(bank_index + 1)
        splined_ws_list.append(mantid.SplineBackground(InputWorkspace=ws, OutputWorkspace=out_ws_name,
                                                       NCoeff=num_splines))
        common.remove_intermediate_workspace(ws)

    return splined_ws_list

