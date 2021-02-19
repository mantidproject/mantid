# This is the workflow script to calibrate VULCAN-X
from calibrate_vulcan_cross_correlation import calibrate_vulcan, load_event_data
from check_calibration_alignment import reduce_calibration
from peak_position_calibration_step1 import fit_diamond_peaks, apply_peaks_positions_calibration
from mantid.simpleapi import LoadDiffCal, SaveDiffCal, mtd

import os
from typing import List, Union, Tuple


def load_diamond_runs(diamond_runs: List[Union[str, int]],
                      user_idf: Union[None,  str],
                      output_dir: str) -> Tuple[str, str]:
    """Load diamond run(s)

    Parameters
    ----------
    diamond_runs: ~list
    user_idf
    output_dir

    Returns
    -------
    ~tuple
        Output workspace name, counts nexus file name

    """
    # counts ws name
    counts_nexus_file_base = 'Counts_'
    if isinstance(diamond_runs[0], int):
        counts_nexus_file_base += f'{diamond_runs[0]}'
    else:
        counts_nexus_file_base += os.path.basename(diamond_runs[0]).split('.')[0]
    counts_nexus_file = os.path.join(output_dir, f'{counts_nexus_file_base}.nxs')

    diamond_ws_name = load_event_data(diamond_runs, None,
                                      counts_nxs_name=counts_nexus_file,
                                      unit_dspace=True,
                                      idf_name=user_idf)

    return diamond_ws_name, counts_nexus_file


def cross_correlate_calibrate(diamond_runs: Union[str, List[Union[int, str]]],
                              vulcan_x_idf: Union[None, str],
                              output_dir):

    if isinstance(diamond_runs, str):
        # Check a valid workspace
        assert mtd.doesExist(diamond_runs)
        diamond_ws_name = diamond_runs
    else:
        # must be a list of nexus file names or runs
        diamond_ws_name, _ = load_diamond_runs(diamond_runs, vulcan_x_idf, output_dir)

    cal_file_name, diamond_ws_name = calibrate_vulcan(diamond_ws_name=diamond_ws_name,
                                                      output_dir=output_dir)

    return cal_file_name, diamond_ws_name


def align_vulcan_data(dia_runs, diff_cal_file_name):
    print(f'Reduce data with calibration file {diff_cal_file_name}')
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
    # --------------------------------------------------------------------------
    # User setup
    #
    # Testing files
    diamond_run = ['/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192227.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192228.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192229.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192230.nxs.h5']

    # Optional user specified Mantid IDF
    vulcan_x_idf = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/data/VULCAN_Definition_pete02.xml'

    # Output path
    output_dir = os.getcwd()
    final_calib_file = 'VULCAN_Calibration_Hybrid.h5'
    #
    # ---------------------------------------------------------------------------

    # Load data (set)
    diamond_ws_name = load_diamond_runs(diamond_run[0:2], vulcan_x_idf, output_dir)

    # Step 1: do cross correlation calibration
    cc_calib_file, diamond_ws_name = cross_correlate_calibrate(diamond_ws_name, output_dir)

    # use the calibration file generated from previous step to align diamond runs
    cc_focus_ws_name, cc_focus_nexus = align_vulcan_data(dia_runs=diamond_run[0:1],
                                                         diff_cal_file_name=cc_calib_file,
                                                         output_dir=output_dir)

    # do peak position calibration
    peak_position_calibrate(cc_focus_ws_name, cc_calib_file, final_calib_file, output_dir)

    # use the calibration file generated from last step to align diamond runs again


if __name__ == '__main__':
    main()
