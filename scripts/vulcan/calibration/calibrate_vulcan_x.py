# This is the workflow script to calibrate VULCAN-X
from calibrate_vulcan_cross_correlation import calibrate_vulcan, load_event_data
from check_calibration_alignment import reduce_calibration, make_group_workspace
from peak_position_calibration_step1 import fit_diamond_peaks, apply_peaks_positions_calibration
from mantid.simpleapi import LoadDiffCal, SaveDiffCal, mtd
from lib_cross_correlation import CrossCorrelateParameter

import os
from typing import List, Union, Tuple, Dict


def load_diamond_runs(diamond_runs: List[Union[str, int]],
                      user_idf: Union[None,  str],
                      output_dir: Union[str, None]) -> Tuple[str, str]:
    """Load diamond run(s)

    Parameters
    ----------
    diamond_runs: ~list
    user_idf
    output_dir: str, None
        if None, then do not write count file

    Returns
    -------
    ~tuple
        Output workspace name, counts nexus file name

    """
    # counts ws name
    if output_dir:
        # only
        counts_nexus_file_base = 'Counts_'
        if isinstance(diamond_runs[0], int):
            counts_nexus_file_base += f'{diamond_runs[0]}'
        else:
            counts_nexus_file_base += os.path.basename(diamond_runs[0]).split('.')[0]
        counts_nexus_file = os.path.join(output_dir, f'{counts_nexus_file_base}.nxs')
    else:
        counts_nexus_file = None

    diamond_ws_name = load_event_data(diamond_runs, None,
                                      counts_nxs_name=counts_nexus_file,
                                      unit_dspace=True,
                                      idf_name=user_idf)

    return diamond_ws_name, counts_nexus_file


def cross_correlate_calibrate(diamond_runs: Union[str, List[Union[int, str]]],
                              cross_correlate_param_dict: Union[None, Dict[str, CrossCorrelateParameter]] = None,
                              bank_calibration_flag: Union[None, Dict[str, bool]] = None,
                              output_dir: str = os.getcwd(),
                              vulcan_x_idf: Union[str, None] = None):
    """

    Default VULCAN-X: when auto reducing, time focus data to pixel 48840 for bank 1 and 2, and 422304 for bank 5.
    those are centers.

    Parameters
    ----------
    diamond_runs
    cross_correlate_param_dict
    bank_calibration_flag
    output_dir
    vulcan_x_idf

    Returns
    -------

    """
    # Check input diamond runs or diamond workspace
    if isinstance(diamond_runs, str):
        # Check a valid workspace
        assert mtd.doesExist(diamond_runs)
        diamond_ws_name = diamond_runs
    else:
        # must be a list of nexus file names or runs
        diamond_ws_name, _ = load_diamond_runs(diamond_runs, vulcan_x_idf, output_dir)

    # Set up default
    if cross_correlate_param_dict is None:
        # peak position in d-Spacing
        bank1_cc_param = CrossCorrelateParameter(1.2614, 0.04, 40704, 80, -0.0003)
        bank2_cc_param = CrossCorrelateParameter(1.2614, 0.04, 40704, 80, -0.0003)
        bank5_cc_param = CrossCorrelateParameter(1.07577, 0.01, 182528, 20, -0.0003)

        # do cross correlation
        cross_correlate_param_dict = {'Bank1': bank1_cc_param,
                                      'Bank2': bank2_cc_param,
                                      'Bank5': bank5_cc_param}
    if bank_calibration_flag is None:
        bank_calibration_flag = {'Bank1': True, 'Bank2': True, 'Bank5': True}

    cal_file_name, diamond_ws_name = calibrate_vulcan(diamond_ws_name,
                                                      cross_correlate_param_dict,
                                                      bank_calibration_flag,
                                                      output_dir=output_dir)

    return cal_file_name, diamond_ws_name


