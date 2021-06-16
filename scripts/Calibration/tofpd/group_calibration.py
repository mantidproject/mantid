import numpy as np
from mantid.simpleapi import (ConvertUnits, ExtractSpectra, CloneWorkspace, Rebin,
                              ExtractUnmaskedSpectra, CrossCorrelate, GetDetectorOffsets,
                              ConvertDiffCal, mtd, ApplyDiffCal, DiffractionFocussing, PDCalibration)


def cc_calibrate_groups(data_ws,
                        group_ws,
                        output_basename,
                        Step = 0.001,
                        DReference=1.2615,
                        Xmin=1.22,
                        Xmax=1.30):
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
        ConvertDiffCal(f'{output_basename}_offsets_{group}',
                       OutputWorkspace=f'{output_basename}_cc_difc_{group}')

    # This part will be replace by changes in ConvertDiffCal
    combined_offset = CloneWorkspace(f'{output_basename}_offsets_{group_list[0]}', OutputWorkspace=f'{output_basename}_cc_offsets')
    combined_offset_si = combined_offset.spectrumInfo()

    for group in group_list[1:]:
        offset_ws = mtd[f'{output_basename}_offsets_{group}']
        si = offset_ws.spectrumInfo()
        for index in range(offset_ws.getNumberHistograms()):
            if not si.isMasked(index) and offset_ws.readY(index)[0] != 0:
                combined_offset_si.setMasked(index, False)
                combined_offset.setY(index, offset_ws.readY(index))

    ConvertDiffCal(combined_offset, OutputWorkspace=f'{output_basename}_cc_diffcal')

    return mtd[f'{output_basename}_cc_diffcal'], mtd[f'{output_basename}_cc_offsets']


def pdcalibration_groups(data_ws,
                         group_ws,
                         cc_diffcal, cc_offsets,
                         output_basename,
                         PeakPositions=(0.3117,0.3257,0.3499,0.4205,0.4645,
                                        0.4768,0.4996,0.5150,0.5441,0.5642,
                                        0.5947,0.6307,0.6866,0.7283,0.8185,
                                        0.8920,1.0758,1.2615,2.0599), # diamond
                         TofBinning=[300,-.001,16666.7],
                         PreviousCalibrationTable=None,
                         PeakFunction='Gaussian',
                         PeakWindow=0.1,
                         PeakWidthPercent=None):

    ApplyDiffCal(data_ws, CalibrationWorkspace=cc_diffcal)
    ConvertUnits(data_ws, Target='dSpacing', OutputWorkspace='_tmp_data_aligned')
    DiffractionFocussing('_tmp_data_aligned', GroupingWorkspace=group_ws, OutputWorkspace='_tmp_data_aligned_focussed')
    ConvertUnits('_tmp_data_aligned_focussed', Target='TOF', OutputWorkspace='_tmp_data_aligned_focussed')

    PDCalibration(InputWorkspace='_tmp_data_aligned_focussed',
                  TofBinning=TofBinning,
                  PreviousCalibrationTable=PreviousCalibrationTable,
                  PeakFunction=PeakFunction,
                  PeakPositions=PeakPositions,
                  PeakWindow=PeakWindow,
                  PeakWidthPercent=PeakWidthPercent,
                  OutputCalibrationTable=f'{output_basename}_pd_diffcal',
                  DiagnosticWorkspaces=f'{output_basename}_pd_diag')

    # This part will be replaced with a new algorithm to do the combining of diffcals
    cc_offset_si = cc_offsets.spectrumInfo()

    cc_and_pd_diffcal = CloneWorkspace(f'{output_basename}_pd_diffcal', OutputWorkspace=f'{output_basename}_cc_pd_diffcal')

    for index in range(cc_offsets.getNumberHistograms()):
        if not cc_offset_si.isMasked(index) and cc_offsets.readY(index)[0] != 0:
            cc_and_pd_diffcal.setCell(index, 1, cc_and_pd_diffcal.cell(index, 1)/(1+cc_offsets.readY(index)[0]))

    return mtd[f'{output_basename}_cc_pd_diffcal']
