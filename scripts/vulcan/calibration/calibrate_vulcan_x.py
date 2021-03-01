# This is the workflow script to calibrate VULCAN-X
from vulcan.calibration.calibrate_vulcan_cross_correlation import (calibrate_vulcan, load_event_data)
from .check_calibration_alignment import reduce_calibration, make_group_workspace
from .peak_position_calibration_step1 import fit_diamond_peaks, apply_peaks_positions_calibration
from .lib_cross_correlation import CrossCorrelateParameter
from mantid.simpleapi import LoadDiffCal, SaveDiffCal, mtd, LoadNexusProcessed
import os
from typing import List, Union, Tuple, Dict
from matplotlib import pyplot as plt
import numpy as np


__all__ = ['load_diamond_runs', 'cross_correlate_calibrate', 'align_vulcan_data']


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
                              vulcan_x_idf: Union[str, None] = None) -> Tuple[str, str]:
    """

    Default VULCAN-X: when auto reducing, time focus data to pixel 48840 for bank 1 and 2, and 422304 for bank 5.
    those are centers.

    Parameters
    ----------
    diamond_runs: str, ~list
        2 possible inputs: (1) name of diamond MatrixWorkspace (2) list of diamond run numbers or nexus file paths
    cross_correlate_param_dict: ~dict
        cross correlation parameters
    bank_calibration_flag: ~dict, None
        flag to calibrate components
    output_dir
    vulcan_x_idf

    Returns
    -------
    ~tuple
        name of calibration file (generated), name of raw diamond EventWorkspace

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
        cross_correlate_param_dict = default_cross_correlation_setup()

    # Set up process calibration
    bank_calibration_flag = process_calibration_flag(cross_correlate_param_dict, bank_calibration_flag)

    # Do cross correlation calibration
    cal_file_name, diamond_ws_name = calibrate_vulcan(diamond_ws_name,
                                                      cross_correlate_param_dict,
                                                      bank_calibration_flag,
                                                      output_dir=output_dir)

    return cal_file_name, diamond_ws_name


def default_cross_correlation_setup() -> Dict[str, CrossCorrelateParameter]:
    # peak position in d-Spacing
    bank1_cc_param = CrossCorrelateParameter('Bank1', reference_peak_position=1.2614, reference_peak_width=0.04,
                                             reference_ws_index=40704, cross_correlate_number=80,
                                             bin_step=-0.0003, start_ws_index=0, end_ws_index=512 * 160)
    bank2_cc_param = CrossCorrelateParameter('Bank2', reference_peak_position=1.2614, reference_peak_width=0.04,
                                             reference_ws_index=40704, cross_correlate_number=80,
                                             bin_step=-0.0003, start_ws_index=512 * 160, end_ws_index=512 * 320)
    bank5_cc_param = CrossCorrelateParameter('Bank3', reference_peak_position=1.07577, reference_peak_width=0.01,
                                             reference_ws_index=182528, cross_correlate_number=20,
                                             bin_step=-0.0003, start_ws_index=512 * 320, end_ws_index=512 * (320 + 72))

    # do cross correlation
    cross_correlate_param_dict = {'Bank1': bank1_cc_param,
                                  'Bank2': bank2_cc_param,
                                  'Bank5': bank5_cc_param}

    return cross_correlate_param_dict


def process_calibration_flag(cross_correlate_param_dict: Dict[str, CrossCorrelateParameter],
                             bank_calibration_flag: Union[Dict[str, bool], None]) -> Dict[str, bool]:

    if bank_calibration_flag is None:
        # default: all True
        bank_calibration_flag = dict()
        for module_name in cross_correlate_param_dict:
            bank_calibration_flag[module_name] = True

    elif len(bank_calibration_flag) < len(cross_correlate_param_dict):
        # fill in blanks
        flags = bank_calibration_flag.values()
        flag_sum = np.sum(np.array(flags))
        if flag_sum == 0:
            # user specifies False
            default_flag = True
        elif flag_sum != len(flags):
            # user specifies both True and False
            raise RuntimeError(f'User specifies both True and False but not all the components.'
                               f'It causes confusion')
        else:
            default_flag = False

        # fill in the default values
        for component_name in cross_correlate_param_dict:
            if component_name not in bank_calibration_flag:
                bank_calibration_flag[component_name] = default_flag

    return bank_calibration_flag


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


def main(step=1):
    # --------------------------------------------------------------------------
    # User setup
    # 
    #
    # Testing files
    # diamond_run = ['/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192227.nxs.h5',
    #                '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192228.nxs.h5',
    #                '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192229.nxs.h5',
    #                '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192230.nxs.h5'][:]

    # day 1: diamond_runs = [192227, 192228]
    # latest
    diamond_runs = [192245, 192246, 192247, 192248][0:1]

    # Optional user specified Mantid IDF
    # vulcan_x_idf = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/data/VULCAN_Definition_pete02.xml'
    vulcan_x_idf = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/data/VULCAN_Definition.xml'

    # Output path
    output_dir = os.path.join(os.getcwd(), 'temp')
    final_calib_file = 'VULCAN_Calibration_Hybrid.h5'
    #
    # ---------------------------------------------------------------------------

    # Load data (set)
    diamond_ws_name, _ = load_diamond_runs(diamond_runs, vulcan_x_idf, output_dir)
                              

    # Step 1: do cross correlation calibration

    # set up a test cross  correlation plan
    tube_cc_plan = cross_correlation_in_tubes()

    # cross correlation
    cc_calib_file, diamond_ws_name = cross_correlate_calibrate(diamond_ws_name,
                                                               cross_correlate_param_dict = tube_cc_plan,
                                                               output_dir=output_dir)

    if step == 1:
        print(f'Returned after cross correlation')
        return

    # Define group plan as a check for step 1 and a plan for step 3
    tube_grouping_plan = [(0, 512, 81920), (81920, 1024, 81920 * 2), (81920 * 2, 256, 200704)]

    # Step 2: use the calibration file generated from previous step to align diamond runs
    cc_focus_ws_name, cc_focus_nexus = align_vulcan_data(diamond_runs=diamond_ws_name,
                                                         diff_cal_file_name=cc_calib_file,
                                                         output_dir=output_dir,
                                                         tube_grouping_plan=tube_grouping_plan)
    if step == 2:
        print(f'Returned after aligning detector with CC-calibrated')
        return

    # Step 3: do peak position calibration
    peak_position_calibrate(cc_focus_ws_name, tube_grouping_plan, cc_calib_file, final_calib_file, output_dir)

    # use the calibration file generated from last step to align diamond runs again


def main_check_calibration_quality():
    """ Check calibration quality
    """
    # Calibration file to check
    diff_cal_file = os.path.join(os.getcwd(), 'TestHybrid210222X.h5')

    # Output
    output_dir = os.path.join(os.getcwd(), 'temp')

    # latest
    diamond_runs = [192245, 192246, 192247, 192248][0:1]

    # Optional user specified Mantid IDF
    vulcan_x_idf = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/data/VULCAN_Definition.xml'

    # Define group plan as a check for step 1 and a plan for step 3
    tube_grouping_plan = [(0, 512, 81920), (81920, 512, 81920 * 2), (81920 * 2, 512, 200704)]

    # No-user touch below this line
    # Step 1: Load data (set)
    diamond_ws_name, _ = load_diamond_runs(diamond_runs, vulcan_x_idf, output_dir)

    # Step 2: use the calibration file generated from previous step to align diamond runs
    # focused_ws_name, focused_nexus =  ...
    align_vulcan_data(diamond_runs=diamond_ws_name, diff_cal_file_name=diff_cal_file,
                      output_dir=output_dir, tube_grouping_plan=tube_grouping_plan)

    # Step 3: check qualities (peak fitting with tube)
    pass  # not implemented yet


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
    print(f'Output directory = {output_dir}')

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


def cross_correlation_in_tubes():
    # peak position in d-Spacing
    # Tube 1
    tube1_cc_param = CrossCorrelateParameter('Tube1', reference_peak_position=1.2614, reference_peak_width=0.04,
                                             reference_ws_index=256,
                                             cross_correlate_number=80,
                                             bin_step=-0.0003, start_ws_index=0, end_ws_index=512)
    boundary_index = 512
    # Rest of 8-Pack 1:center = 2 * 512 + 256
    pack1_cc_param = CrossCorrelateParameter('Bank1', reference_peak_position=1.2614, reference_peak_width=0.04,
                                             reference_ws_index=2 * 512 + 256,
                                             cross_correlate_number=80,
                                             bin_step=-0.0003, start_ws_index=boundary_index, end_ws_index=512 * 4)
    boundary_index = 512 * 4
    # Rest of bank 1
    bank1_cc_param = CrossCorrelateParameter('Bank1', reference_peak_position=1.2614, reference_peak_width=0.04,
                                             reference_ws_index=40704, cross_correlate_number=80,
                                             bin_step=-0.0003, start_ws_index=boundary_index, end_ws_index=512 * 160)
    # Bank 2
    bank2_cc_param = CrossCorrelateParameter('Bank2', reference_peak_position=1.2614, reference_peak_width=0.04,
                                             reference_ws_index=40704, cross_correlate_number=80,
                                             bin_step=-0.0003, start_ws_index=512 * 160, end_ws_index=512 * 320)
    # Bank 5
    bank5_cc_param = CrossCorrelateParameter('Bank3', reference_peak_position=1.07577, reference_peak_width=0.01,
                                             reference_ws_index=182528, cross_correlate_number=20,
                                             bin_step=-0.0003, start_ws_index=512 * 320, end_ws_index=512 * (320 + 72))

    # do cross correlation
    cross_correlate_param_dict = {'Tube1': tube1_cc_param,
                                  'Pack1': pack1_cc_param,
                                  'Bank1': bank1_cc_param,
                                  'Bank2': bank2_cc_param,
                                  'Bank5': bank5_cc_param}

    return cross_correlate_param_dict


if __name__ == '__main__':
    main(step=2)
    # test_bank_wise_calibration()
    # test_single_spectrum_peak_fitting()
    # test_bank_wise_calibration()
    # main_check_calibration_quality()
