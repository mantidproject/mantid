from mantid_helper import mtd_convert_units, load_nexus
from lib_cross_correlation import (cross_correlate_vulcan_data,
                                   verify_vulcan_difc,
                                   save_calibration, merge_detector_calibration)
import os
from mantid.simpleapi import (CreateGroupingWorkspace, LoadEventNexus, Plus,
                              ConvertToMatrixWorkspace,
                              SaveNexusProcessed, LoadNexusProcessed, mtd,
                              DeleteWorkspace, LoadInstrument)
from typing import Union, List, Tuple


# Cross correlation algorithm setup
# TODO FIXME - this can be 2
CROSS_CORRELATE_PEAK_FIT_NUMBER = 1


def load_event_data(nexus_paths: List[str],
                    cutoff_time: Union[int, None] = None,
                    counts_nxs_name: Union[str, None] = None,
                    unit_dspace: bool = True,
                    idf_name: Union[str, None] = None) -> str:
    """Load event file

    Parameters
    ----------
    nexus_paths: ~list
        List of nexus files
    cutoff_time: int
        max time to load in seconds
    counts_nxs_name: str
        Path for OUTPUT processed nexus file containing counts of each spectrum
    unit_dspace: bool
        Flag to convert the workspace unit to dSpacing
    idf_name: str, None
        Mantid IDF to override the geometry from NeXus file

    Returns
    -------
    str
        EventWorkspace name

    """
    assert isinstance(nexus_paths, list) and not isinstance(nexus_paths, str), f'Nexus paths {nexus_paths}' \
                                                                               f' must be a list'

    # load data
    diamond_ws_name = os.path.basename(nexus_paths[0]).split('.')[0] + '_diamond'
    print(f'[INFO] Loading {nexus_paths} to {diamond_ws_name}')

    # Load a file or files
    if len(nexus_paths) == 1:
        # Load a single file
        # tet mode?
        test_arg = {}
        if cutoff_time:
            test_arg['max_time'] = cutoff_time
        # load
        load_nexus(data_file_name=nexus_paths[0],
                   output_ws_name=diamond_ws_name,
                   meta_data_only=False,
                   **test_arg)
    else:
        # Load a series of data files
        LoadEventNexus(Filename=nexus_paths[0], OutputWorkspace=diamond_ws_name)
        # Load more files
        for file_index in range(1, len(nexus_paths)):
            dia_wksp_i = "%s_diamond_%d" % ('VULCANX', file_index)
            LoadEventNexus(Filename=nexus_paths[file_index], OutputWorkspace=dia_wksp_i)
            Plus(LHSWorkspace=diamond_ws_name,
                 RHSWorkspace=dia_wksp_i,
                 OutputWorkspace=diamond_ws_name,
                 ClearRHSWorkspace=True)

    if idf_name:
        print(f'[CHECK 1] number of histograms = {mtd[diamond_ws_name].getNumberHistograms()},'
              f'Spectrum 1 Detector = {mtd[diamond_ws_name].getDetector(0).getID()}')
        # Reload instrument (do not trust the old one)
        LoadInstrument(Workspace=diamond_ws_name,
                       Filename=idf_name,
                       InstrumentName='VULCAN',
                       RewriteSpectraMap=False) 
        print(f'[CHECK 2] number of histograms = {mtd[diamond_ws_name].getNumberHistograms()},'
              f'Spectrum 1 Detector = {mtd[diamond_ws_name].getDetector(0).getID()}')

    # convert to d-Spacing
    if unit_dspace:
        mtd_convert_units(diamond_ws_name, 'dSpacing')

    # Save counts
    if counts_nxs_name:
        counts_ws_name = f'{diamond_ws_name}_counts'
        ConvertToMatrixWorkspace(InputWorkspace=diamond_ws_name, OutputWorkspace=counts_ws_name)
        SaveNexusProcessed(InputWorkspace=counts_ws_name, Filename=counts_nxs_name)
        DeleteWorkspace(Workspace=counts_ws_name)

    return diamond_ws_name


