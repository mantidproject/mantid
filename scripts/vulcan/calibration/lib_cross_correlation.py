# script to do cross-correlation
import os
import math
from mantid.api import AnalysisDataService as mtd
from mantid.simpleapi import CrossCorrelate, GetDetectorOffsets, ConvertDiffCal, SaveDiffCal
from mantid.simpleapi import Plus, CreateWorkspace
from mantid.simpleapi import CloneWorkspace
from mantid.simpleapi import Rebin
from mantid.simpleapi import GeneratePythonScript, SaveNexusProcessed
import bisect
import numpy as np
import mantid_helper
from lib_analysis import report_masked_pixels
from typing import Dict, Tuple, Any, Union
import h5py


"""
Total = 81920 * 2 + (24900 - 6468) * 2 = 200704
Bank 1: 81920 .. center = 40,960 - ??? ...  40960
Bank 2: 81920 ... center = 61,140  ...  61140
Bank 5: 36864 ... center = 182,272 ...182272
"""
VULCAN_X_PIXEL_RANGE = {'Bank1': (0, 81920),  # 160 tubes
                        'Bank2': (81920, 163840),   # 160 tubes
                        'Bank5': (163840, 200704)   # 72 tubes
                        }


# TODO - all the hardcoded pixel numbers will be replaced!
def verify_vulcan_difc(ws_name: str,
                       cal_table_name: str,
                       mask_ws_name: str,
                       fallback_incorrect_difc_pixels: bool,
                       mask_incorrect_difc_pixels: bool):
    """Verify and possibly correct DIFCs if necessary

    Parameters
    ----------
    ws_name
    cal_table_name: str
         name of Calibration workspace (a TableWorkspace)
    mask_ws_name
    fallback_incorrect_difc_pixels: bool
        If calibrated DIFC is obviously wrong, fallback to the engineered value
    mask_incorrect_difc_pixels: bool
        if fallback_incorrect_difc_pixels==False, choice to discard (aka mask) bad pixels

    Returns
    -------

    """
    # Define const parameterr
    diamond_event_ws = mtd[ws_name]
    mask_ws = mtd[mask_ws_name]
    cal_table_ws = mtd[cal_table_name]
    difc_col_index = 1

    # Generate a dictionary for each bank
    bank_info_dict = VULCAN_X_PIXEL_RANGE
    # bank_info_dict = {'Bank1': (0, 81920),  # 160 tubes
    #                   'Bank2': (81920, 163840),   # 160 tubes
    #                   'Bank5': (163840, 200704)   # 72 tubes
    #                   }

    if diamond_event_ws.getNumberHistograms() != 200704:
        raise NotImplementedError('Bank information dictionary is out of date')

    # Init file
    difc_h5 = h5py.File(f'{diamond_event_ws}_DIFC.h5', 'w')

    for bank_name in ['Bank1', 'Bank2', 'Bank5']:
        # pixel range
        pid_0, pid_f = bank_info_dict[bank_name]
        num_pixels = pid_f - pid_0
        # calculate DIFC from IDF and get calibrated DIFC
        bank_idf_vec = np.ndarray(shape=(num_pixels,), dtype='float')
        bank_cal_vec = np.ndarray(shape=(num_pixels,), dtype='float')
        for irow in range(pid_0, pid_f):
            bank_idf_vec[irow - pid_0] = calculate_difc(diamond_event_ws, irow)
            bank_cal_vec[irow - pid_0] = cal_table_ws.cell(irow, difc_col_index)
        # compare and make report
        difc_diff_vec = bank_cal_vec - bank_idf_vec
        bank_entry = difc_h5.create_group(bank_name)
        # data
        h5_data = np.array([bank_cal_vec, bank_idf_vec, difc_diff_vec]).transpose()
        bank_entry.create_dataset('DIFC_cal_raw_diff', data=h5_data)

        # correct the unphysical (bad) calibrated DIFC to default DIF: Bank1, Bank2, Bank5
        param_dict = {}
        if fallback_incorrect_difc_pixels:
            param_dict['cal_table'] = cal_table_ws
            param_dict['difc_col_index'] = 1

        correct_difc_to_default(bank_idf_vec, bank_cal_vec, difc_tol=20,
                                start_row_number=pid_0,
                                mask_ws=mask_ws, mask_erroneous_pixels=mask_incorrect_difc_pixels,
                                **param_dict)

    # Close file
    difc_h5.close()


def calculate_difc(ws, ws_index):
    """Calculate DIFC of one spectrum

    Parameters
    ----------
    ws:
        workspace instance
    ws_index: int
        workspace index from 0

    Returns
    -------
    float
        DIFC

    """
    det_pos = ws.getDetector(ws_index).getPos()
    source_pos = ws.getInstrument().getSource().getPos()
    sample_pos = ws.getInstrument().getSample().getPos()

    source_sample = sample_pos - source_pos
    det_sample = det_pos - sample_pos
    angle = det_sample.angle(source_sample)

    l1 = source_sample.norm()
    l2 = det_sample.norm()

    # DIFC:  ath.sqrt(L1+L2) #\sqrt{L1+L2}
    difc = 252.816 * 2 * math.sin(angle * 0.5) * (l1 + l2)

    return difc


