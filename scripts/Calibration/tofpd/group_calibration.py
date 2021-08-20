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
                              SaveDiffCal, DeleteWorkspace, logger,
                              RenameWorkspace, Integration, CloneWorkspace,
                              CreateGroupingWorkspace, CreateDetectorTable,
                              CreateEmptyTableWorkspace)

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
                        MaxDSpaceShift=None,
                        OffsetThreshold=1E-4,
                        SkipCrossCorrelation=[]):
    """This will perform the CrossCorrelate/GetDetectorOffsets on a group
    of detector pixel.

    It works by looping over the different groups in the group_ws,
    extracting all unmasked spectra of a group, then running
    CrossCorrelate and GetDetectorOffsets on just that group, and
    combinning the results at the end. When running a group,
    CrossCorrelate and GetDetectorOffsets could be cycled until
    converging of offsets is reached, given the user input offset
    threshold. If offset threshold is specified to be equal to or
    larger than 1.0, no cycling will be carried out.

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
    :param OffsetThreshold: Convergence threshold for cycling cross correlation, default 1E-4
    :param SkipCrossCorrelation: Skip cross correlation for specified groups.
    :return: Combined DiffCal workspace from all the different groups
    """
    if previous_calibration:
        ApplyDiffCal(data_ws, CalibrationWorkspace=previous_calibration)

    data_d = ConvertUnits(data_ws, Target='dSpacing', OutputWorkspace='data_d')

    group_list = np.unique(group_ws.extractY())

    _accum_cc = None
    to_skip = []
    for group in group_list:
        # Figure out input parameters for CrossCorrelate and GetDetectorOffset, specifically
        # for those parameters for which both a single value and a list is accepted. If a
        # list is given, that means different parameter setup will be used for different groups.
        Xmin_group = Xmin[int(group) - 1] if type(Xmin) == list else Xmin
        Xmax_group = Xmax[int(group) - 1] if type(Xmax) == list else Xmax
        MDS_group = MaxDSpaceShift[int(group) - 1] if type(MaxDSpaceShift) == list else MaxDSpaceShift
        DRef_group = DReference[int(group) - 1] if type(DReference) == list else DReference
        OT_group = OffsetThreshold[int(group) - 1] if type(OffsetThreshold) == list else OffsetThreshold
        cycling = OT_group < 1.0

        indexes = np.where(group_ws.extractY().flatten() == group)[0]
        sn = np.array(group_ws.getSpectrumNumbers())[indexes]
        try:
            ws_indexes = [data_d.getIndexFromSpectrumNumber(int(i)) for i in sn]
        except RuntimeError:
            # data does not contain spectrum in group
            continue

        if group in SkipCrossCorrelation:
            to_skip.extend(ws_indexes)

        ExtractSpectra(data_d, WorkspaceIndexList=ws_indexes, OutputWorkspace='_tmp_group_cc')
        ExtractUnmaskedSpectra('_tmp_group_cc', OutputWorkspace='_tmp_group_cc')
        ExtractSpectra(data_ws, WorkspaceIndexList=ws_indexes, OutputWorkspace='_tmp_group_cc_raw')
        ExtractUnmaskedSpectra('_tmp_group_cc_raw', OutputWorkspace='_tmp_group_cc_raw')
        num_spectra = mtd['_tmp_group_cc'].getNumberHistograms()
        if num_spectra < 2:
            continue
        Rebin('_tmp_group_cc', Params=f'{Xmin_group},{Step},{Xmax_group}', OutputWorkspace='_tmp_group_cc')

        # Figure out brightest spectra to be used as the reference for cross correlation.
        CloneWorkspace('_tmp_group_cc_raw', OutputWorkspace='_tmp_group_cc_raw_tmp')
        intg = Integration('_tmp_group_cc_raw_tmp',
                           StartWorkspaceIndex=0,
                           EndWorkspaceIndex=num_spectra-1,
                           OutputWorkspace='_tmp_group_intg')
        brightest_spec_index = int(np.argmax(np.array([intg.readY(i)[0] for i in range(num_spectra)])))

        # Cycling cross correlation. At each step, we will use the obtained offsets and DIFC's from
        # previous step to obtain new DIFC's. In this way, spectra in group will come closer and closer
        # to each other as the cycle goes. This will continue until converging criterion is reached. The
        # converging criterion is set in such a way that the median value of all the non-zero offsets
        # should be smaller than the threshold (user tuned parameter, default to 1E-4, meaning 0.04%
        # relative offset).
        num_cycle = 1
        while True:
            CrossCorrelate('_tmp_group_cc',
                           Xmin=Xmin_group, XMax=Xmax_group,
                           MaxDSpaceShift=MDS_group,
                           ReferenceSpectra=brightest_spec_index,
                           WorkspaceIndexMin=0,
                           WorkspaceIndexMax=num_spectra-1,
                           OutputWorkspace='_tmp_group_cc')

            bin_range = (Xmax_group-Xmin_group)/Step
            GetDetectorOffsets(InputWorkspace='_tmp_group_cc',
                               Step=Step,
                               Xmin=-bin_range, XMax=bin_range,
                               DReference=DRef_group,
                               MaxOffset=1,
                               OutputWorkspace='_tmp_group_cc')

            if group not in SkipCrossCorrelation:
                offsets_tmp = []
                for item in ws_indexes:
                    if abs(mtd['_tmp_group_cc'].readY(item)) != 0:
                        offsets_tmp.append(abs(mtd['_tmp_group_cc'].readY(item)))
                offsets_tmp = np.array(offsets_tmp)
                logger.information(f'Running group-{group}, cycle-{num_cycle}.')
                logger.information(f'Median offset (no sign) = {np.median(offsets_tmp)}')
                converged = np.median(offsets_tmp) < OT_group
            else:
                for item in ws_indexes:
                    mtd['_tmp_group_cc'].dataY(item)[0] = 0.0
                logger.information(f'Cross correlation skipped for group-{group}.')
                converged = True

            if not cycling or converged:
                if cycling and converged:
                    if group not in SkipCrossCorrelation:
                        logger.information(f'Cross correlation for group-{group} converged, ')
                        logger.information(f'with offset threshold {OT_group}.')
                break
            else:
                previous_calibration = ConvertDiffCal('_tmp_group_cc',
                                                      PreviousCalibration=previous_calibration,
                                                      OutputWorkspace='_tmp_group_cc_diffcal')
                ApplyDiffCal('_tmp_group_cc_raw', CalibrationWorkspace='_tmp_group_cc_diffcal')
                ConvertUnits('_tmp_group_cc_raw', Target='dSpacing', OutputWorkspace='_tmp_group_cc')
                Rebin('_tmp_group_cc', Params=f'{Xmin_group},{Step},{Xmax_group}', OutputWorkspace='_tmp_group_cc')

            num_cycle += 1

        if not _accum_cc:
            _accum_cc = RenameWorkspace('_tmp_group_cc')
        else:
            _accum_cc += mtd['_tmp_group_cc']
            # DeleteWorkspace('_tmp_group_cc')

    previous_calibration = ConvertDiffCal('_accum_cc',
                                          PreviousCalibration=previous_calibration,
                                          OutputWorkspace=f'{output_basename}_cc_diffcal')

    DeleteWorkspace('_accum_cc')
    DeleteWorkspace('_tmp_group_cc')
    DeleteWorkspace('_tmp_group_cc_raw')
    if cycling:
        DeleteWorkspace('_tmp_group_cc_diffcal')

    return mtd[f'{output_basename}_cc_diffcal']


