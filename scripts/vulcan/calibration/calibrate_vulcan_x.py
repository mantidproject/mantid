# This is the workflow script to calibrate VULCAN-X
from calibrate_vulcan_cross_correlation import calibrate_vulcan
from check_calibration_alignment import reduce_calibration
from peak_position_calibration_step1 import fit_diamond_peaks, apply_peaks_positions_calibration
from mantid.simpleapi import LoadDiffCal, SaveDiffCal

import os
from typing import List, Union


def cross_correlate_calibrate(diamond_runs: List[Union[int, str]],
                              vulcan_x_idf: Union[None, str]):

    cal_file_name, diamond_ws_name = calibrate_vulcan(diamond_nexus=diamond_runs[1:2],
                                                      load_cutoff_time=None,
                                                      user_idf=vulcan_x_idf)

    return cal_file_name, diamond_ws_name


def align_vulcan_data(dia_runs, diff_cal_file_name):
    focused_ws_name, focused_nexus = reduce_calibration(dia_runs[:],
                                                        calibration_file=diff_cal_file_name,
                                                        idf_file=None,  # 'data/VULCAN_Definition_pete02.xml',
                                                        apply_mask=True,
                                                        align_detectors=True)

    return focused_ws_name, focused_nexus


def peak_position_calibrate(focused_diamond_ws_name, src_diff_cal_h5, target_diff_cal_h5):

    # Fit west bank
    west_res = fit_diamond_peaks(focused_diamond_ws_name, 0)
    # Fit east bank
    east_res = fit_diamond_peaks(focused_diamond_ws_name, 1)
    # Fit high angle bank
    high_angel_res = fit_diamond_peaks(focused_diamond_ws_name, 2)

    # apply 2nd round calibration to diffraction calibration file
    # Load calibration file
    calib_outputs = LoadDiffCal(Filename=src_diff_cal_h5,
                                InputWorkspace=focused_diamond_ws_name,
                                WorkspaceName='DiffCal_Vulcan')
    diff_cal_table_name = str(calib_outputs.OutputCalWorkspace)

    # Update calibration table and save
    apply_peaks_positions_calibration(diff_cal_table_name, [west_res, east_res, high_angel_res])

    # Save to new diffraction calibration file
    SaveDiffCal(CalibrationWorkspace=diff_cal_table_name,
                GroupingWorkspace=calib_outputs.OutputGroupingWorkspace,
                MaskWorkspace=calib_outputs.OutputMaskWorkspace,
                Filename=target_diff_cal_h5)

    return target_diff_cal_h5


def main():
    # User setup
    # Testing files
    diamond_run = ['/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192227.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192228.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192229.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192230.nxs.h5']
    #
    vulcan_x_idf = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration/data/VULCAN_Definition_pete02.xml'

    # output name
    final_calib_file = 'VULCAN_Calibration_Hybrid.h5'

    # do cross correlation calibration
    cc_calib_file, diamond_ws_name = cross_correlate_calibrate(diamond_run[0:1], vulcan_x_idf)

    # use the calibration file generated from previous step to align diamond runs
    cc_focus_ws_name, cc_focus_nexus = align_vulcan_data(dia_runs=diamond_run[0:1],
                                                         diff_cal_file_name=cc_calib_file)

    # do peak position calibration
    peak_position_calibrate(cc_focus_ws_name, cc_calib_file, final_calib_file)

    # use the calibration file generated from last step to align diamond runs again


if __name__ == '__main__':
    main()