def copy_bank_wise_offset_values(target_calib_ws, ref_calib_ws, bank_name):
    """Copy over offset values from reference calibration by bank

    Parameters
    ----------
    target_calib_ws: str
        target TableWorkspace
    ref_calib_ws: str
        source TableWorkspace
    bank_name: str
        bank name in (Bank1, Bank2, Bank5)

    Returns
    -------
    None

    """
    start_pid, end_pid = VULCAN_X_PIXEL_RANGE[bank_name]
    row_range = range(start_pid, end_pid)

    # if bank_name == 'Bank1':
    #     row_range = range(0, 3234)
    # elif bank_name == 'east':
    #     row_range = range(3234, 6468)
    # elif bank_name == 'high angle':
    #     row_range = range(6468, 24900)
    # else:
    #     raise RuntimeError('balbal {}'.format(bank_name))

    # Get the workspaces handlers
    if isinstance(target_calib_ws, str):
        target_calib_ws = mantid_helper.retrieve_workspace(target_calib_ws, True)
    if isinstance(ref_calib_ws, str):
        ref_calib_ws = mantid_helper.retrieve_workspace(ref_calib_ws, True)

    # Copy over values
    num_cols = target_calib_ws.columnCount()
    for row_index in row_range:
        for col_index in range(num_cols):
            target_calib_ws.setCell(row_index, col_index, ref_calib_ws.cell(row_index, col_index))


def copy_bank_wise_masks(target_mask_ws, ref_mask_ws: Union[str, Any], bank_name: str):
    """Copy over masks from reference mask workspace to target workspace for a specific bank

    Parameters
    ----------
    target_mask_ws
    ref_mask_ws: str, MaskWorkspace
        reference mask workspace
    bank_name

    Returns
    -------
    None

    """
    # Get pixel ID (detector ID) range
    start_pid, end_pid = VULCAN_X_PIXEL_RANGE[bank_name]
    ws_index_range = range(start_pid, end_pid)

    # apply
    if isinstance(target_mask_ws, str):
        mask_ws = mantid_helper.retrieve_workspace(target_mask_ws, True)
    else:
        mask_ws = target_mask_ws
    if isinstance(ref_mask_ws, str):
        ref_mask_ws = mantid_helper.retrieve_workspace(ref_mask_ws, True)

    # static
    num_masked = 0
    for iws in ws_index_range:
        ref_y_i = ref_mask_ws.dataY(iws)[0]
        mask_ws.dataY(iws)[0] = ref_y_i
        if ref_y_i > 0.5:
            num_masked += 1
    # END-FOR

    print('[REPORT] Apply {} masked detectors from workspace {} range workspace index {}:{}'
          ''.format(num_masked, ref_mask_ws, ws_index_range[0], ws_index_range[-1]))


def calculate_detector_2theta(workspace, ws_index):
    """ Calculate a detector's 2theta angle
    :param workspace:
    :param ws_index: where the detector is
    :return:
    """
    detpos = workspace.getDetector(ws_index).getPos()
    samplepos = workspace.getInstrument().getPos()
    sourcepos = workspace.getInstrument().getSource().getPos()
    q_out = detpos - samplepos
    q_in = samplepos - sourcepos

    twotheta = q_out.angle(q_in) / math.pi * 180

    return twotheta


