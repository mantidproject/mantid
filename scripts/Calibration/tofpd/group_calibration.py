import numpy as np
from mantid.simpleapi import (ConvertUnits, ExtractSpectra, CloneWorkspace, Rebin,
                              ExtractUnmaskedSpectra, CrossCorrelate, GetDetectorOffsets,
                              ConvertDiffCal, mtd, ApplyDiffCal, DiffractionFocussing, PDCalibration)


def cc_calibrate_groups(data_ws,
                        group_ws,
                        output_basename,
                        previous_calibration=None,
                        Step = 0.001,
                        DReference=1.2615,
                        Xmin=1.22,
                        Xmax=1.30):
    if previous_calibration:
        ApplyDiffCal(data_ws, CalibrationWorkspace=previous_calibration)

    data_d = ConvertUnits(data_ws, Target='dSpacing')

    # skip group 0 because this is eveything that wasn't included in a group
    group_list = np.unique(group_ws.extractY())

    for n, group in enumerate(group_list):
        indexes = np.where(group_ws.extractY().flatten() == group)[0]
        ExtractSpectra(data_d, WorkspaceIndexList=indexes, OutputWorkspace=f'_tmp_group_{group}')
        ExtractUnmaskedSpectra(f'_tmp_group_{group}', OutputWorkspace=f'_tmp_group_{group}')
        Rebin(f'_tmp_group_{group}', Params=f'{Xmin},{Step},{Xmax}', OutputWorkspace=f'_tmp_group_{group}')
        CrossCorrelate(f'_tmp_group_{group}',
                       Xmin=Xmin, XMax=Xmax,
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
                         output_basename,
                         previous_calibration=None,
                         PeakPositions=(0.3117,0.3257,0.3499,0.4205,0.4645,
                                        0.4768,0.4996,0.5150,0.5441,0.5642,
                                        0.5947,0.6307,0.6866,0.7283,0.8185,
                                        0.8920,1.0758,1.2615,2.0599), # diamond
                         TofBinning=(300,-.001,16666.7),
                         PeakFunction='Gaussian',
                         PeakWindow=0.1,
                         PeakWidthPercent=None):

    ApplyDiffCal(data_ws, CalibrationWorkspace=cc_diffcal)
    ConvertUnits(data_ws, Target='dSpacing', OutputWorkspace='_tmp_data_aligned')
    DiffractionFocussing('_tmp_data_aligned', GroupingWorkspace=group_ws, OutputWorkspace='_tmp_data_aligned_focussed')
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

    pd_diffcal = mtd[f'{output_basename}_pd_diffcal']

    cc_and_pd_diffcal = CloneWorkspace(f'{output_basename}_pd_diffcal', OutputWorkspace=f'{output_basename}_cc_pd_diffcal')

    cc_det_to_difc = dict(zip(cc_diffcal.column('detid'), cc_diffcal.column('difc')))

    grouped = mtd['_tmp_data_aligned_focussed']
    si = grouped.spectrumInfo()
    grouped_det_to_difc = {}

    for detid in cc_and_pd_diffcal.column('detid'):
        ind = list(grouped.getIndicesFromDetectorIDs([1]))
        if ind:
            grouped_det_to_difc[detid] = si.difcUncalibrated(ind[0])

    if previous_calibration:
        previous_calibration_det_to_difc = dict(zip(previous_calibration.column('detid'), previous_calibration.column('difc')))
        eng_det_to_difc = {}

        si = data_ws.spectrumInfo()
        for detid in cc_and_pd_diffcal.column('detid'):
            ind = list(data_ws.getIndicesFromDetectorIDs([1]))
            if ind:
                eng_det_to_difc[detid] = si.difcUncalibrated(ind[0])

        for n, detid in enumerate(cc_and_pd_diffcal.column('detid')):
            if detid in cc_det_to_difc and detid in grouped_det_to_difc:
                cc_and_pd_diffcal.setCell(n, 1,
                                          pd_diffcal.cell(n, 1)
                                          * cc_det_to_difc[detid]
                                          * eng_det_to_difc[detid]
                                          / grouped_det_to_difc[detid]
                                          / previous_calibration_det_to_difc[detid])
    else:
        for n, detid in enumerate(cc_and_pd_diffcal.column('detid')):
            if detid in cc_det_to_difc and detid in grouped_det_to_difc:
                cc_and_pd_diffcal.setCell(n, 1,
                                          pd_diffcal.cell(n, 1)
                                          * cc_det_to_difc[detid]
                                          / grouped_det_to_difc[detid])

    return mtd[f'{output_basename}_cc_pd_diffcal']
