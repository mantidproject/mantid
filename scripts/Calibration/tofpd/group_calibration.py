import sys
import os
import json
import datetime
import numpy as np
from mantid.simpleapi import (ConvertUnits, ExtractSpectra, Rebin,
                              MaskDetectors, ExtractUnmaskedSpectra,
                              CrossCorrelate, GetDetectorOffsets,
                              ConvertDiffCal, mtd, ApplyDiffCal,
                              DiffractionFocussing, PDCalibration,
                              Load, LoadMask, CombineDiffCal,
                              LoadDiffCal, LoadDetectorsGroupingFile,
                              SaveDiffCal, DeleteWorkspace)

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

    data_d = ConvertUnits(data_ws, Target='dSpacing', OutputWorkspace='data_d')

    group_list = np.unique(group_ws.extractY())

    for group in group_list:
        indexes = np.where(group_ws.extractY().flatten() == group)[0]
        sn = np.array(group_ws.getSpectrumNumbers())[indexes]
        try:
            ws_indexes = [data_d.getIndexFromSpectrumNumber(int(i)) for i in sn]
        except RuntimeError:
            # data does not contain spectrum in group
            continue
        ExtractSpectra(data_d, WorkspaceIndexList=ws_indexes, OutputWorkspace='_tmp_group_cc')
        ExtractUnmaskedSpectra('_tmp_group_cc', OutputWorkspace='_tmp_group_cc')
        if mtd['_tmp_group_cc'].getNumberHistograms() < 2:
            continue
        Rebin('_tmp_group_cc', Params=f'{Xmin},{Step},{Xmax}', OutputWorkspace='_tmp_group_cc')
        CrossCorrelate('_tmp_group_cc',
                       Xmin=Xmin, XMax=Xmax,
                       MaxDSpaceShift=MaxDSpaceShift,
                       WorkspaceIndexMin=0,
                       WorkspaceIndexMax=mtd['_tmp_group_cc'].getNumberHistograms()-1,
                       OutputWorkspace='_tmp_group_cc')

        bin_range = (Xmax-Xmin)/Step
        GetDetectorOffsets(InputWorkspace='_tmp_group_cc',
                           Step=Step,
                           Xmin=-bin_range, XMax=bin_range,
                           DReference=DReference,
                           MaxOffset=1,
                           OutputWorkspace='_tmp_group_cc')
        previous_calibration = ConvertDiffCal('_tmp_group_cc',
                                              PreviousCalibration=previous_calibration,
                                              OutputWorkspace=f'{output_basename}_cc_diffcal')
        DeleteWorkspace('_tmp_group_cc')

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
    :return: tuple of DiffCal and Mask from CrossCorrelate combined with DiffCal from PDCalibration of grouped workspace
    """

    ApplyDiffCal(data_ws, CalibrationWorkspace=cc_diffcal)
    ConvertUnits(data_ws, Target='dSpacing', OutputWorkspace='_tmp_data_aligned')
    DiffractionFocussing('_tmp_data_aligned', GroupingWorkspace=group_ws, OutputWorkspace='_tmp_data_aligned')

    # Remove the following line after new CombineDiffCal algorithm is implemented as that will use the calibrated difc
    ApplyDiffCal('_tmp_data_aligned', ClearCalibration=True)

    ConvertUnits('_tmp_data_aligned', Target='TOF', OutputWorkspace='_tmp_data_aligned')

    PDCalibration(InputWorkspace='_tmp_data_aligned',
                  TofBinning=TofBinning,
                  PreviousCalibrationTable=previous_calibration,
                  PeakFunction=PeakFunction,
                  PeakPositions=PeakPositions,
                  PeakWindow=PeakWindow,
                  PeakWidthPercent=PeakWidthPercent,
                  OutputCalibrationTable=f'{output_basename}_pd_diffcal',
                  DiagnosticWorkspaces=f'{output_basename}_pd_diag')

    CombineDiffCal(PixelCalibration=cc_diffcal,
                   GroupedCalibration=f'{output_basename}_pd_diffcal',
                   CalibrationWorkspace='_tmp_data_aligned',
                   MaskWorkspace=f'{output_basename}_pd_diffcal_mask',
                   OutputWorkspace=f'{output_basename}_cc_pd_diffcal')

    DeleteWorkspace('_tmp_data_aligned')

    return mtd[f'{output_basename}_cc_pd_diffcal'], mtd[f'{output_basename}_pd_diffcal_mask']


def do_group_calibration(data_ws,
                         group_ws,
                         previous_calibration=None,
                         output_basename="group_calibration",
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

    diffcal, mask = pdcalibration_groups(data_ws,
                                         group_ws,
                                         cc_diffcal,
                                         output_basename,
                                         previous_calibration,
                                         **pdcal_kwargs)

    return diffcal, mask


def process_json(json_filename):
    """This will read a json file, process the data and save the calibration.

    Only ``Calibrant`` and ``Groups`` are required.

    An example input showing every possible options is:

    .. code-block:: JSON

      {
        "Calibrant": "12345",
        "Groups": "/path/to/groups.xml",
        "Mask": "/path/to/mask.xml",
        "Instrument": "NOM",
        "Date" : "2019_09_04",
        "SampleEnvironment": "shifter",
        "PreviousCalibration": "/path/to/cal.h5",
        "CalDirectory": "/path/to/output_directory",
        "CrossCorrelate": {"Step": 0.001,
                           "DReference: 1.5,
                           "Xmin": 1.0,
                           "Xmax": 3.0,
                           "MaxDSpaceShift": 0.25},
        "PDCalibration": {"PeakPositions": [1, 2, 3],
                          "TofBinning": (300,0.001,16666),
                          "PeakFunction": 'Gaussian',
                          "PeakWindow": 0.1,
                          "PeakWidthPercent": 0.001}
      }
    """
    with open(json_filename) as json_file:
        args = json.load(json_file)

    calibrant = args['Calibrant']
    groups = args['Groups']
    sample_env = args.get('SampleEnvironment', 'UnknownSampleEnvironment')
    mask = args.get('Mask')
    instrument = args.get('Instrument', 'NOM')
    cc_kwargs = args.get('CrossCorrelate', {})
    pdcal_kwargs = args.get('PDCalibration', {})
    previous_calibration = args.get('PreviousCalibration')

    date = str(args.get('Date', datetime.datetime.now().strftime('%Y_%m_%d')))
    caldirectory = str(args.get('CalDirectory', os.path.abspath('.')))

    calfilename = f'{caldirectory}/{instrument}_{calibrant}_{date}_{sample_env}.h5'
    print('going to create calibration file: %s' % calfilename)

    filename = f'{instrument}_{calibrant}'

    ws = Load(filename)

    groups = LoadDetectorsGroupingFile(groups, InputWorkspace=ws)

    if mask:
        mask = LoadMask(mask)
        MaskDetectors(ws, MaskedWorkspace=mask)

    if previous_calibration:
        previous_calibration = LoadDiffCal(previous_calibration,
                                           MakeGroupingWorkspace=False,
                                           MakeMaskWorkspace=False)

    diffcal, mask = do_group_calibration(ws,
                                         groups,
                                         previous_calibration,
                                         cc_kwargs=cc_kwargs,
                                         pdcal_kwargs=pdcal_kwargs)

    SaveDiffCal(CalibrationWorkspace=diffcal,
                MaskWorkspace=mask,
                Filename=calfilename)


if __name__ == '__main__':
    infile = os.path.abspath(sys.argv[1])
    process_json(infile)