def cross_correlate_calibrate(ws_name: str,
                              peak_position: float,
                              peak_min: float,
                              peak_max: float,
                              ws_index_range: Tuple[int, int],
                              reference_ws_index: int,
                              cc_number: int,
                              max_offset: float,
                              binning: float,
                              ws_name_posfix='',
                              peak_fit_time=1):
    """Calibrate instrument (with a diamond run) with cross correlation algorithm
    on a specified subset of spectra in a diamond workspace

    This is the CORE workflow algorithm for cross-correlation calibration

    Parameters
    ----------
    ws_name: str
        diamond workspace name
    peak_position: float
        reference peak position
    peak_min: float
        peak range lower boundary
    peak_max: float
        peak range upper boundary
    ws_index_range: ~tuple
        starting workspace index, ending workspace index (excluded)
    reference_ws_index: int
        workspace index of the reference detector
    cc_number: int
        Cross correlation range (for XMin and XMax)
    max_offset: float
        Maximum offset allowed for detector offsets
    binning: float
        binning step
    ws_name_posfix: str
        posfix of the workspace created in the algorithm
    peak_fit_time: int
        number of peak fitting in GetDetectorOffsets

    Returns
    -------
    ~tuple
        OffsetsWorkspace name, MaskWorkspace name

    """
    # Process inputs: reference of input workspace
    diamond_event_ws = mantid_helper.retrieve_workspace(ws_name, True)
    if peak_fit_time == 1:
        fit_twice = False
    else:
        fit_twice = True
    if fit_twice:
        raise RuntimeError('Fit twice is not supported yet')

    # get reference detector position
    det_pos = diamond_event_ws.getDetector(reference_ws_index).getPos()
    twotheta = calculate_detector_2theta(diamond_event_ws, reference_ws_index)
    print('[INFO] Reference spectra = {0}  position @ {1}   2-theta = {2}'
          ''.format(reference_ws_index, det_pos, twotheta))
    print(f'[INFO] Workspace Index range: {ws_index_range[0]}, {ws_index_range[1]}; Binning = {binning}')

    # TODO - NIGHT - shall change from bank to bank
    Rebin(InputWorkspace=ws_name, OutputWorkspace=ws_name, Params='0.5, -{}, 2.0'.format(abs(binning)))

    # Cross correlate spectra using interval around peak at peakpos (d-Spacing)
    cc_ws_name = 'cc_' + ws_name + '_' + ws_name_posfix
    CrossCorrelate(InputWorkspace=ws_name,
                   OutputWorkspace=cc_ws_name,
                   ReferenceSpectra=reference_ws_index,
                   WorkspaceIndexMin=ws_index_range[0], WorkspaceIndexMax=ws_index_range[1],
                   XMin=peak_min, XMax=peak_max)

    # Get offsets for pixels using interval around cross correlations center and peak at peakpos (d-Spacing)
    offset_ws_name = 'offset_' + ws_name + '_' + ws_name_posfix
    mask_ws_name = 'mask_' + ws_name + '_' + ws_name_posfix

    # TODO - THIS IS AN IMPORTANT PARAMETER TO SET THE MASK
    # min_peak_height = 1.0

    print('[DB...BAT] ref peak pos = {}, xrange = {}, {}'.format(peak_position, -cc_number, cc_number))
    try:
        GetDetectorOffsets(InputWorkspace=cc_ws_name,
                           OutputWorkspace=offset_ws_name,
                           MaskWorkspace=mask_ws_name,
                           Step=abs(binning),
                           DReference=peak_position,
                           XMin=-cc_number,
                           XMax=cc_number,
                           MaxOffset=max_offset,
                           PeakFunction='Gaussian',  # 'PseudoVoigt', # Gaussian
                           # FIXME - Following are new features belonged to an in-progress enhancement
                           # MinimumPeakHeight=min_peak_height,  # any peak is lower than 1 shall be masked!
                           # FitEachPeakTwice=fit_twice,
                           # PeakFitResultTableWorkspace=cc_ws_name + '_fit'
                           )
        # TODO FIXME - may remove this save effort later
        # SaveNexusProcessed(InputWorkspace=offset_ws_name, Filename=f'Step2_Offset_{offset_ws_name}.nxs')
    except RuntimeError as run_err:
        # failed to do cross correlation
        print(f'[DEBUG] Step = {abs(binning)}, DReference = {peak_position}, XMin/XMax = +/- {cc_number}, MaxOffset = {max_offset}')
        SaveNexusProcessed(InputWorkspace=cc_ws_name, Filename=f'Step1_CC_{cc_ws_name}.nxs')
        # raise run_err
        return None, run_err

    # Do analysis to the calibration result
    # TODO - NIGHT - Make it better
    # it returns full set of spectra
    # print('[INFO] OffsetsWorkspace {}: spectra number = {}'
    #       ''.format(offset_ws_name, mtd[offset_ws_name].getNumberHistograms()))
    # report_masked_pixels(diamond_event_ws, mtd[mask_ws_name], ws_index_range[0], ws_index_range[1])

    # check result and remove interval result
    # TODO - FUTURE NEXT - consider whether the cross correlate workspace shall be removed or not
    if False and mtd.doesExist(ws_name + "cc" + ws_name_posfix):
        mtd.remove(ws_name + "cc")

    return offset_ws_name, mask_ws_name