def create_groups(vulcan_ws_name=None) -> str:
    """Create group workspace
    """
    # create group workspace
    group_ws_name = 'VULCAN_3Bank_Groups'

    # 3 group mode
    if vulcan_ws_name is None:
        group_ws = CreateGroupingWorkspace(InstrumentName='vulcan',
                                           GroupDetectorsBy='Group',
                                           OutputWorkspace=group_ws_name)
    else:
        group_ws = CreateGroupingWorkspace(InputWorkspace=vulcan_ws_name,
                                           GroupDetectorsBy='bank', 
                                           OutputWorkspace=group_ws_name)

    # sanity check
    assert group_ws

    return group_ws_name


def calibrate_vulcan(diamond_nexus: List[str],
                     output_calib_file_name='VULCAN_Calibration_CC',   # FIXME - this is prefix!
                     load_cutoff_time: Union[None, int] = None,
                     user_idf: Union[None, str] = None) -> Tuple[str, str]:
    """Main calibration workflow algorithm

    Refer to pyvdrive.script.calibration.vulcan_cal_instruent_calibration.py

    Parameters
    ----------
    diamond_nexus: str
        path to diamond Nexus file
    output_calib_file_name: str:
        Path to the output calibration file (.h5)
    load_cutoff_time: int, None
        maximum relative time to load in second
    user_idf: str, None
        if given, load user provided IDF to EventWorksapce loaded from diamond nexus

    Returns
    -------
    tuple
        calibration file name, diamond event workspace

    """
    # TODO - VULCAN-X: when auto reducing, time focus data to pixel 48840 for bank 1 and 2, and 422304 for bank 5.
    # TODO  -          those are centers.

    # Load data and convert unit to dSpacing
    print(f'[DEBUG] diamond_nexus: {diamond_nexus} of type {type(diamond_nexus)}')
    count_ws_name = f'{os.path.basename(diamond_nexus[0]).split(".")[0]}_counts.nxs'
    diamond_ws_name = load_event_data(diamond_nexus,
                                      load_cutoff_time,
                                      counts_nxs_name=count_ws_name,
                                      unit_dspace=True,
                                      idf_name=user_idf)

    # do cross correlation:
    calib_flag = {'Bank1': True, 'Bank2': True, 'Bank5': True}
    r = cross_correlate_vulcan_data(diamond_ws_name, calib_flag,
                                    cc_fit_time=CROSS_CORRELATE_PEAK_FIT_NUMBER,
                                    prefix='1fit')
    print(f'Type of returned value from cross_correlate_vulcan_data: {type(r)}')
    offset_ws_dict, mask_ws_dict = r

    # About output
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
    grouping_ws_name = create_groups(diamond_ws_name)

    calib_file_name = save_calibration(calib_ws_name=calib_ws_name,
                                       mask_ws_name=str(mask_ws),
                                       group_ws_name=grouping_ws_name,
                                       calib_file_prefix=output_calib_file_name)

    # Align the diamond workspace 
    # very diamond workspace
    # TODO FIXME - remove this if-else-block afterwards
    if mtd.doesExist(diamond_ws_name):
        diamond_ws = mtd[diamond_ws_name]
        print(f'Workspace: {diamond_ws_name} type = {type(diamond_ws)} Histograms = {diamond_ws.getNumberHistograms()}')
    else:
        print(f'Workspace: {diamond_ws_name} is deleted')
    # END-IF-ELSE

    return calib_file_name, diamond_ws_name


def test_main_calibrate():
    # Testing files
    diamond_run = ['/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192227.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192228.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192229.nxs.h5',
                   '/SNS/VULCAN/IPTS-26807/nexus/VULCAN_192230.nxs.h5']
    # 
    vulcan_x_idf = '/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration/data/VULCAN_Definition_pete02.xml'

    calibrate_vulcan(diamond_nexus=diamond_run[:],
                     load_cutoff_time=None,
                     user_idf=vulcan_x_idf,
                     )

    # Align the event workspace (todo)


if __name__ == '__main__':
    test_main_calibrate()
