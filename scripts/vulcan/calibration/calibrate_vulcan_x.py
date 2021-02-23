# This is the workflow script to calibrate VULCAN-X
from calibrate_vulcan_cross_correlation import calibrate_vulcan, load_event_data
from check_calibration_alignment import reduce_calibration, make_group_workspace
from peak_position_calibration_step1 import fit_diamond_peaks, apply_peaks_positions_calibration
from mantid.simpleapi import LoadDiffCal, SaveDiffCal, mtd, LoadNexusProcessed
from lib_cross_correlation import CrossCorrelateParameter
import os
from typing import List, Union, Tuple, Dict
from matplotlib import pyplot as plt
import numpy as np


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
                      output_dir: str,
                      tube_grouping_plan: List[Tuple[int, int, int]]) -> Tuple[str, str]:
    """

    Parameters
    ----------
    diamond_runs: ~list, str
        list of diamond runs (nexus file path) or diamond EventWorkspace
    diff_cal_file_name
    output_dir: str
        output directory
    tube_grouping_plan: ~list
        List of 3 tuples to indicate how to group

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
    if tube_grouping_plan:
        tube_group_ws_name = 'TubeGroup'
        tube_group = make_group_workspace(diamond_ws_name, tube_group_ws_name, tube_grouping_plan)
    else:
        tube_group = None

    print(f'Reduce data with calibration file {diff_cal_file_name}')
    focused_ws_name, focused_nexus = reduce_calibration(diamond_ws_name,
                                                        calibration_file=diff_cal_file_name,
                                                        idf_file=None,  # 'data/VULCAN_Definition_pete02.xml',
                                                        apply_mask=True,
                                                        align_detectors=True,
                                                        customized_group_ws_name=tube_group,
                                                        output_dir=output_dir)

    return focused_ws_name, focused_nexus


def peak_position_calibrate(focused_diamond_ws_name,
                            grouping_plan: List[Tuple[int, int, int]],
                            src_diff_cal_h5,
                            target_diff_cal_h5,
                            output_dir):

    #
    print(f'Peak position calibration: input workspace {focused_diamond_ws_name}: '
          f'number of spectra = {mtd[focused_diamond_ws_name].getNumberHistograms()}')

    # Fit peaks
    tube_res_list = list()
    pixel_range_left_list = list()
    pixel_range_right_list = list()
    last_focused_group_index = 0
    for start_ws_index, step, end_ws_index in grouping_plan:
        # calculate new workspace index range in the focused workspace
        # NOTE that whatever in the grouping plan is for raw workspace!
        num_groups = (end_ws_index - start_ws_index) // step
        start_group_index = last_focused_group_index
        end_group_index = start_group_index + num_groups

        # Pixel range
        bank_group_left_pixels = list(np.arange(start_ws_index, end_ws_index, step))
        bank_group_right_pixels = list(np.arange(start_ws_index, end_ws_index, step) + step)
        # fit: num groups spectra
        print(f'{focused_diamond_ws_name}: Fit from {start_group_index} to {end_group_index} (exclusive)')
        bank_residual_list = fit_diamond_peaks(focused_diamond_ws_name, start_group_index, end_group_index, output_dir)

        # sanity check
        assert len(bank_residual_list) == end_group_index - start_group_index
        # append
        tube_res_list.extend(bank_residual_list)
        pixel_range_left_list.extend(bank_group_left_pixels)
        pixel_range_right_list.extend(bank_group_right_pixels)

        # update
        last_focused_group_index = end_group_index
        # END-FOR

    # apply 2nd round calibration to diffraction calibration file
    # Load calibration file
    calib_outputs = LoadDiffCal(Filename=src_diff_cal_h5,
                                InputWorkspace=focused_diamond_ws_name,
                                WorkspaceName='DiffCal_Vulcan')
    diff_cal_table_name = str(calib_outputs.OutputCalWorkspace)

    # apply  2nd-round calibration
    apply_peaks_positions_calibration(diff_cal_table_name,
                                      zip(tube_res_list, pixel_range_left_list, pixel_range_right_list))

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

    # Define group plan as a check for step 1 and a plan for step 3
    tube_grouping_plan = [(0, 512, 81920), (81920, 1024, 81920 * 2), (81920 * 2, 256, 200704)]

    # Step 2: use the calibration file generated from previous step to align diamond runs
    cc_focus_ws_name, cc_focus_nexus = align_vulcan_data(diamond_runs=diamond_ws_name,
                                                         diff_cal_file_name=cc_calib_file,
                                                         output_dir=output_dir,
                                                         tube_grouping_plan=tube_grouping_plan)

    # Step 3: do peak position calibration
    peak_position_calibrate(cc_focus_ws_name, tube_grouping_plan, cc_calib_file, final_calib_file, output_dir)

    # use the calibration file generated from last step to align diamond runs again


def test_bank_wise_calibration():

    # Set up
    cc_focus_ws_name = 'VULCAN_384Banks'
    cc_focus_nexus = os.path.join('peakdatafull',
                                  'VULCAN_192245_CalMasked_384banks')
    LoadNexusProcessed(Filename=cc_focus_nexus, OutputWorkspace=cc_focus_ws_name)

    # source and target calibration file
    cc_calib_file = os.path.join('peakdatafull', 'VULCAN_192245_Calibration_CC.h5')

    final_calib_file = 'TestHybrid.h5'
    output_dir = os.path.join(os.getcwd(), 'temp')

    # tube group plan
    tube_grouping_plan = [(0, 512, 81920), (81920, 1024, 81920 * 2), (81920 * 2, 256, 200704)]

    # calibrate
    output = peak_position_calibrate(cc_focus_ws_name, tube_grouping_plan, cc_calib_file, final_calib_file, output_dir)
    print(f'Writing calibration file to {output}')


def test_single_spectrum_peak_fitting():
    """Test peak fitting methods

    Returns
    -------

    """

    # Set up
    cc_focus_ws_name = 'FocusedDiamond'
    cc_focus_nexus = os.path.join('peakdatafull',
                                  'VULCAN_192245_CalMasked_384banks.nxs')
    LoadNexusProcessed(Filename=cc_focus_nexus, OutputWorkspace=cc_focus_ws_name)

    output_dir = os.path.join(os.getcwd(), 'temp')

    # Fit west bank
    bank_1_num_tubes = 160
    bank_2_num_tubes = 80
    bank_5_num_tubes = 144
    # Define the pixel (workspace range)
    bank_group_index_ranges = {'Bank1': (0, bank_1_num_tubes),
                               'Bank2': (bank_1_num_tubes, bank_1_num_tubes + bank_2_num_tubes),
                               'Bank5': (bank_1_num_tubes + bank_2_num_tubes,
                                         bank_1_num_tubes + bank_2_num_tubes + bank_5_num_tubes)}

    tube_res_list = list()
    for bank_name in bank_group_index_ranges:
        # get workspace range information
        start_group_index, end_group_index = bank_group_index_ranges[bank_name]
        # fit
        tube_i_res = fit_diamond_peaks(cc_focus_ws_name, start_group_index, end_group_index, output_dir)
        print(f'{bank_name}: Tube number: {len(tube_i_res)}')
        # append
        tube_res_list.extend(tube_i_res)
    # END-FOR

    print(f'Tube number: {len(tube_res_list)}')

    # for i_tube in range(num_tubes):
    #     print(f'Tube {i_tube}:  Number fitted peaks = {tube_res_list[i_tube].num_points}')

    # Prepare plotting
    vec_tube_index = list()
    vec_pos = list()
    vec_fwhm = list()
    vec_calibrated_pos = list()

    plot_peak_pos = 1.07577

    for i_tube in range(len(tube_res_list)):
        # Find a peak to plot
        peak_pos_vec = tube_res_list[i_tube].vec_x
        num_peaks = len(peak_pos_vec)
        peak_index = -1
        for ipeak in range(num_peaks - 1, -1, -1):
            if abs(peak_pos_vec[peak_index] - plot_peak_pos) / plot_peak_pos < 0.2:
                peak_index = ipeak
                break
        # not found
        if peak_index < 0:
            continue

        # append tube index
        vec_tube_index.append(i_tube)
        # append peak position
        vec_pos.append(peak_pos_vec[peak_index])
        # append peak width
        vec_fwhm.append(tube_res_list[i_tube].vec_width[peak_index])
        # calculate model
        exp_pos = tube_res_list[i_tube].optimized_d_vec[peak_index]
        vec_calibrated_pos.append(exp_pos)
    # convert all to vector
    vec_tube_index = np.array(vec_tube_index)
    vec_pos = np.array(vec_pos)
    vec_fwhm = np.array(vec_fwhm)
    vec_calibrated_pos = np.array(vec_calibrated_pos)
    print(f'Number of data points = {len(vec_calibrated_pos)}')

    # Plot
    import time
    time.sleep(0.5)
    plt.cla()
    plt.figure(figsize=(40, 30))
    plt.errorbar(vec_tube_index, vec_pos, vec_fwhm, linestyle='None', color='black', marker='o')
    plt.plot(vec_tube_index, vec_calibrated_pos, linestyle='None', color='red', marker='D', label='Expected')
    plt.legend()
    plt.savefig('raw_positions_vs_tube.png')


if __name__ == '__main__':
    # main()
    # test_bank_wise_calibration()
    # test_single_spectrum_peak_fitting()
    test_bank_wise_calibration()