# FIXME - this method needs a better name
def correct_difc_to_default(idf_difc_vec, cal_difc_vec, difc_tol,
                            start_row_number: int,
                            mask_ws,
                            mask_erroneous_pixels: bool,
                            cal_table: Union[Any, None]=None,
                            difc_col_index: Union[int, None]=None):
    """Process DIFC if calbirated value is out of tolerance

    Parameters
    ----------
    idf_difc_vec: numpy.array
         DIFC calculated from the instrument geometry (engineered)
    cal_difc_vec: numpy.array
        DIFC calculated from the calibration
    cal_table: ~TableWorkspace, None
        calibration table workspace, calibration value table (TableWorkspace)
    start_row_number: int
        starting row number the first element in DIFC vector
    difc_tol: float
        tolerance on the difference between calculated difc and calibrated difc
    difc_col_index: int, None
        column index of the DIFC in the table workspace
    mask_ws: ~MaskWorkspace
        mask workspace
    mask_erroneous_pixels: bool
        if True, mask the pixels with DIFC out of tolerance

    Returns
    -------

    """
    # difference between IDF and calibrated DIFC
    difc_diff_vec = idf_difc_vec - cal_difc_vec

    print(f'[INFO] DIFC tolerance = {difc_tol}:'
          f'Calibrated DIFC with difference to engineered DIFC with beyond tolerance will be reset.')
    print(f'[INFO] DIFC number = {cal_difc_vec.shape}')

    # go over all the DIFCs
    num_wild_difc = 0
    message = ''
    for index in range(len(difc_diff_vec)):
        if abs(difc_diff_vec[index]) > difc_tol:
            # calibrated DIFC is out of tolerance

            # is it already masked
            if mask_ws.readY(index + start_row_number)[0] < 0.5:
                num_wild_difc += 1
                masked = False
            else:
                masked = True

            # fall back or mask
            if cal_table:
                # fallback DIFC to original
                cal_table.setCell(index + start_row_number, difc_col_index, idf_difc_vec[index])
            elif mask_erroneous_pixels and not masked:
                mask_ws.dataY(start_row_number + index)[0] = 1.

            # error message
            mask_sig = 'Previously masked' if masked else 'Fallback' if cal_table else 'Mask'
            message += '{0}: ws-index = {1}, diff = {2}...  {3}\n' \
                       ''.format(index, index + start_row_number, difc_diff_vec[index], mask_sig)
        # END-IF
    # END-FOR
    print(f'[INFO] Number of DIFC incorrect = {num_wild_difc} out of {cal_difc_vec.shape}\n{message}')

    # Mask detectors
    if mask_erroneous_pixels and num_wild_difc > 0:
        apply_masks(mask_ws)


# TODO - FUTURE - Convert this method to a more general form
def cross_correlate_vulcan_data(diamond_ws_name: str,
                                calib_flag: Dict,
                                cc_fit_time: int = 1,
                                prefix: str = '1fit') -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """Calibrate VULCAN runs with cross correlation algorithm

    Main entrance cross-correlation (for VULCAN Bank1/2/5).

    Vulcan Team (Ke):
    VULCAN-X: when auto reducing, time focus data to pixel (ID) 48840 for bank 1 and 2,
    ...       and 422304 (ID) for bank 5.
    ...       those are centers.
    wsindex = 40704    pixel id = 48840
    wsindex = 182528   pixel id = 422304

    Parameters
    ----------
    diamond_ws_name: str
        input diamond workspace name
    calib_flag: ~dict
        calibration panel flag
    cc_fit_time: int
        number of peak fitting in the GetDetectorOffsets
    prefix: str
        output workspace prefix

    Returns
    -------
    ~tuple
        offset workspace dictionary, mask workspace dictionary

    """
    # Version issue
    if cc_fit_time == 2:
        raise RuntimeError(f'Current GetDetectorOffsets cannot support cc_fit_time = {cc_fit_time}')

    # peak position in d-Spacing
    # FIXME TODO - make this flexible
    peakpos1 = 1.2614
    peakpos2 = 1.2614
    peakpos3 = 1.07577

    offset_ws_dict = dict()
    mask_ws_dict = dict()

    # Bank 1
    if calib_flag['Bank1']:
        bank_name = 'Bank1'
        start_ws_index, end_ws_index = VULCAN_X_PIXEL_RANGE[bank_name]
        ref_ws_index = 40704
        peak_width = 0.04  # modified from 0.005
        cc_number_bank1 = 80
        bank1_offset, bank1_mask = cross_correlate_calibrate(diamond_ws_name, peakpos1,
                                                             peakpos1 - peak_width, peakpos1 + peak_width,
                                                             (start_ws_index, end_ws_index - 1),  # Note: inclusive
                                                             ref_ws_index, cc_number_bank1,
                                                             1, -0.0003, f'{bank_name}_{prefix}',
                                                             peak_fit_time=cc_fit_time)
        if bank1_offset is None:
            err_msg = bank1_mask
            print('[ERROR] Unable to calibrate {} by cross correlation: {}'.format(bank_name, err_msg))
        else:
            offset_ws_dict[bank_name] = bank1_offset
            mask_ws_dict[bank_name] = bank1_mask
    # END-IF

    # East bank
    if calib_flag['Bank2']:
        bank_name = 'Bank2'
        start_ws_index, end_ws_index = VULCAN_X_PIXEL_RANGE[bank_name]
        ref_ws_index = 40704  # 4854 ends with an even right-shift spectrum
        peak_width = 0.04
        cc_number_bank2 = 80
        bank2_offset, bank2_mask = cross_correlate_calibrate(diamond_ws_name, peakpos2,
                                                             peakpos2 - peak_width, peakpos2 + peak_width,
                                                             (start_ws_index, end_ws_index - 1),
                                                             ref_ws_index, cc_number_bank2,
                                                             1, -0.0003, f'{bank_name}_{prefix}',
                                                             peak_fit_time=cc_fit_time)
        if bank2_offset is None:
            err_msg = bank2_mask
            print('[ERROR] Unable to calibrate {} by cross correlation: {}'.format(bank_name, err_msg))
        else:
            offset_ws_dict[bank_name] = bank2_offset
            mask_ws_dict[bank_name] = bank2_mask
    # END-IF

    # High angle
    if calib_flag['Bank5']:
        # High angle bank
        bank_name = 'Bank5'
        start_ws_index, end_ws_index = VULCAN_X_PIXEL_RANGE[bank_name]
        ref_ws_index = 182528
        peak_width = 0.01
        cc_number = 20
        bank5_offset, bank5_mask = cross_correlate_calibrate(diamond_ws_name, peakpos3,
                                                             peakpos3 - peak_width, peakpos3 + peak_width,
                                                             (start_ws_index, end_ws_index - 1),
                                                             ref_ws_index, cc_number=cc_number,
                                                             max_offset=1, binning=-0.0003,
                                                             ws_name_posfix=f'{bank_name}_{prefix}',
                                                             peak_fit_time=cc_fit_time)
        if bank5_offset is None:
            err_msg = bank5_mask
            print('[ERROR] Unable to calibrate {} by cross correlation: {}'.format(bank_name, err_msg))
        else:
            offset_ws_dict[bank_name] = bank5_offset
            mask_ws_dict[bank_name] = bank5_mask
    # END-IF

    if len(offset_ws_dict) == 0:
        raise RuntimeError('No bank is calibrated.  Either none of them is flagged Or all of them failed')

    return offset_ws_dict, mask_ws_dict


