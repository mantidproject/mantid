from mantid_helper import mtd_convert_units, load_nexus, load_grouping_file, load_calibration_file
from lib_cross_correlation import (cross_correlate_vulcan_data,
                                   save_calibration, export_difc)
from lib_analysis import check_correct_difcs
import os
from mantid.api import AnalysisDataService as mtd
from mantid.simpleapi import CreateGroupingWorkspace


def load_data(nexus_path, test_mode):

    # load data
    diamond_ws_name = os.path.basename(nexus_path).split('.')[0] + '_diamond'
    print(f'[INFO] Loading {nexus_path} to {diamond_ws_name}')

    # Test mode
    test_arg = {}
    if test_mode:
        test_arg['max_time'] = 300

    load_nexus(data_file_name=nexus_path,
               output_ws_name=diamond_ws_name,
               meta_data_only=False,
               **test_arg)

    # convert to d-Spacing
    mtd_convert_units(diamond_ws_name, 'dSpacing')

    return diamond_ws_name


def load_detector_grouping(group_ws_name,
                           grouping_file: str = None,
                           ref_cal_h5: str = None):
    if mtd.doesExist(group_ws_name):
        return group_ws_name

    if grouping_file:
        group_ws_name = f'{os.path.basename(grouping_file).split(".")[0]}_grouping'
        load_grouping_file(grouping_file_name=grouping_file, grouping_ws_name=group_ws_name)
    elif ref_cal_h5:
        # using reference calibration file for grouping workspace
        base_name = os.path.basename(ref_cal_h5.split(".")[0])
        ref_ws_name = f'{base_name}_ref_calib'
        outputs, ref_offset_ws = load_calibration_file(ref_cal_h5, ref_ws_name, diamond_ws_name,
                                                                     load_cal=False)
        group_ws = outputs.OutputGroupingWorkspace
        group_ws_name = group_ws.name()
    else:
        # not specified
        CreateGroupingWorkspace(InputWorkspace='vulcan_diamond', OutputWorkspace=group_ws_name)

    return group_ws_name


def calibrate_vulcan(diamond_nexus, test_mode, difc_file_name):
    # main calibration workflow algorithm
    # refer to pyvdrive.script.calibration.vulcan_cal_instruent_calibration.py

    # Load data
    diamond_ws_name = load_data(diamond_nexus, test_mode)

    # load grouping workspace
    grouping_ws_name = load_detector_grouping()

    # do cross correlation:
    if True:
        calib_flag = 'west'
        cross_correlate_vulcan_data(diamond_ws_name, grouping_ws_name, calib_flag, fit_time=1, flag='1fit')
    else:
        # This optiona may not work in the current Mantid master/ornl-next yet
        cross_correlate_vulcan_data(diamond_ws_name, group_ws_name, fit_time=2, flag='2fit')

    # About output
    calib_offset_ws_name = 'vulcan_diamond_2fit_offset'

    # Export cross correlated result, DIFC and etc for analysis
    # check the difference between DIFCs
    check_correct_difcs(ws_name='vulcan_diamond')

    # save calibration file
    save_calibration(offset_ws_name=calib_offset_ws_name,
                     mask_ws='vulcan_diamond_2fit_mask',
                     group_ws_name=None,
                     calib_ws_name='vulcan_diamond',
                     num_groups=num_banks,
                     calib_file_prefix='blabla')

    # save difc file
    if difc_file_name:
        export_difc(offset_ws=calib_offset_ws_name, file_name=difc_file_name)


def test_main():
    pass


if __name__ == '__main__':
    test_main()
