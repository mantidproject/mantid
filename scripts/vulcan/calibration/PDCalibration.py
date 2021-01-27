from mantid.simpleapi import *
instrument = "VULCAN"

if instrument == "PG3":
    dia_run = 39169
    van_run = 37830
if instrument == "NOM":
    dia_run = 122825
    van_run = 122827
if instrument == "VULCAN":
    dia_run = 164960
    van_run = 0

dia_wksp = "%s_%d" % (instrument, dia_run)
van_wksp = "%s_%d" % (instrument, van_run)

filterBadPulses=10
LoadEventAndCompress(Filename=dia_wksp, OutputWorkspace=dia_wksp,
                     MaxChunkSize=16, FilterBadPulses=filterBadPulses)
CropWorkspace(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp, XMin=300, XMax=16666.7)

# generates a workspace of delta-d/d which could be used to estimate uncertainties
#EstimateResolutionDiffraction(InputWorkspace=dia_wksp, OutputWorkspace='resolution', DeltaTOF=5)

# start from a previous calibration
#oldCal='/SNS/PG3/shared/CALIBRATION/2017_1_2_11A_CAL/PG3_PAC_d37861_2017_07_28-ALL.h5'

# POWGEN uses tabulated reflections for diamond
dvalues = (0.3117,0.3257,0.3499,0.4205,0.4645,0.4768,0.4996,0.5150,0.5441,0.5642,0.5947,
           0.6307,.6866,.7283,.8185,.8920,1.0758,1.2615,2.0599)

PDCalibration(InputWorkspace=dia_wksp,
              TofBinning=[300,-.001,16666.7],
              #PreviousCalibration=oldCal,
              PeakPositions=dvalues,
              OutputCalibrationTable=instrument,
              DiagnosticWorkspaces='%s_diag' % dia_wksp,
              CalibrationParameters='DIFC',
              MinimumPeakHeight=5)

dbinning=(.01,-.001,3.)
AlignDetectors(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp, CalibrationWorkspace=instrument)
CropWorkspace(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp,
              XMin=dbinning[0], XMax=dbinning[2])
Rebin(InputWorkspace=dia_wksp, OutputWorkspace=dia_wksp, Params=dbinning)

# Create additional mask via vanadium
'''
mask_final = '%s_mask_final' % instrument
van_mask_detdiag = '%s_mask_detdiag' % instrument
Load(Filename=van_wksp,
     OutputWorkspace=van_wksp,
     FilterBadPulses=filterBadPulses)
DetectorDiagnostic(InputWorkspace=van_wksp, OutputWorkspace=van_mask_detdiag,
                   RangeLower=300, RangeUpper=16666.7,  # TOF range to use
                   LowThreshold=10,  # minimum number of counts for a detector
                   LevelsUp=1)  # median calculated from the tube
DeleteWorkspace(van_wksp)

BinaryOperateMasks(
    InputWorkspace1='%s_mask' % instrument,
    InputWorkspace2=van_mask_detdiag,
    OperationType='OR',
    OutputWorkspace=mask_final)
'''

SaveDiffCal(CalibrationWorkspace=instrument,
            MaskWorkspace=mask_final,
            Filename="%s_pdcalibration.h5" % instrument)

SaveNexusProcessed(InputWorkspace='%s_diag' % dia_wksp,
                   Filename="%s_pdcalibration_diagnostics.h5" % instrument)