def merge_detector_calibration(offset_ws_dict: Dict,
                               mask_ws_dict: Dict,
                               num_banks: int,
                               output_ws_name: str,
                               ref_calib_ws: Union[None, str],
                               ref_mask_ws: Union[None, str]) -> Tuple[str, Any, Any]:
    """Merge calibrated (per bank) detector offsets and masks

    Parameters
    ----------
    offset_ws_dict
    mask_ws_dict
    num_banks
    output_ws_name
    ref_calib_ws: str, None
        reference calibration workspace.
    ref_mask_ws: str, None
        reference mask workspace

    Returns
    -------
    ~tuple
        calibration workspace, offset workspace, mask workspace

    """
    # Get banks recorded in the calibrated offsets and masks
    bank_names = list(offset_ws_dict.keys())
    bank_names.sort()
    print(f'[DEBUG] Bank names: {bank_names}')

    # Merge offsets and masks
    out_offset_ws_name = f'{output_ws_name}_offset'
    out_mask_ws_name = f'{output_ws_name}_mask'

    for ib, bank_name in enumerate(bank_names):
        if ib == 0:
            # Clone first bank's mask and offsets for output
            CloneWorkspace(InputWorkspace=offset_ws_dict[bank_name], OutputWorkspace=out_offset_ws_name)
            CloneWorkspace(InputWorkspace=mask_ws_dict[bank_name], OutputWorkspace=out_mask_ws_name)
            print('Offsets and Mask of {} is cloned for output'.format(bank_name))
        else:
            # merge
            # print(f'Target output offset: {out_offset_ws}, output mask: {out_mask_ws}')
            # print(f'Merging bank {bank_name}: {out_mask_ws} <--  {mask_ws_dict[bank_name]}')
            _merge_partial_offset_mask_workspaces(out_offset_ws_name, offset_ws_dict[bank_name],
                                                  out_mask_ws_name, mask_ws_dict[bank_name])
    # END-FOR

    # Convert to diff calibratin table:  convert the offsets workspace to difc calibration workspace
    calib_ws_name = f'{output_ws_name}_cal'
    ConvertDiffCal(OffsetsWorkspace=out_offset_ws_name,
                   OutputWorkspace=calib_ws_name)

    # Copy value over reference mask and DIFC calibration workspace if
    # 1. some bank is not calibrated
    # 2. reference mask and offset workspaces are provided
    # TODO FIXME - VULCAN's banks name shall be defined as enumerate constants
    vulcan_bank_list = ['Bank1', 'Bank2', 'Bank2']
    for bank_name in vulcan_bank_list:
        # skip calibrated banks
        if bank_name in offset_ws_dict.keys():
            continue

        print('[INFO] Applying {}:{} to {}'.format(ref_calib_ws, bank_name, calib_ws_name))
        if ref_calib_ws:
            copy_bank_wise_offset_values(calib_ws_name, ref_calib_ws, bank_name)
        if ref_mask_ws:
            copy_bank_wise_masks(out_mask_ws_name, ref_mask_ws, bank_name)
            # Apply masks from mask bit to instrument (this is a pure Mantid issue)
            apply_masks(out_mask_ws_name)
    # END-FOR

    return calib_ws_name, out_offset_ws_name, out_mask_ws_name


