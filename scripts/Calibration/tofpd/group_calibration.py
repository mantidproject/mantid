import sys
import os
import json
import datetime
import numpy as np
from mantid.simpleapi import (
    CompressEvents,
    ConvertUnits,
    ExtractSpectra,
    Rebin,
    MaskDetectors,
    ExtractUnmaskedSpectra,
    CrossCorrelate,
    GetDetectorOffsets,
    ConvertDiffCal,
    mtd,
    ApplyDiffCal,
    DiffractionFocussing,
    PDCalibration,
    Load,
    LoadMask,
    CombineDiffCal,
    LoadDiffCal,
    LoadDetectorsGroupingFile,
    SaveDiffCal,
    DeleteWorkspace,
    logger,
    RenameWorkspace,
    Integration,
    CreateGroupingWorkspace,
    CreateDetectorTable,
    CreateEmptyTableWorkspace,
    SmoothData,
)

# Diamond peak positions in d-space
DIAMOND = (
    0.3117,
    0.3257,
    0.3499,
    0.4205,
    0.4645,
    0.4768,
    0.4996,
    0.5150,
    0.5441,
    0.5642,
    0.5947,
    0.6307,
    0.6866,
    0.7283,
    0.8185,
    0.8920,
    1.0758,
    1.2615,
    2.0599,
)


def _getBrightestWorkspaceIndex(wksp):
    """Figure out brightest spectra to be used as the reference for cross correlation.
    The brightest spectrum will be used as the reference
    """
    intg = Integration(InputWorkspace=wksp, OutputWorkspace="_tmp_group_intg")  # RangeLower=1.22, RangeUpper=1.30, )
    brightest_spec_index = int(np.argmax(intg.extractY()))
    DeleteWorkspace("_tmp_group_intg")
    return brightest_spec_index


def _createDetToWkspIndexMap(wksp):
    mapping = {}
    spectrumInfo = wksp.spectrumInfo()
    for wksp_index in range(wksp.getNumberHistograms()):
        detids = wksp.getSpectrum(wksp_index).getDetectorIDs()
        for detid in detids:  # add each detector id in separately
            if spectrumInfo.isMasked(wksp_index):
                mapping[detid] = -1
            else:
                mapping[detid] = wksp_index
    return mapping


