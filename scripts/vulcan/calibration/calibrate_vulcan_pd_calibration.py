from mantid.simpleapi import (LoadEventAndCompress, LoadEventNeXus, CropWorkspace,
                              PDCalibration, SaveDiffCal, mtd,
                              AlignDetectors, Rebin, SaveNexusProcessed)
instrument = "VULCAN"


def main_calibration(load_full=False, bin_step=-.001):

    # Define diamond run and vanadium run (None)
    dia_run = 164960

    # Load data and process
    dia_wksp = "%s_%d" % (instrument, dia_run)

    bad_pulse_threshold = 10
    if load_full:
        # Load full size data
        LoadEventNeXus(Filename=dia_wksp, OutputWorkspace=dia_wksp,
                       MaxChunkSize=16, FilterBadPulses=bad_pulse_threshold)
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
    dvalues = (0.3117, 0.3257, 0.3499, 0.4205, 0.4645, 0.4768, 0.4996, 0.5150, 0.5441, 0.5642, 0.5947,
               0.6307, .6866, .7283, .8185, .8920, 1.0758, 1.2615, 2.0599)

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
    assert mtd.doesExists(mask_ws_name)
    SaveDiffCal(CalibrationWorkspace=instrument,
                MaskWorkspace=mask_ws_name,
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
    main_calibration(bin_step=-0.0003)
