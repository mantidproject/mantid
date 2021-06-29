import numpy as np
from mantid.simpleapi import (ConvertUnits, ExtractSpectra,
                              CloneWorkspace, Rebin,
                              ExtractUnmaskedSpectra, CrossCorrelate,
                              GetDetectorOffsets, ConvertDiffCal, mtd,
                              ApplyDiffCal, DiffractionFocussing,
                              PDCalibration)

# Diamond peak positions in d-space
DIAMOND = (0.3117,0.3257,0.3499,0.4205,0.4645,
           0.4768,0.4996,0.5150,0.5441,0.5642,
           0.5947,0.6307,0.6866,0.7283,0.8185,
           0.8920,1.0758,1.2615,2.0599)


def cc_calibrate_groups(data_ws,
                        group_ws,
                        output_basename="_tmp_group_cc_calibration",
                        previous_calibration=None,
                        Step=0.001,
                        DReference=1.2615,
                        Xmin=1.22,
                        Xmax=1.30,
                        MaxDSpaceShift=None):
    """This will perform the CrossCorrelate/GetDetectorOffsets on a group
    of detector pixel.

    It works by looping over the different groups in the group_ws,
    extracting all unmasked spectra of a group, then running
    CrossCorrelate and GetDetectorOffsets on just that group, and
    combinning the results at the end.

    The first unmasked spectra of the group will be used for the
    ReferenceSpectra in CrossCorrelate.

    :param data_ws: Input calibration raw data (in TOF), assumed to already be correctly masked
    :param group_ws: grouping workspace, e.g. output from LoadDetectorsGroupingFile
    :param output_basename: Optional name to use for temporay and output workspace
    :param previous_calibration: Optional previous diffcal workspace
    :param Step: step size for binning of data and input for GetDetectorOffsets, default 0.001
    :param DReference: Derefernce parameter for GetDetectorOffsets, default 1.2615
    :param Xmin: Xmin parameter for CrossCorrelate, default 1.22
    :param Xmax: Xmax parameter for CrossCorrelate, default 1.30
    :param MaxDSpaceShift: MaxDSpaceShift paramter for CrossCorrelate, default None
    :return: Combinned DiffCal workspace from all the different groups
    """
    if previous_calibration:
        ApplyDiffCal(data_ws, CalibrationWorkspace=previous_calibration)

    data_d = ConvertUnits(data_ws, Target='dSpacing')

    group_list = np.unique(group_ws.extractY())

    for group in group_list:
        indexes = np.where(group_ws.extractY().flatten() == group)[0]
        ExtractSpectra(data_d, WorkspaceIndexList=indexes, OutputWorkspace=f'_tmp_group_{group}')
        ExtractUnmaskedSpectra(f'_tmp_group_{group}', OutputWorkspace=f'_tmp_group_{group}')
        if mtd[f'_tmp_group_{group}'].getNumberHistograms() < 2:
            continue
        Rebin(f'_tmp_group_{group}', Params=f'{Xmin},{Step},{Xmax}', OutputWorkspace=f'_tmp_group_{group}')
        CrossCorrelate(f'_tmp_group_{group}',
                       Xmin=Xmin, XMax=Xmax,
                       MaxDSpaceShift=MaxDSpaceShift,
                       WorkspaceIndexMin=0,
                       WorkspaceIndexMax=mtd[f'_tmp_group_{group}'].getNumberHistograms()-1,
                       OutputWorkspace=f'_tmp_group_{group}_cc')

        bin_range = (Xmax-Xmin)/Step
        GetDetectorOffsets(InputWorkspace=f'_tmp_group_{group}_cc',
                           Step=Step,
                           Xmin=-bin_range, XMax=bin_range,
                           DReference=DReference,
                           MaxOffset=1,
                           OutputWorkspace=f'{output_basename}_offsets_{group}')
        previous_calibration = ConvertDiffCal(f'{output_basename}_offsets_{group}',
                                              PreviousCalibration=previous_calibration,
                                              OutputWorkspace=f'{output_basename}_cc_diffcal')

    return mtd[f'{output_basename}_cc_diffcal']


