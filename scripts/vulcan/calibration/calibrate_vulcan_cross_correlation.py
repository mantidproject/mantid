from mantid_helper import mtd_convert_units, load_nexus
from lib_cross_correlation import (CrossCorrelateParameter, cross_correlate_vulcan_data,
                                   verify_vulcan_difc,
                                   save_calibration, merge_detector_calibration)
import os
from mantid.simpleapi import (CreateGroupingWorkspace, LoadEventNexus, Plus,
                              ConvertToMatrixWorkspace,
                              SaveNexusProcessed, LoadNexusProcessed, mtd,
                              DeleteWorkspace, LoadInstrument)
from typing import Union, List, Tuple, Dict


# Cross correlation algorithm setup
# TODO FIXME - this can be 2
CROSS_CORRELATE_PEAK_FIT_NUMBER = 1


def load_event_data(nexus_paths: List[Union[str, int]],
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
    def load_nexus_run(nexus_run):
        if isinstance(nexus_run, int):
            # user provide run number
            wksp = "%s_%d" % ('VULCAN', nexus_run)
            file_name = wksp
        else:
            # user provide file name
            wksp = os.path.basename(nexus_run).split('.')[0]
            file_name = nexus_run
        LoadEventNexus(Filename=file_name, OutputWorkspace=wksp, NumberOfBins=1)

        return wksp

    # Check input (not very Python)
    assert isinstance(nexus_paths, list) and not isinstance(nexus_paths, str), f'Nexus paths {nexus_paths}' \
                                                                               f' must be a list'

    # Load a file or files
    if len(nexus_paths) == 1 and cutoff_time:
        # Load a single file
        if isinstance(nexus_paths[0], int):
            raise RuntimeError(f'Single run with cutoff time does not support run-number-only input.')
        # determine diamone workspace name
        diamond_ws_name = os.path.basename(nexus_paths[0]).split('.')[0] + '_diamond'
        print(f'[INFO] Loading {nexus_paths} to {diamond_ws_name}')
        # and allow test mode?
        test_arg = {}
        test_arg['max_time'] = cutoff_time
        # load
        load_nexus(data_file_name=nexus_paths[0],
                   output_ws_name=diamond_ws_name,
                   meta_data_only=False,
                   **test_arg)
    else:
        # Load a series of data files
        diamond_ws_name = load_nexus_run(nexus_paths[0])

        # Load more files
        for file_index in range(1, len(nexus_paths)):
            dia_wksp_i = load_nexus_run(nexus_paths[file_index])
            Plus(LHSWorkspace=diamond_ws_name,
                 RHSWorkspace=dia_wksp_i,
                 OutputWorkspace=diamond_ws_name,
                 ClearRHSWorkspace=True)

    # Load IDF if the one in Nexus shall be replaced
    if idf_name:
        # Reload instrument (do not trust the old one)
        LoadInstrument(Workspace=diamond_ws_name,
                       Filename=idf_name,
                       InstrumentName='VULCAN',
                       RewriteSpectraMap=False)

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
        #
        raise RuntimeError('This is for VULCAN-NOT-X.')
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


def calibrate_vulcan(diamond_ws_name: str,
                     cross_correlate_param_dict: Dict[str, CrossCorrelateParameter],
                     calibration_flag: Dict[str, bool],
                     output_dir: str) -> Tuple[str, str]:
    """Main calibration workflow algorithm

    Refer to pyvdrive.script.calibration.vulcan_cal_instruent_calibration.py

    Parameters
    ----------
    diamond_ws_name:
    output_dir:

    Returns
    -------
    tuple
        calibration file name, diamond event workspace

    """
    # Check inputs
    assert mtd.doesExist(diamond_ws_name), f'Workspace {diamond_ws_name} does not exist'

    # Call workflow algorithm to calibrate VULCAN data by cross-correlation
    r = cross_correlate_vulcan_data(diamond_ws_name,
                                    cross_correlate_param_dict,
                                    calibration_flag,
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
                       mask_incorrect_difc_pixels=True,
                       output_dir=output_dir)

    # merge calibration result from bank-based cross correlation and  save calibration file
    # Export cross correlated result, DIFC and etc for analysis
    # Generate grouping workspace
    grouping_ws_name = create_groups(diamond_ws_name)

    output_calib_file_name = f'{diamond_ws_name}_Calibration_CC'
    calib_file_name = save_calibration(calib_ws_name=calib_ws_name,
                                       mask_ws_name=str(mask_ws),
                                       group_ws_name=grouping_ws_name,
                                       calib_file_prefix=output_calib_file_name,
                                       output_dir=output_dir)

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
