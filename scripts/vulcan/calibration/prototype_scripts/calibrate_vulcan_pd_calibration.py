from mantid.simpleapi import (LoadEventAndCompress, CropWorkspace, Plus,
                              PDCalibration, SaveDiffCal, mtd, LoadInstrument,
                              AlignDetectors, Rebin, SaveNexusProcessed, CreateGroupingWorkspace)
from mantid.simpleapi import LoadEventNexus

instrument = "VULCAN"


def main_calibration(load_full=False, bin_step=-.001):

    # Define diamond run and vanadium run (None)
    # dia_run = 164960
    # dia_run = 192217
    dia_runs = [192227, 192228, 192229, 192230]

    # Load data and process
    dia_wksp = "%s_%d" % (instrument, dia_runs[0])

    bad_pulse_threshold = 10
    if load_full:
        # Load full size data
        LoadEventNexus(Filename=dia_wksp, OutputWorkspace=dia_wksp)
        # Load more files
        for file_index in range(1, 4):
            dia_wksp_i = "%s_%d" % (instrument, dia_runs[file_index])
            LoadEventNexus(Filename=dia_wksp_i, OutputWorkspace=dia_wksp_i)
            Plus(LHSWorkspace=dia_wksp,
                 RHSWorkspace=dia_wksp_i,
                 OutputWorkspace=dia_wksp, ClearRHSWorkspace=True)
        # Reload insturment (do not trust the old one)
        LoadInstrument(Workspace=dia_wksp,
                       Filename='/SNS/users/wzz/Mantid_Project/mantid/scripts/vulcan/calibration/vulcan-x-alpha/VULCAN_Definition_pete02.xml',
                       MonitorList='-3--2', InstrumentName='VULCAN', RewriteSpectraMap=True)

        # TODO - make this back!    MaxChunkSize=16, FilterBadPulses=bad_pulse_threshold)
    else:
        # Compress and crop
        LoadEventAndCompress(Filename=dia_wksp, OutputWorkspace=dia_wksp,
                             MaxChunkSize=16, FilterBadPulses=bad_pulse_threshold)
        CropWorkspace(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp, XMin=300, XMax=16666.7)

    # generates a workspace of delta-d/d which could be used to estimate uncertainties
    # EstimateResolutionDiffraction(InputWorkspace=dia_wksp, OutputWorkspace='resolution', DeltaTOF=5)

    # start from a previous calibration
    # oldCal='/SNS/PG3/shared/CALIBRATION/2017_1_2_11A_CAL/PG3_PAC_d37861_2017_07_28-ALL.h5'

    # POWGEN uses tabulated reflections for diamond
    # dvalues = (0.3117, 0.3257, 0.3499, 0.4205, 0.4645, 0.4768, 0.4996, 0.5150, 0.5441, 0.5642, 0.5947,
    #            0.6307, .6866, .7283, .8185, .8920, 1.0758, 1.2615, 2.0599)

    dvalues = [0.31173, 0.32571, 0.34987, 0.42049, 0.46451, 0.47679, 0.49961, 0.51499, 0.54411, 0.56414,
               0.60309, 0.63073, 0.68665, 0.72830, 0.81854, 0.89198, 1.07577, 1.26146, 2.05995]

    dia_wksp_name = str(dia_wksp)
    PDCalibration(InputWorkspace=dia_wksp,
                  TofBinning=[300, bin_step, 16666.7],
                  # PreviousCalibration=oldCal,
                  PeakPositions=dvalues,
                  OutputCalibrationTable=instrument,
                  DiagnosticWorkspaces='%s_diag' % dia_wksp,
                  CalibrationParameters='DIFC',
                  MinimumPeakHeight=5)

    # Save
    mask_ws_name = f'{instrument}_mask'
    assert mtd.doesExist(mask_ws_name)
    print(f'instrument: type = {type(instrument)}')
    if isinstance(instrument, str):
        print(f'ADS: {instrument} exists: {mtd.doesExist(instrument)}')

    # 3 group mode
    group_ws_name = 'VULCAN_3Banks_Group'
    group_ws = CreateGroupingWorkspace(InputWorkspace=dia_wksp_name,
                                       GroupDetectorsBy='bank',
                                       OutputWorkspace=group_ws_name)

    SaveNexusProcessed(InputWorkspace=instrument, Filename=f'cal_table.nxs')
    # FIXME - group workspace cannot be saved SaveNexusProcessed(InputWorkspace=group_ws_name, Filename=f'group_table.nxs')

    assert group_ws

    SaveDiffCal(CalibrationWorkspace=instrument,
                MaskWorkspace=mask_ws_name,
                GroupingWorkspace=group_ws_name,
                Filename="%s_pdcalibration.h5" % instrument)

    SaveNexusProcessed(InputWorkspace='%s_diag' % dia_wksp, Filename=f'{instrument}_pdcalib_diag.nxs')


def verify_calibration(dia_wksp):

    dbinning = .01, -.001, 3.
    AlignDetectors(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp, CalibrationWorkspace=instrument)
    CropWorkspace(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp,
                  XMin=dbinning[0], XMax=dbinning[2])
    Rebin(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp, Params=dbinning)

    SaveNexusProcessed(InputWorkspace='%s_diag' % dia_wksp,
                       Filename="%s_pdcalibration_diagnostics.h5" % instrument)

    return


if __name__ == '__main__':
    main_calibration(load_full=True, bin_step=-0.0003)
