from mantid_helper import mtd_convert_units, load_nexus, load_calibration_file
from lib_cross_correlation import (cross_correlate_vulcan_data,
                                   verify_vulcan_difc,
                                   save_calibration, merge_detector_calibration)
from lib_analysis import (align_focus_event_ws)
import os
from mantid.simpleapi import (CreateGroupingWorkspace,
                              ConvertToMatrixWorkspace,
                              SaveNexusProcessed, LoadNexusProcessed,
                              DeleteWorkspace)
from typing import Union


def load_event_data(nexus_path,
                    cutoff_time: int = 300,
                    counts_nxs_name: Union[str, None] = None,
                    unit_dspace: bool = True) -> str:
    """Load event file

    Parameters
    ----------
    nexus_path
    cutoff_time: int
        max time to load in seconds
    counts_nxs_name: str
        Path for OUTPUT processed nexus file containing counts of each spectrum

    Returns
    -------
    str
        EventWorkspace name

    """

    # load data
    diamond_ws_name = os.path.basename(nexus_path).split('.')[0] + '_diamond'
    print(f'[INFO] Loading {nexus_path} to {diamond_ws_name}')

    # Test mode
    test_arg = {}
    if cutoff_time:
        test_arg['max_time'] = cutoff_time

    # Load file
    load_nexus(data_file_name=nexus_path,
               output_ws_name=diamond_ws_name,
               meta_data_only=False,
               **test_arg)

    # convert to d-Spacing
    if unit_dspace:
        mtd_convert_units(diamond_ws_name, 'dSpacing')

    # Save counts
    if counts_nxs_name:
        counts_ws_name = f'{diamond_ws_name}_counts'
        ConvertToMatrixWorkspace(diamond_ws_name, counts_ws_name)
        SaveNexusProcessed(InputWorkspace=counts_ws_name, Filename=counts_nxs_name)
        DeleteWorkspace(Workspace=counts_ws_name)

    return diamond_ws_name


def create_groups() -> str:
    """Create group workspace
    """
    # create group workspace
    group_ws_name = 'VULCAN_3Bank_Groups'

    # 3 group mode
    group_ws = CreateGroupingWorkspace(InstrumentName='vulcan',
                                       GroupDetectorsBy='Group',
                                       OutputWorkspace=group_ws_name)

    # sanity check
    assert group_ws

    return group_ws_name


def calibrate_vulcan(diamond_nexus: str,
                     load_cutoff_time: Union[None, int],
                     difc_file_name: Union[None, str]):
    """Main calibration workflow algorithm

    Refer to pyvdrive.script.calibration.vulcan_cal_instruent_calibration.py

    Parameters
    ----------
    diamond_nexus: str
        path to diamond Nexus file
    load_cutoff_time: int, None
        maximum relative time to load in second
    difc_file_name: str, None
        path to output DIFC

    Returns
    -------

    """
    # NOTE TODO FIXME - VULCAN-X: when auto reducing, time focus data to pixel 48840 for bank 1 and 2, and 422304 for bank 5. those are centers.


    # TODO FIXME - this can be 2
    CROSS_CORRELATE_PEAK_FIT_NUMBER = 1

    # Load data
    diamond_ws_name = load_event_data(diamond_nexus, load_cutoff_time)

    # do cross correlation:
    calib_flag = {'west': True, 'east': True, 'high angle': True}
    r = cross_correlate_vulcan_data(diamond_ws_name, calib_flag,
                                    cc_fit_time=CROSS_CORRELATE_PEAK_FIT_NUMBER,
                                    prefix='1fit')
    print(f'Type of returned value from cross_correlate_vulcan_data: {type(r)}')
    offset_ws_dict, mask_ws_dict = r

    # About output
    # TODO - this can be more flexible
    base_output_ws_name = f'VULCAN_Calibration_{CROSS_CORRELATE_PEAK_FIT_NUMBER}Fit'

    # Merge calibration and masks
    rt = merge_detector_calibration(ref_calib_ws=None,
                                    ref_mask_ws=None,
                                    offset_ws_dict=offset_ws_dict,
                                    mask_ws_dict=mask_ws_dict,
                                    num_banks=3,
                                    output_ws_name=base_output_ws_name)
    calib_ws_name, offset_ws, mask_ws = rt

    # Check, mask or fallback the difference between calibrated and engineered DIFCs
    verify_vulcan_difc(ws_name=diamond_ws_name,
                       cal_table_name=calib_ws_name,
                       mask_ws_name=str(mask_ws),
                       fallback_incorrect_difc_pixels=False,
                       mask_incorrect_difc_pixels=True)

    # merge calibration result from bank-based cross correlation and  save calibration file
    # Export cross correlated result, DIFC and etc for analysis
    # Generate grouping workspace
    grouping_ws_name = create_groups()

    save_calibration(calib_ws_name=calib_ws_name,
                     mask_ws_name=str(mask_ws),
                     group_ws_name=grouping_ws_name,
                     calib_file_prefix='vulcan_cc_1fit')


