from mantid_helper import mtd_convert_units, load_nexus, load_grouping_file, load_calibration_file
from lib_cross_correlation import (cross_correlate_vulcan_data,
                                   save_calibration, export_difc, merge_detector_calibration)
from lib_analysis import check_and_correct_difc
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


def load_detector_grouping(ref_ws_name,
                           group_ws_name,
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
        CreateGroupingWorkspace(InputWorkspace=ref_ws_name, OutputWorkspace=group_ws_name)

    return group_ws_name


def calibrate_vulcan(diamond_nexus, test_mode, difc_file_name):
    # main calibration workflow algorithm
    # refer to pyvdrive.script.calibration.vulcan_cal_instruent_calibration.py

    # Load data
    diamond_ws_name = load_data(diamond_nexus, test_mode)

    # do cross correlation:
    if True:
        calib_flag = {'west': True, 'east': True, 'high angle': True}
        r = cross_correlate_vulcan_data(diamond_ws_name, calib_flag, cc_fit_time=1, prefix='1fit')
        print(f'Type of returned value from cross_correlate_vulcan_data: {type(r)}')
        offset_ws_dict, mask_ws_dict = r
    else:
        # This optiona may not work in the current Mantid master/ornl-next yet
        cross_correlate_vulcan_data(diamond_ws_name, group_ws_name, cc_fit_time=2, prefix='2fit')

    # About output
    # TODO - this can be more flexible
    output_offset_ws_name = 'vulcan_diamond_2fit_offset'

    # Export cross correlated result, DIFC and etc for analysis
    # load grouping workspace
    grouping_ws_name = load_detector_grouping(diamond_ws_name, 'vulcan_grouping')
    # FIXME - haven't found out how grouping workspace is used for 3 bank case
    # grouping_ws_name = None

    #
    rt = merge_detector_calibration(ref_calib_ws=None,
                                    ref_mask_ws=None,
                                    offset_ws_dict=offset_ws_dict,
                                    mask_ws_dict=mask_ws_dict,
                                    num_banks=3,
                                    output_ws_name='VULCAN_1fit_test')
    calib_ws_name, offset_ws, mask_ws = rt

    # Correct obviously erroneous DIFC
    # check the difference between DIFCs
    if False:
        check_and_correct_difc(ws_name=diamond_ws_name,
                               cal_table_name=calib_ws_name,
                               mask_ws_name=str(mask_ws))

    # merge calibration result from bank-based cross correlation and  save calibration file
    save_calibration(calib_ws_name=calib_ws_name,
                     mask_ws_name=str(mask_ws),
                     group_ws_name=grouping_ws_name,
                     calib_file_prefix='test_pre_x')


def test_main():
    calibrate_vulcan(diamond_nexus='/SNS/VULCAN/IPTS-21356/nexus/VULCAN_164960.nxs.h5', test_mode=True, difc_file_name='testout.h5') 


if __name__ == '__main__':
    test_main()