def pdcalibration_groups(data_ws,
                         group_ws,
                         cc_diffcal,
                         output_basename="_tmp_group_pd_calibration",
                         previous_calibration=None,
                         PeakPositions=DIAMOND,
                         TofBinning=(300,-.001,16666.7),
                         PeakFunction='IkedaCarpenterPV',
                         PeakWindow=0.1,
                         PeakWidthPercent=None):
    """This will perform PDCalibration of the group data and combine the
    results with the results of `cc_calibrate_groups`.

    This works by converting the data into d-spacing using the diffcal
    from the cross-correlation, then grouping the data using
    DiffractionFocussing after which it's converted back into TOF
    using an arbitarty diffcal (the combined of all detectors in the
    group). PDCalibration is performed on this grouped workspace after
    which the diffcal's are all combined according to

    .. math::

        DIFC_{effective} = DIFC_{PD} * DIFC_{CC} / DIFC_{arbitarty}

    :param data_ws: Input calibration raw data (in TOF), assumed to already be correctly masked
    :param group_ws: grouping workspace, e.g. output from LoadDetectorsGroupingFile
    :param cc_diffcal: DiffCal workspace which is the output from cc_calibrate_groups
    :param output_basename: Optional name to use for temporay and output workspace
    :param previous_calibration: Optional previous diffcal workspace
    :param PeakPositions: PeakPositions parameter of PDCalibration, default Diamond peaks
    :param TofBinning: TofBinning parameter of PDCalibration, default (300,-.001,16666.7)
    :param PeakFunction: PeakFunction parameter of PDCalibration, default 'IkedaCarpenterPV'
    :param PeakWindow: PeakWindow parameter of PDCalibration, default 0.1
    :param PeakWidthPercent: PeakWidthPercent parameter of PDCalibration, default None
    :return: DiffCal from CrossCorrelate combined with DiffCal from PDCalibration of grouped workspace
    """

    ApplyDiffCal(data_ws, CalibrationWorkspace=cc_diffcal)
    ConvertUnits(data_ws, Target='dSpacing', OutputWorkspace='_tmp_data_aligned')
    DiffractionFocussing('_tmp_data_aligned', GroupingWorkspace=group_ws, OutputWorkspace='_tmp_data_aligned_focussed')
    ApplyDiffCal('_tmp_data_aligned_focussed', ClearCalibration=True)
    ConvertUnits('_tmp_data_aligned_focussed', Target='TOF', OutputWorkspace='_tmp_data_aligned_focussed')

    PDCalibration(InputWorkspace='_tmp_data_aligned_focussed',
                  TofBinning=TofBinning,
                  PreviousCalibrationTable=previous_calibration,
                  PeakFunction=PeakFunction,
                  PeakPositions=PeakPositions,
                  PeakWindow=PeakWindow,
                  PeakWidthPercent=PeakWidthPercent,
                  OutputCalibrationTable=f'{output_basename}_pd_diffcal',
                  DiagnosticWorkspaces=f'{output_basename}_pd_diag')

    # Everything below will all be replaced by be the new CombineDiffCal algorithm
    pd_diffcal = mtd[f'{output_basename}_pd_diffcal']

    cc_and_pd_diffcal = CloneWorkspace(f'{output_basename}_pd_diffcal', OutputWorkspace=f'{output_basename}_cc_pd_diffcal')

    cc_det_to_difc = dict(zip(cc_diffcal.column('detid'), cc_diffcal.column('difc')))

    grouped = mtd['_tmp_data_aligned_focussed']
    specInfo = grouped.spectrumInfo()
    grouped_det_to_difc = {}

    for detid in cc_and_pd_diffcal.column('detid'):
        ind = list(grouped.getIndicesFromDetectorIDs([detid]))
        if ind:
            grouped_det_to_difc[detid] = specInfo.difcUncalibrated(ind[0])

    for n, detid in enumerate(cc_and_pd_diffcal.column('detid')):
        if detid in cc_det_to_difc and detid in grouped_det_to_difc:
            cc_and_pd_diffcal.setCell(n, 1,
                                      pd_diffcal.cell(n, 1)
                                      * cc_det_to_difc[detid]
                                      / grouped_det_to_difc[detid])

    return mtd[f'{output_basename}_cc_pd_diffcal']


def do_group_calibration(data_ws,
                         group_ws,
                         previous_calibration=None,
                         output_basename = "group_calibration",
                         cc_kwargs={},
                         pdcal_kwargs={}):
    """This just calls cc_calibrate_group then feed that results into
    pdcalibration_groups, returning the results.

    :param data_ws: Input calibration raw data (in TOF), assumed to already be correctly masked
    :param group_ws: grouping workspace, e.g. output from LoadDetectorsGroupingFile
    :param previous_calibration: Optional previous diffcal workspace
    :param output_basename: name to use for temporay and output workspace, default group_calibration
    :param cc_kwargs: dict of parameters to pass to cc_calibrate_groups
    :param pdcal_kwargs: dict of parameters to pass to pdcalibration_groups
    :return: The final diffcal after running cc_calibrate_groups and  pdcalibration_groups
    """

    cc_diffcal = cc_calibrate_groups(data_ws,
                                     group_ws,
                                     output_basename,
                                     previous_calibration,
                                     **cc_kwargs)

    diffcal = pdcalibration_groups(data_ws,
                                   group_ws,
                                   cc_diffcal,
                                   output_basename,
                                   previous_calibration,
                                   **pdcal_kwargs)

    return diffcal