# TODO FIXME - this method is still in progress of prototyping
#            - COMPARE TO REGULAR pixel-wise setup
# TODO - XMin and XMax are empirical values
def instrument_wide_cross_correlation_prototye(focused_ws_name, reference_ws_index: int = 1,
                                               min_d: float = 1.0649999999999999,
                                               max_d: float = 1.083):
    """Main algorithm to do cross-correlation among different banks of VULCAN.

    This is the second round calibration using the data file
    1. calibrated by previous calibration file based on inner bank cross correlation
    2. diffraction focused
    For the instrument with bank 1/2/5, the input file shall be a 3 bank

    Parameters
    ----------
    focused_ws_name
    reference_ws_index: int
        Reference spectrum for algorithm CrossCorrelate
    min_d
    max_d

    Returns
    -------

    """
    # Example: focused_ws_name = 'vulcan_diamond_3bank'
    num_spec = mtd[focused_ws_name].getNumbetHistograms()

    # Cross correlate an already-focused workspace, for example, with 3 spectra for 3 focused banks
    focused_ws_name_cced = f'{focused_ws_name}_cced'  # example: 'cc_vulcan_diamond_3bank'
    CrossCorrelate(InputWorkspace=focused_ws_name, OutputWorkspace=focused_ws_name_cced,
                   ReferenceSpectra=reference_ws_index,
                   WorkspaceIndexMax=num_spec - 1,
                   XMin=min_d,
                   XMax=max_d)

    # Get detector offsets
    offset_ws_name = f'{focused_ws_name}_offset'
    GetDetectorOffsets(InputWorkspace=focused_ws_name_cced,
                       Step=0.00029999999999999997,
                       DReference=1.0757699999999999,  # FIXME - is this peak position?
                       XMin=-20,
                       XMax=20,
                       OutputWorkspace=offset_ws_name,
                       # FIXME - the followings are new features of an in-progress enhancing version
                       # FitEachPeakTwice=True, PeakFitResultTableWorkspace='ddd', OutputFitResult=True,
                       # MinimumPeakHeight=1
                       )

    return offset_ws_name


def apply_masks(mask_ws):
    """
    apply masked Y to detector
    :param mask_ws:
    :return:
    """
    # collect the masked spectra
    mask_wsindex_list = list()
    for iws in range(mask_ws.getNumberHistograms()):
        if mask_ws.readY(iws)[0] > 0.5:
            mask_wsindex_list.append(iws)

    mask_bits = mask_ws.extractY().flatten()
    print(f'[DEBUG MASK] {mask_ws}: {len(np.where(mask_bits > 0.1)[0])}')

    # mask all detectors explicitly
    mask_ws_name = mask_ws.name()
    mask_ws.maskDetectors(WorkspaceIndexList=mask_wsindex_list)
    mask_ws = mtd[mask_ws_name]

    print('[INFO] {}: # Masked Detectors = {}'.format(mask_ws.name(), len(mask_wsindex_list)))

    return mask_ws


def _merge_mask_workspaces(target_mask_ws_name: str,
                           source_mask_ws_name: str):
    """Merge (add) 2 MaskWorkspaces from source mask workspace to target mask workspace.
    The original masked pixels in the target workspace will be reserved

    Parameters
    ----------
    target_mask_ws_name: str
        name of the target mask workspace to have mask to merge into
    source_mask_ws_name: str
        name of the source mask workspace to have mask to merge from

    Returns
    -------
    None

    """
    print('[MERGING MASK] {} + {} = {}'.format(target_mask_ws_name, source_mask_ws_name, target_mask_ws_name))

    # Get the non-zero spectra from LHS and RHS (as masks)
    # lhs_index_list = list(np.where(mtd[target_mask_ws_name].extractY().flatten() > 0.1)[0])
    # rhs_index_list = list(np.where(mtd[source_mask_ws_name].extractY().flatten() > 0.1)[0])
    lhs_index_list = np.where(mtd[target_mask_ws_name].extractY().flatten() > 0.1)[0].tolist()
    rhs_index_list = np.where(mtd[source_mask_ws_name].extractY().flatten() > 0.1)[0].tolist()
    print(f'[MERGING MASK] LHS: masked spectrum = {len(lhs_index_list)}, '
          f'RHS: masked spectrum = {len(rhs_index_list)}')

    # Set the masked spectra from source MaskWorkspace to target MaskWorkspace
    target_mask_ws = mtd[target_mask_ws_name]

    for masked_index in rhs_index_list:
        masked_index = int(masked_index)
        target_mask_ws.dataY(masked_index)[0] = 1.

    # Mask pixels
    # merge 2 lists
    lhs_index_list.extend(rhs_index_list)
    lhs_index_list.extend(rhs_index_list)
    # remove redundant pixels
    merged_masked_pixels = list(set(lhs_index_list))
    # mask all detectors explicitly
    print(f'{type(merged_masked_pixels)}')
    target_mask_ws.maskDetectors(WorkspaceIndexList=merged_masked_pixels)

    # verify output
    print(f'[MERGING MASK] {target_mask_ws_name}: sum(Y) = {np.sum(mtd[target_mask_ws_name].extractY().flatten())}')