def test_main_report_calibration():
    """(Testing) main to import a calibration file and make reports
    """
    # Set up
    # calib_file = 'pdcalibration/VULCAN_pdcalibration.h5'
    calib_file = 'vulcan_cc_1fit.h5'
    counts_file = 'VULCAN_164960_matrix.nxs'

    # Load counts for each pixel
    diamond_count_ws = LoadNexusProcessed(Filename=counts_file, OutputWorkspace='Diamond_Counts')
    # counts_vec = diamond_count_ws.extractY().sum(axis=1).flatten()

    # Load calibration file
    calib_ws_tuple = load_calibration_file(calib_file,
                                           output_name='VULCAN_calibration',
                                           ref_ws_name=str(diamond_count_ws))

    # FIXME - This is hard coded and bound to VULCAN but not VULCAN-X
    pixels_range = {'west': (0, 3234),
                    'east': (3234, 6468),
                    'high angle': (6468, None)}

    # Report offsets
    verify_vulcan_difc(ws_name=str(diamond_count_ws),
                       cal_table_name=str(calib_ws_tuple.OutputCalWorkspace),
                       mask_ws_name=str(calib_ws_tuple.OutputMaskWorkspace),
                       fallback_incorrect_difc_pixels=False,
                       mask_incorrect_difc_pixels=False)

    # Report masks
    from lib_analysis import report_masked_pixels
    for bank in ['west', 'east', 'high angle']:
        print(f'[MASK] Bank {bank}')
        report = report_masked_pixels(diamond_count_ws, calib_ws_tuple.OutputMaskWorkspace,
                                      pixels_range[bank][0], pixels_range[bank][1])
        print(report)


def test_main_apply_calibration():
    # Load raw workspace
    # Load data
    # diamond_nexus = '/SNS/VULCAN/IPTS-21356/nexus/VULCAN_164960.nxs.h5'
    diamond_nexus = '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192230.nxs.h5'
    load_cutoff_time = None   # seconds
    diamond_ws_name = load_event_data(diamond_nexus, load_cutoff_time, unit_dspace=False)

    # Specify a specific calibration file
    # Round 1:
    # calib_file_name = 'vulcan_cc_1fit.h5'
    # calib_file_name = 'VULCAN_pdcalibration.h5'
    calib_file_name='VULCAN_calibration_pre_beta.h5'

    # Round 2:
    # calib_file_name = 'VULCAN_calibration_pd2.h5'

    # Load calibration file
    calib_tuple = load_calibration_file(calib_file_name, 'VulcanX_PD_Calib', diamond_ws_name)
    calib_cal_ws = calib_tuple.OutputCalWorkspace
    calib_group_ws = calib_tuple.OutputGroupingWorkspace
    calib_mask_ws = calib_tuple.OutputMaskWorkspace

    # Align, focus and export
    align_focus_event_ws(diamond_ws_name, str(calib_cal_ws), str(calib_group_ws), str(calib_mask_ws))


def test_main_calibrate():
    calibrate_vulcan(diamond_nexus='/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192230.nxs.h5',
                     load_cutoff_time=None,  # 300
                     difc_file_name='VULCAN_calibration_cc_beta.h5')


if __name__ == '__main__':
    choice = '3'
    if choice == '1':
        test_main_calibrate()
    elif choice == '2':
        test_main_report_calibration()
    elif choice == '3':
        test_main_apply_calibration()
    else:
        raise NotImplementedError(f'Workspace choice {choice} is not supported')