def cc_calibrate_groups(
    data_ws,
    group_ws,
    output_basename="_tmp_group_cc_calibration",
    previous_calibration=None,
    Step=0.001,
    DReference=1.2615,
    Xmin=1.22,
    Xmax=1.30,
    MaxDSpaceShift=None,
    OffsetThreshold=1e-4,
    SkipCrossCorrelation=[],
    PeakFunction="Gaussian",
    SmoothNPoints=0,
):
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
    :param output_basename: Optional name to use for temporary and output workspace
    :param previous_calibration: Optional previous diffcal workspace
    :param Step: step size for binning of data and input for GetDetectorOffsets, default 0.001
    :param DReference: Derefernce parameter for GetDetectorOffsets, default 1.2615
    :param Xmin: Minimum d-spacing for CrossCorrelate, default 1.22
    :param Xmax: Maximum d-spacing for CrossCorrelate, default 1.30
    :param MaxDSpaceShift: MaxDSpaceShift paramter for CrossCorrelate, default None
    :param OffsetThreshold: Convergence threshold for cycling cross correlation, default 1E-4
    :param SkipCrossCorrelation: Skip cross correlation for specified groups
    :param PeakFunction: Peak function to use for extracting the offset
    :param SmoothNPoints: Number of points for smoothing spectra, for cross correlation ONLY
    :return: Combined DiffCal workspace from all the different groups
    """
    if previous_calibration:
        ApplyDiffCal(data_ws, CalibrationWorkspace=previous_calibration)

    det2WkspIndex = _createDetToWkspIndexMap(data_ws)

    _accum_cc = None
    to_skip = []
    for group in group_ws.getGroupIDs():
        if group == -1:
            continue  # this is a group of unset pixels
        # Figure out input parameters for CrossCorrelate and GetDetectorOffset, specifically
        # for those parameters for which both a single value and a list is accepted. If a
        # list is given, that means different parameter setup will be used for different groups.
        Xmin_group = Xmin[int(group) - 1] if isinstance(Xmin, list) else Xmin
        Xmax_group = Xmax[int(group) - 1] if isinstance(Xmax, list) else Xmax
        MDS_group = MaxDSpaceShift[int(group) - 1] if isinstance(MaxDSpaceShift, list) else MaxDSpaceShift
        DRef_group = DReference[int(group) - 1] if isinstance(DReference, list) else DReference
        OT_group = OffsetThreshold[int(group) - 1] if isinstance(OffsetThreshold, list) else OffsetThreshold
        pf_group = PeakFunction[int(group) - 1] if isinstance(PeakFunction, list) else PeakFunction
        snpts_group = SmoothNPoints[int(group) - 1] if isinstance(SmoothNPoints, list) else SmoothNPoints
        cycling = OT_group < 1.0

        try:
            # detector ids that get focussed together
            detids = group_ws.getDetectorIDsOfGroup(int(group))
            # convert to workspace indices in the input data
            ws_indices = np.asarray([det2WkspIndex[detid] for detid in detids])
            # remove masked spectra
            ws_indices = ws_indices[ws_indices != -1]
        except RuntimeError:
            # data does not contain spectrum in group
            continue

        if group in SkipCrossCorrelation:
            to_skip.extend(ws_indices)

        num_spectra = len(ws_indices)  # takes masking into account
        if num_spectra < 2:
            to_skip.extend(ws_indices)
            continue  # go to next group

        # grab out the spectra in d-space and time-of-flight
        ExtractSpectra(data_ws, WorkspaceIndexList=ws_indices, OutputWorkspace="_tmp_group_cc_raw")
        ConvertUnits("_tmp_group_cc_raw", Target="dSpacing", OutputWorkspace="_tmp_group_cc_main")
        Rebin("_tmp_group_cc_main", Params=(Xmin_group, Step, Xmax_group), OutputWorkspace="_tmp_group_cc_main")
        if snpts_group >= 3:
            SmoothData("_tmp_group_cc_main", NPoints=snpts_group, OutputWorkspace="_tmp_group_cc_main")

        # Figure out brightest spectra to be used as the reference for cross correlation.
        brightest_spec_index = _getBrightestWorkspaceIndex("_tmp_group_cc_main")

        # Cycling cross correlation. At each step, we will use the obtained offsets and DIFC's from
        # previous step to obtain new DIFC's. In this way, spectra in group will come closer and closer
        # to each other as the cycle goes. This will continue until converging criterion is reached. The
        # converging criterion is set in such a way that the median value of all the non-zero offsets
        # should be smaller than the threshold (user tuned parameter, default to 1E-4, meaning 0.04%
        # relative offset).
        num_cycle = 1
        while True:
            # take the data in d-space and perform cross correlation on a subset of spectra
            # the output workspace is the cross correlation data for only the requested spectra
            CrossCorrelate(
                InputWorkspace="_tmp_group_cc_main",
                Xmin=Xmin_group,
                XMax=Xmax_group,
                MaxDSpaceShift=MDS_group,
                ReferenceSpectra=brightest_spec_index,
                WorkspaceIndexMin=0,
                WorkspaceIndexMax=num_spectra - 1,
                OutputWorkspace="_tmp_group_cc_main",
            )

            bin_range = (Xmax_group - Xmin_group) / Step
            GetDetectorOffsets(
                InputWorkspace="_tmp_group_cc_main",
                Step=Step,
                Xmin=-bin_range,
                XMax=bin_range,
                DReference=DRef_group,
                MaxOffset=1,
                PeakFunction=pf_group,
                OutputWorkspace="_tmp_group_cc_main",
            )

            if group in SkipCrossCorrelation:
                # set the detector offsets to zero
                for item in ws_indices:
                    mtd["_tmp_group_cc_main"].dataY(int(item))[0] = 0.0
                logger.notice(f"Cross correlation skipped for group-{group}.")
                converged = True
            else:
                # collect the non-zero offsets for determining convergence
                offsets_tmp = np.abs(mtd["_tmp_group_cc_main"].extractY())
                offsets_tmp = offsets_tmp[offsets_tmp != 0.0]
                logger.notice(f"Running group-{group}, cycle-{num_cycle}.")
                logger.notice(f"Median offset (no sign) = {np.median(offsets_tmp)}")
                logger.notice(f"Running group-{group}, cycle-{num_cycle}.")
                logger.notice(f"Median offset (no sign) = {np.median(offsets_tmp)}")
                # it has converged if the median is less than offset-threshold for the group
                converged = np.median(offsets_tmp) < OT_group

            if not cycling or converged:
                if cycling and converged:
                    if group not in SkipCrossCorrelation:
                        logger.notice(f"Cross correlation for group-{group} converged, ")
                        logger.notice(f"with offset threshold {OT_group}.")
                break
            else:
                # create the input data for the next loop by applying the calibration
                previous_calibration = ConvertDiffCal(
                    "_tmp_group_cc_main", PreviousCalibration=previous_calibration, OutputWorkspace="_tmp_group_cc_diffcal"
                )
                ApplyDiffCal("_tmp_group_cc_raw", CalibrationWorkspace="_tmp_group_cc_diffcal")
                ConvertUnits("_tmp_group_cc_raw", Target="dSpacing", OutputWorkspace="_tmp_group_cc_main")
                Rebin("_tmp_group_cc_main", Params=f"{Xmin_group},{Step},{Xmax_group}", OutputWorkspace="_tmp_group_cc_main")

            num_cycle += 1

        if not _accum_cc:
            _accum_cc = RenameWorkspace("_tmp_group_cc_main")
        else:
            _accum_cc += mtd["_tmp_group_cc_main"]

    previous_calibration = ConvertDiffCal(
        "_accum_cc", PreviousCalibration=previous_calibration, OutputWorkspace=f"{output_basename}_cc_diffcal"
    )

    DeleteWorkspace("_accum_cc")
    DeleteWorkspace("_tmp_group_cc_main")
    DeleteWorkspace("_tmp_group_cc_raw")
    if cycling and "_tmp_group_cc_diffcal" in mtd:
        DeleteWorkspace("_tmp_group_cc_diffcal")

    return mtd[f"{output_basename}_cc_diffcal"], to_skip


def pdcalibration_groups(
    data_ws,
    group_ws,
    cc_diffcal,
    to_skip,
    output_basename="_tmp_group_pd_calibration",
    previous_calibration=None,
    PeakPositions=DIAMOND,
    TofBinning=(300, -0.001, 16666.7),
    PeakFunction="IkedaCarpenterPV",
    PeakWindow=0.1,
    PeakWidthPercent=None,
    BadCalibThreshold=100,
):
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
    :param to_skip: Groups to skip the cross correlation stage
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

    # time-focus the data into the requested number of groups
    ApplyDiffCal(data_ws, CalibrationWorkspace=cc_diffcal)
    ConvertUnits(data_ws, Target="dSpacing", OutputWorkspace=data_ws)
    DiffractionFocussing(data_ws, GroupingWorkspace=group_ws, OutputWorkspace="_tmp_data_aligned")
    if mtd["_tmp_data_aligned"].id() == "EventWorkspace":
        CompressEvents(InputWorkspace="_tmp_data_aligned", OutputWorkspace="_tmp_data_aligned")
    ConvertUnits("_tmp_data_aligned", Target="TOF", OutputWorkspace="_tmp_data_aligned")
    # rebin the data and drop events even if they were present before
    Rebin(InputWorkspace="_tmp_data_aligned", OutputWorkspace="_tmp_data_aligned", Params=TofBinning, PreserveEvents=False)

    # put the input data back into time-of-flight
    ConvertUnits(data_ws, Target="dSpacing", OutputWorkspace=data_ws)

    instrument = data_ws.getInstrument().getName()

    if instrument == "POWGEN":
        pdcalib_for_powgen(
            mtd["_tmp_data_aligned"],
            TofBinning,
            previous_calibration,
            PeakFunction,
            PeakPositions,
            PeakWindow,
            PeakWidthPercent,
            f"{output_basename}_pd_diffcal",
            f"{output_basename}_pd_diag",
        )
        if to_skip:
            ExtractSpectra(data_ws, WorkspaceIndexList=to_skip, OutputWorkspace="_tmp_group_to_skip")
            ExtractUnmaskedSpectra("_tmp_group_to_skip", OutputWorkspace="_tmp_group_to_skip")
            pdcalib_for_powgen(
                mtd["_tmp_group_to_skip"],
                [300, -0.0008, 16667],
                previous_calibration,
                PeakFunction,
                PeakPositions,
                PeakWindow,
                PeakWidthPercent,
                f"{output_basename}_pd_diffcal_skip",
                f"{output_basename}_pd_diag_skip",
            )
    else:
        PDCalibration(
            InputWorkspace="_tmp_data_aligned",
            TofBinning=TofBinning,
            PreviousCalibrationTable=previous_calibration,
            PeakFunction=PeakFunction,
            PeakPositions=PeakPositions,
            PeakWindow=PeakWindow,
            PeakWidthPercent=PeakWidthPercent,
            OutputCalibrationTable=f"{output_basename}_pd_diffcal",
            MaskWorkspace=f"{output_basename}_pd_diffcal_mask",
            DiagnosticWorkspaces=f"{output_basename}_pd_diag",
        )
        if to_skip:
            if isinstance(to_skip, list):
                to_skip = [int(element) for element in to_skip]  # issue with numpy.int64
            ExtractSpectra(data_ws, WorkspaceIndexList=to_skip, OutputWorkspace="_tmp_group_to_skip")
            ExtractUnmaskedSpectra("_tmp_group_to_skip", OutputWorkspace="_tmp_group_to_skip")
            PDCalibration(
                InputWorkspace="_tmp_group_to_skip",
                TofBinning=TofBinning,
                PreviousCalibrationTable=previous_calibration,
                PeakFunction=PeakFunction,
                PeakPositions=PeakPositions,
                PeakWindow=PeakWindow,
                PeakWidthPercent=PeakWidthPercent,
                OutputCalibrationTable=f"{output_basename}_pd_diffcal_skip",
                DiagnosticWorkspaces=f"{output_basename}_pd_diag_skip",
            )

    CombineDiffCal(
        PixelCalibration=cc_diffcal,
        GroupedCalibration=f"{output_basename}_pd_diffcal",
        CalibrationWorkspace="_tmp_data_aligned",
        MaskWorkspace=f"{output_basename}_pd_diffcal_mask",
        OutputWorkspace=f"{output_basename}_cc_pd_diffcal_tmp",
    )

    DeleteWorkspace("_tmp_data_aligned")

    out_table = CreateEmptyTableWorkspace(OutputWorkspace=f"{output_basename}_cc_pd_diffcal")
    out_table.addColumn("int", "detid")
    out_table.addColumn("double", "difc")
    out_table.addColumn("double", "difa")
    out_table.addColumn("double", "tzero")
    num_hist = data_ws.getNumberHistograms()
    for i in range(num_hist):
        difc_bak = mtd["calib_table_bak"].row(i)["DIFC"]
        if i in to_skip:
            difc_calib = mtd[f"{output_basename}_pd_diffcal_skip"].row(i)["difc"]
        else:
            difc_calib = mtd[f"{output_basename}_cc_pd_diffcal_tmp"].row(i)["difc"]
        if mtd[f"{output_basename}_pd_diffcal_mask"].readY(i)[0] == 0.0:
            diff_difc = abs(difc_bak - difc_calib) / difc_calib * 100.0
        else:
            diff_difc = np.inf
        if diff_difc >= BadCalibThreshold:
            difc_calib = difc_bak
        new_row = {
            "detid": mtd[f"{output_basename}_cc_pd_diffcal_tmp"].row(i)["detid"],
            "difc": difc_calib,
            "difa": mtd[f"{output_basename}_cc_pd_diffcal_tmp"].row(i)["difa"],
            "tzero": mtd[f"{output_basename}_cc_pd_diffcal_tmp"].row(i)["tzero"],
        }
        out_table.addRow(new_row)

    DeleteWorkspace(f"{output_basename}_cc_pd_diffcal_tmp")

    return mtd[f"{output_basename}_cc_pd_diffcal"]


def pdcalib_for_powgen(
    wks_in, TofBinning, previous_calibration, PeakFunction, PeakPositions, PeakWindow, PeakWidthPercent, OutPDCalib, OutPDCalibDiag
):
    """This will perform PDCalibration for input workspace. This is a special function
    defined for POWGEN for which it seems that we need to conduct the calibration
    three times consecutively.

    :param wks_in: Input workspace
    :param TofBinning: TofBinning parameter of PDCalibration, default (300,-.001,16666.7)
    :param previous_calibration: Optional previous diffcal workspace
    :param PeakFunction: PeakFunction parameter of PDCalibration, default 'IkedaCarpenterPV'
    :param PeakWindow: PeakWindow parameter of PDCalibration, default 0.1
    :param PeakWidthPercent: PeakWidthPercent parameter of PDCalibration, default None
    :param OutPDCalib: Output calibration table
    :param OutPDCalibDiag: Output diagnostics workspace group
    :return: Calibration table workspace and diagnostics workspace group
    """

    PDCalibration(
        InputWorkspace=wks_in,
        TofBinning=TofBinning,
        PreviousCalibrationTable=previous_calibration,
        PeakFunction=PeakFunction,
        PeakPositions=PeakPositions,
        PeakWindow=PeakWindow,
        PeakWidthPercent=PeakWidthPercent,
        OutputCalibrationTable="PDCalib",
        MaskWorkspace="PDCalib_mask",
        DiagnosticWorkspaces="diag",
    )
    PDCalibration(
        InputWorkspace=wks_in,
        TofBinning=[TofBinning[0], TofBinning[1] / 2, TofBinning[2]],
        PreviousCalibrationTable="PDCalib",
        PeakFunction=PeakFunction,
        PeakPositions=PeakPositions,
        PeakWindow=PeakWindow,
        PeakWidthPercent=PeakWidthPercent / 2.0,
        OutputCalibrationTable="PDCalib",
        MaskWorkspace="PDCalib_mask",
        DiagnosticWorkspaces="diag",
    )
    PDCalibration(
        InputWorkspace=wks_in,
        TofBinning=[TofBinning[0], TofBinning[1] / 2, TofBinning[2]],
        PreviousCalibrationTable="PDCalib",
        PeakFunction=PeakFunction,
        PeakPositions=PeakPositions,
        PeakWindow=PeakWindow,
        PeakWidthPercent=PeakWidthPercent / 2.0,
        OutputCalibrationTable=OutPDCalib,
        MaskWorkspace="PDCalib_mask",
        DiagnosticWorkspaces=OutPDCalibDiag,
    )
    DeleteWorkspace("PDCalib")
    DeleteWorkspace("PDCalib_mask")
    DeleteWorkspace("diag")


def do_group_calibration(data_ws, group_ws, previous_calibration=None, output_basename="group_calibration", cc_kwargs={}, pdcal_kwargs={}):
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

    cc_diffcal, to_skip = cc_calibrate_groups(data_ws, group_ws, output_basename, previous_calibration, **cc_kwargs)

    diffcal = pdcalibration_groups(data_ws, group_ws, cc_diffcal, to_skip, output_basename, previous_calibration, **pdcal_kwargs)

    return diffcal


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

    calibrant_file = args.get("CalibrantFile", None)
    if calibrant_file is None:
        calibrant = args["Calibrant"]
    groups = args["Groups"]
    out_groups_by = args.get("OutputGroupsBy", "Group")
    sample_env = args.get("SampleEnvironment", "UnknownSampleEnvironment")
    mask = args.get("Mask")
    instrument = args.get("Instrument", "NOM")
    cc_kwargs = args.get("CrossCorrelate", {})
    pdcal_kwargs = args.get("PDCalibration", {})
    previous_calibration = args.get("PreviousCalibration")

    date = str(args.get("Date", datetime.datetime.now().strftime("%Y_%m_%d")))
    caldirectory = str(args.get("CalDirectory", os.path.abspath(".")))

    if calibrant_file is not None:
        ws = Load(calibrant_file)
        calibrant = ws.getRun().getProperty("run_number").value
    else:
        filename = f"{instrument}_{calibrant}"
        ws = Load(filename)

    calfilename = f"{caldirectory}/{instrument}_{calibrant}_{date}_{sample_env}.h5"
    logger.notice(f"going to create calibration file: {calfilename}")

    groups = LoadDetectorsGroupingFile(groups, InputWorkspace=ws)

    if mask:
        mask = LoadMask(instrument, mask)
        MaskDetectors(ws, MaskedWorkspace=mask)

    if previous_calibration:
        previous_calibration = LoadDiffCal(previous_calibration, MakeGroupingWorkspace=False, MakeMaskWorkspace=False)

    diffcal = do_group_calibration(ws, groups, previous_calibration, cc_kwargs=cc_kwargs, pdcal_kwargs=pdcal_kwargs)
    mask = mtd["group_calibration_pd_diffcal_mask"]

    CreateGroupingWorkspace(InputWorkspace=ws, GroupDetectorsBy=out_groups_by, OutputWorkspace="out_groups")
    SaveDiffCal(CalibrationWorkspace=diffcal, MaskWorkspace=mask, GroupingWorkspace=mtd["out_groups"], Filename=calfilename)


if __name__ == "__main__":
    infile = os.path.abspath(sys.argv[1])
    process_json(infile)