def _merge_partial_offset_mask_workspaces(offset_ws_name: str,
                                          partial_offset_ws_name: str,
                                          mask_ws_name: str,
                                          partial_mask_ws_name: str) -> Tuple[str, str]:
    """Merge partially calibrated offsets and masks to the final offsets and masks workspace

    Parameters
    ----------
    offset_ws_name: str
        target final offset workspace name
    partial_offset_ws_name: str
        target
    mask_ws_name
    partial_mask_ws_name

    Returns
    -------
    ~tuple
        final offset workspace name, final mask workspace name

    """
    # Merge offsets
    # use Plus to combine 2 offsets workspace
    Plus(LHSWorkspace=offset_ws_name, RHSWorkspace=partial_offset_ws_name,
         OutputWorkspace=offset_ws_name)

    # Merge masks
    print('[MERGING MASK] Partial: Number of masked spectra = {0} in {1} ... Sum(Y) = {2}'
          ''.format(mtd[partial_mask_ws_name].getNumberMasked(), partial_mask_ws_name,
                    np.sum(mtd[partial_mask_ws_name].extractY().flatten())))
    # Mask:
    # Make sure mask_ws_name is a string for workspace name
    mask_ws_name = str(mask_ws_name)

    # merge masks workspace
    _merge_mask_workspaces(mask_ws_name, partial_mask_ws_name)

    print('[MERGING MASK] Summed:  Number of masked spectra = {0} in {1} ... sum[Y] = {2}'
          ''.format(mtd[mask_ws_name].getNumberMasked(), mask_ws_name,
                    np.sum(mtd[mask_ws_name].extractY().flatten())))

    return offset_ws_name, mask_ws_name


def save_calibration(calib_ws_name: str,
                     mask_ws_name: str,
                     group_ws_name: str,
                     calib_file_prefix: str) -> str:
    """Export calibrated result to calibration file

    Save calibration (calibration table, mask and grouping) to legacy .cal and current .h5 file

    Parameters
    ----------
    calib_ws_name: str
        (DIFC) calibration workspace name
    mask_ws_name
    group_ws_name
    calib_file_prefix

    Returns
    -------
    str
        calibration file name

    """
    # save
    #  get file name and unlink existing one
    # TODO/FIXME - output directory shall be specified by users
    out_file_name = os.path.join(os.getcwd(), calib_file_prefix + '.h5')
    if os.path.exists(out_file_name):
        os.unlink(out_file_name)

    print(f'[SAVING CAL] Mask {mask_ws_name} Y = {np.sum(mtd[mask_ws_name].extractY().flatten())} '
          f'Masked = {mtd[mask_ws_name].getNumberMasked()}')

    # Save for Mantid diffraction calibration file
    SaveNexusProcessed(InputWorkspace=calib_ws_name, Filename=f'{calib_ws_name}.nxs')
    SaveDiffCal(CalibrationWorkspace=calib_ws_name,
                GroupingWorkspace=group_ws_name,
                MaskWorkspace=mask_ws_name,
                Filename=out_file_name)

    # Save calibration script to python file
    # FIXME - I doubt how many useful information can be saved
    py_name = os.path.join(os.getcwd(), calib_file_prefix + '.py')
    GeneratePythonScript(InputWorkspace=calib_ws_name, Filename=py_name)

    # Save DIFC
    difc_file_name = os.path.join(os.getcwd(), calib_file_prefix + '_difc.dat')
    export_difc(calib_ws_name, difc_file_name)

    return out_file_name


def export_difc(calib_ws_name, out_file_name):
    """
    Export DIFC file
    :param calib_ws_name:
    :param out_file_name:
    :return:
    """
    calib_ws = mantid_helper.retrieve_workspace(calib_ws_name)

    wbuf = '{}\n'.format(calib_ws.getColumnNames())
    for ir in range(calib_ws.rowCount()):
        wbuf += '{}   {}   {}\n'.format(calib_ws.cell(ir, 0), calib_ws.cell(ir, 1), calib_ws.cell(ir, 2))

    out_file = open(out_file_name, 'w')
    out_file.write(wbuf)
    out_file.close()


# --------------  Methods for Cross Correlation Algorithm Analysis -------------------------
def analyze_instrument_cross_correlation_quality(cross_correlation_ws_dict, getdetoffset_result_ws_dict):
    """
    evaluate (by matching the fitted cross-correlation peaks to those calculated from
    CrossCorrelation) the peak fitting result from GetDetectorOffsets in order to create
    list of spectra to be masked.
    :param cross_correlation_ws_dict:
    :param getdetoffset_result_ws_dict:
    :return:
    """
    cost_ws_dict = dict()
    for bank_name in ['Bank1', 'Bank2', 'Bank5']:
        cc_diamond_ws_name = cross_correlation_ws_dict[bank_name]
        fit_result_table_name = getdetoffset_result_ws_dict[bank_name]

        # create the workspaces
        cost_list = evaluate_bank_cross_correlate_quality(cc_diamond_ws_name, fit_result_table_name)
        cost_array = np.array(cost_list).transpose()
        cost_ws_name_i = '{0}_cost'.format(bank_name)
        CreateWorkspace(DataX=cost_array[0], DataY=cost_array[1], NSpec=1,
                        OutputWorkspace=cost_ws_name_i)

        cost_ws_dict[bank_name] = cost_ws_name_i
    # END-FOR

    return cost_ws_dict