def pdcalibration_groups(data_ws,
                         group_ws,
                         cc_diffcal,
                         mask=None,
                         output_basename="_tmp_group_pd_calibration",
                         previous_calibration=None,
                         PeakPositions=DIAMOND,
                         TofBinning=(300,-.001,16666.7),
                         PeakFunction='IkedaCarpenterPV',
                         PeakWindow=0.1,
                         PeakWidthPercent=None,
                         BadCalibThreshold=100):
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
    :param BadCalibThreshold: Threshold for relative difference between calibrated DIFC and engineering value.
    :return: tuple of DiffCal and Mask (both as TableWorkspace objects) holding the combined DiffCal.
    """

    CreateDetectorTable(data_ws, DetectorTableWorkspace="calib_table_bak")

    ApplyDiffCal(data_ws, CalibrationWorkspace=cc_diffcal)
    ConvertUnits(data_ws, Target='dSpacing', OutputWorkspace='_tmp_data_aligned')
    DiffractionFocussing('_tmp_data_aligned', GroupingWorkspace=group_ws, OutputWorkspace='_tmp_data_aligned')

    ConvertUnits('_tmp_data_aligned', Target='TOF', OutputWorkspace='_tmp_data_aligned')

    instrument = data_ws.getInstrument().getName()

    if instrument != 'POWGEN':
        PDCalibration(InputWorkspace='_tmp_data_aligned',
                      TofBinning=TofBinning,
                      PreviousCalibrationTable=previous_calibration,
                      PeakFunction=PeakFunction,
                      PeakPositions=PeakPositions,
                      PeakWindow=PeakWindow,
                      PeakWidthPercent=PeakWidthPercent,
                      OutputCalibrationTable=f'{output_basename}_pd_diffcal',
                      DiagnosticWorkspaces=f'{output_basename}_pd_diag')
    else:
        PDCalibration(InputWorkspace='_tmp_data_aligned',
                      TofBinning=TofBinning,
                      PreviousCalibrationTable=previous_calibration,
                      PeakFunction=PeakFunction,
                      PeakPositions=PeakPositions,
                      PeakWindow=PeakWindow,
                      PeakWidthPercent=PeakWidthPercent,
                      OutputCalibrationTable='PDCalib',
                      DiagnosticWorkspaces='diag')
        PDCalibration(InputWorkspace='_tmp_data_aligned',
                      TofBinning=[TofBinning[0], TofBinning[1]/2, TofBinning[2]],
                      PreviousCalibrationTable='PDCalib',
                      PeakFunction=PeakFunction,
                      PeakPositions=PeakPositions,
                      PeakWindow=PeakWindow,
                      PeakWidthPercent=PeakWidthPercent/2.0,
                      OutputCalibrationTable='PDCalib',
                      DiagnosticWorkspaces='diag')
        PDCalibration(InputWorkspace='_tmp_data_aligned',
                      TofBinning=[TofBinning[0], TofBinning[1]/2, TofBinning[2]],
                      PreviousCalibrationTable='PDCalib',
                      PeakFunction=PeakFunction,
                      PeakPositions=PeakPositions,
                      PeakWindow=PeakWindow,
                      PeakWidthPercent=PeakWidthPercent/2.0,
                      OutputCalibrationTable=f'{output_basename}_pd_diffcal',
                      DiagnosticWorkspaces=f'{output_basename}_pd_diag')
        DeleteWorkspace('PDCalib')
        DeleteWorkspace('PDCalib_mask')
        DeleteWorkspace('diag')

    CombineDiffCal(PixelCalibration=cc_diffcal,
                   GroupedCalibration=f'{output_basename}_pd_diffcal',
                   CalibrationWorkspace='_tmp_data_aligned',
                   MaskWorkspace=f'{output_basename}_pd_diffcal_mask',
                   OutputWorkspace=f'{output_basename}_cc_pd_diffcal_tmp')

    DeleteWorkspace('_tmp_data_aligned')

    out_table = CreateEmptyTableWorkspace(OutputWorkspace=f'{output_basename}_cc_pd_diffcal')
    out_table.addColumn("int", "detid")
    out_table.addColumn("double", "difc")
    out_table.addColumn("double", "difa")
    out_table.addColumn("double", "tzero")
    num_hist = data_ws.getNumberHistograms()
    for i in range(num_hist):
        difc_bak = mtd['calib_table_bak'].row(i)['DIFC']
        difc_calib = mtd[f'{output_basename}_cc_pd_diffcal_tmp'].row(i)['difc']
        if mtd[f'{output_basename}_pd_diffcal_mask'].readY(i)[0] == 0.0:
            diff_difc = abs(difc_bak - difc_calib) / difc_calib * 100.0
        else:
            diff_difc = 0.0
        if diff_difc >= BadCalibThreshold:
            difc_calib = difc_bak
        new_row = { 'detid': mtd[f'{output_basename}_cc_pd_diffcal_tmp'].row(i)['detid'],
                    'difc': difc_calib,
                    'difa': mtd[f'{output_basename}_cc_pd_diffcal_tmp'].row(i)['difa'],
                    'tzero': mtd[f'{output_basename}_cc_pd_diffcal_tmp'].row(i)['tzero'] }
        out_table.addRow(new_row)

    DeleteWorkspace(f'{output_basename}_cc_pd_diffcal_tmp')

    return mtd[f'{output_basename}_cc_pd_diffcal'], mtd[f'{output_basename}_pd_diffcal_mask']


def do_group_calibration(data_ws,
                         group_ws,
                         previous_calibration=None,
                         mask=None,
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
                                         mask,
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

    calibrant_file = args.get('CalibrantFile', None)
    if calibrant_file is None:
        calibrant = args['Calibrant']
    groups = args['Groups']
    out_groups_by = args.get('OutputGroupsBy', 'Group')
    sample_env = args.get('SampleEnvironment', 'UnknownSampleEnvironment')
    mask = args.get('Mask')
    instrument = args.get('Instrument', 'NOM')
    cc_kwargs = args.get('CrossCorrelate', {})
    pdcal_kwargs = args.get('PDCalibration', {})
    previous_calibration = args.get('PreviousCalibration')

    date = str(args.get('Date', datetime.datetime.now().strftime('%Y_%m_%d')))
    caldirectory = str(args.get('CalDirectory', os.path.abspath('.')))

    if calibrant_file is not None:
        ws = Load(calibrant_file)
        calibrant = ws.getRun().getProperty('run_number').value
    else:
        filename = f'{instrument}_{calibrant}'
        ws = Load(filename)

    calfilename = f'{caldirectory}/{instrument}_{calibrant}_{date}_{sample_env}.h5'
    logger.information(f'going to create calibration file: {calfilename}')

    groups = LoadDetectorsGroupingFile(groups, InputWorkspace=ws)

    if mask:
        mask = LoadMask(instrument, mask)
        MaskDetectors(ws, MaskedWorkspace=mask)

    if previous_calibration:
        previous_calibration = LoadDiffCal(previous_calibration,
                                           MakeGroupingWorkspace=False,
                                           MakeMaskWorkspace=False)

    diffcal, mask = do_group_calibration(ws,
                                         groups,
                                         previous_calibration,
                                         mask,
                                         cc_kwargs=cc_kwargs,
                                         pdcal_kwargs=pdcal_kwargs)

    CreateGroupingWorkspace(InputWorkspace=ws,
                            GroupDetectorsBy=out_groups_by,
                            OutputWorkspace='out_groups')
    SaveDiffCal(CalibrationWorkspace=diffcal,
                MaskWorkspace=mask,
                GroupingWorkspace=mtd['out_groups'],
                Filename=calfilename)


if __name__ == '__main__':
    infile = os.path.abspath(sys.argv[1])
    process_json(infile)