def align_vulcan_data(diamond_runs: Union[str, List[Union[int, str]]],
                      diff_cal_file_name: str,
                      output_dir: str) -> Tuple[str, str]:
    """

    Parameters
    ----------
    diamond_runs: ~list, str
        list of diamond runs (nexus file path) or diamond EventWorkspace
    diff_cal_file_name
    output_dir

    Returns
    -------

    """
    if isinstance(diamond_runs, str):
        # Check a valid workspace
        assert mtd.doesExist(diamond_runs)
        diamond_ws_name = diamond_runs
    else:
        # must be a list of nexus file names or runs
        diamond_ws_name, _ = load_diamond_runs(diamond_runs, None, output_dir)

    # TODO FIXME - shall this method be revealed to the client?
    tube_group_ws_name = 'TubeGroup'
    tube_grouping_plan = [(0, 512, 81920), (81920, 1024, 81920 * 2), (81920 * 2, 256, 200704)]
    tube_group = make_group_workspace(diamond_ws_name, tube_group_ws_name, tube_grouping_plan)

    print(f'Reduce data with calibration file {diff_cal_file_name}')
    focused_ws_name, focused_nexus = reduce_calibration(diamond_ws_name,
                                                        calibration_file=diff_cal_file_name,
                                                        idf_file=None,  # 'data/VULCAN_Definition_pete02.xml',
                                                        apply_mask=True,
                                                        align_detectors=True,
                                                        customized_group_ws_name=tube_group,
                                                        output_dir=output_dir)

    return focused_ws_name, focused_nexus


def peak_position_calibrate(focused_diamond_ws_name, src_diff_cal_h5, target_diff_cal_h5, output_dir):

    #
    print(f'Peak position calibration: input workspace {focused_diamond_ws_name}: '
          f'number of spectra = {mtd[focused_diamond_ws_name].getNumberHistograms()}')

    # Fit west bank
    west_res = fit_diamond_peaks(focused_diamond_ws_name, 0, output_dir)
    # Fit east bank
    east_res = fit_diamond_peaks(focused_diamond_ws_name, 1, output_dir)
    # Fit high angle bank
    high_angel_res = fit_diamond_peaks(focused_diamond_ws_name, 2, output_dir)

    # apply 2nd round calibration to diffraction calibration file
    # Load calibration file
    calib_outputs = LoadDiffCal(Filename=src_diff_cal_h5,
                                InputWorkspace=focused_diamond_ws_name,
                                WorkspaceName='DiffCal_Vulcan')
    diff_cal_table_name = str(calib_outputs.OutputCalWorkspace)

    # Update calibration table and save
    apply_peaks_positions_calibration(diff_cal_table_name, [west_res, east_res, high_angel_res])

    # Target calibration file
    if os.path.dirname(target_diff_cal_h5) == '':
        target_diff_cal_h5 = os.path.join(output_dir, target_diff_cal_h5)

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
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192230.nxs.h5'][:]

    # Optional user specified Mantid IDF
    vulcan_x_idf = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/data/VULCAN_Definition_pete02.xml'

    # Output path
    output_dir = '/tmp/'  # os.getcwd()
    final_calib_file = 'VULCAN_Calibration_Hybrid.h5'
    #
    # ---------------------------------------------------------------------------

    # Load data (set)
    diamond_ws_name, _ = load_diamond_runs(diamond_run, vulcan_x_idf, output_dir)

    # Step 1: do cross correlation calibration
    cc_calib_file, diamond_ws_name = cross_correlate_calibrate(diamond_ws_name, output_dir=output_dir)

    # Step 2: use the calibration file generated from previous step to align diamond runs
    cc_focus_ws_name, cc_focus_nexus = align_vulcan_data(diamond_runs=diamond_ws_name,
                                                         diff_cal_file_name=cc_calib_file,
                                                         output_dir=output_dir)

    # Step 3: do peak position calibration
    peak_position_calibrate(cc_focus_ws_name, cc_calib_file, final_calib_file, output_dir)

    # use the calibration file generated from last step to align diamond runs again


if __name__ == '__main__':
    main()