def evaluate_bank_cross_correlate_quality(data_ws_name, fit_param_table_name):
    """ evaluate cross correlation quality by fitting the peaks
    :param data_ws_name:
    :param fit_param_table_name:
    :return:
    """
    # data_ws = AnalysisDataService.retrieve(data_ws_name)
    data_ws = mtd[data_ws_name]
    # param_table_ws = AnalysisDataService.retrieve(fit_param_table_name)
    param_table_ws = mtd[fit_param_table_name]

    cost_list = list()

    bad_ws_index_list = list()
    for row_index in range(param_table_ws.rowCount()):

        ws_index = param_table_ws.cell(row_index, 0)
        peak_pos = param_table_ws.cell(row_index, 1)
        peak_height = param_table_ws.cell(row_index, 3)
        peak_sigma = param_table_ws.cell(row_index, 2)
        bkgd_a0 = param_table_ws.cell(row_index, 4)
        bkgd_a1 = param_table_ws.cell(row_index, 5)

        # avoid bad pixels
        if peak_sigma < 1 or peak_sigma > 15 or peak_height < 1 or peak_height > 5:
            bad_ws_index_list.append(ws_index)
            continue

        peak_fwhm = peak_sigma * 2.355

        x_min = peak_pos - 0.5 * peak_fwhm
        x_max = peak_pos + 0.5 * peak_fwhm

        vec_x = data_ws.readX(ws_index)
        i_min = bisect.bisect(vec_x, x_min)
        i_max = bisect.bisect(vec_x, x_max)

        vec_x = vec_x[i_min:i_max]
        obs_y = data_ws.readY(ws_index)[i_min:i_max]
        model_y = gaussian(vec_x, peak_height, peak_pos, peak_sigma, bkgd_a0, bkgd_a1, 'guassian')
        cost = np.sqrt(np.sum((model_y - obs_y)**2))/len(obs_y)

        cost_list.append([ws_index, cost])
    # END-FOR

    print('Bad pixels number: {0}\n\t... They are {1}'.format(len(bad_ws_index_list), bad_ws_index_list))

    return cost_list, bad_ws_index_list


def calculate_model(data_ws_name, ws_index, fit_param_table_name) -> str:
    """Calculate a Gaussian model for one spectrum to compare with raw data

    Parameters
    ----------
    data_ws_name: str
        reference cross-correlation workspace name
    ws_index: int, None
        specific workspace to calculate
    fit_param_table_name: str
        parameter Table workspace name

    Returns
    -------
    str
        Gaussian model workspace

    """
    # Get parameter name
    data_ws = mtd[data_ws_name]
    param_table_ws = mtd[fit_param_table_name]
    out_ws_name = None

    # Loop over fit result parameter table
    for row_index in range(param_table_ws.rowCount()):

        # Only take the specified workspace index
        ws_index_i = param_table_ws.cell(row_index, 0)
        if ws_index != ws_index_i:
            continue

        peak_pos = param_table_ws.cell(row_index, 1)
        peak_height = param_table_ws.cell(row_index, 3)
        peak_sigma = param_table_ws.cell(row_index, 2)
        bkgd_a0 = param_table_ws.cell(row_index, 4)
        bkgd_a1 = param_table_ws.cell(row_index, 5)

        peak_fwhm = peak_sigma * 2.355

        x_min = peak_pos - 0.5 * peak_fwhm
        x_max = peak_pos + 0.5 * peak_fwhm

        vec_x = data_ws.readX(ws_index)
        i_min = bisect.bisect(vec_x, x_min)
        i_max = bisect.bisect(vec_x, x_max)

        vec_x = vec_x[i_min:i_max]
        obs_y = data_ws.readY(ws_index)[i_min:i_max]
        model_y = gaussian(vec_x, peak_height, peak_pos, peak_sigma, bkgd_a0, bkgd_a1, 'guassian')
        cost = np.sqrt(np.sum((model_y - obs_y)**2))/len(obs_y)

        print('[INFO] Cost x = {0}'.format(cost))

        out_ws_name = 'model_{0}'.format(ws_index)
        CreateWorkspace(vec_x, model_y, NSpec=1, OutputWorkspace=out_ws_name)

        # END-LOOP
        break

    # Sanity check
    if out_ws_name is None:
        raise RuntimeError(f'Workspace index {ws_index} cannot be found in {param_table_ws}')

    return out_ws_name


def gaussian(vec_dspace, peak_intensity, peak_center, sigma, a0, a1):
    """ Calculate peak profile function from a vector.
    Implemented is Gaussian

    :param vec_dspace:
    :param peak_intensity:
    :param peak_center:
    :param sigma:
    :param a0:
    :param a1:
    :return:
    """

    # function_type = gaussian:
    vec_y = peak_intensity * np.exp(
        -0.5 * (vec_dspace - peak_center) ** 2 / sigma ** 2) + a0 + a1 * vec_dspace

    return vec_y
