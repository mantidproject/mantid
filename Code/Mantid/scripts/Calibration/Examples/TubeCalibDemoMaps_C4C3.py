#pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MAPS - Execute this
#
# Here we run the calibration of a selected part of MAPS consisting of several components
# by running setTubeSpecByString several times.

#
import tube
from mantid.api import WorkspaceFactory  # For table worskspace of calibrations
from tube_spec import TubeSpec
#from tube_calib_fit_params import  # To handle fit parameters


# == Set parameters for calibration ==

filename = 'MAP14919.raw' # Name of calibration run
rangeLower = 2000 # Integrate counts in each spectra from rangeLower to rangeUpper
rangeUpper = 10000 #

# Set what we want to calibrate (e.g whole intrument or one door )
CalibratedComponent1 = 'C4_window'  # Calibrate C4 window
CalibratedComponent2 = 'C3_window'  # Calibrate C3 window


# Get calibration raw file and integrate it
rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
print "Integrating Workspace"
CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
DeleteWorkspace(rawCalibInstWS)
print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate"

# == Create Objects needed for calibration ==

# Specify components to calibrate
thisTubeSet = TubeSpec(CalibInstWS)
thisTubeSet.setTubeSpecByString(CalibratedComponent1)
thisTubeSet.setTubeSpecByString(CalibratedComponent2)

# Specify the known positions
knownPos = [-0.50,-0.16,-0.00, 0.16, 0.50 ]
funcForm = [2,1,1,1,2]

print "Created objects needed for calibration."

# == Get the calibration and put results into calibration table ==

calibrationTable, peakTable = tube.calibrate(CalibInstWS, thisTubeSet, knownPos, funcForm,\
    outputPeak=True)
print "Got calibration (new positions of detectors) "

# == Apply the Calibation ==
ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
print "Applied calibration"


# == Save workspace ==
#SaveNexusProcessed( CalibInstWS, path+'TubeCalibDemoMapsResult.nxs',"Result of Running TCDemoMaps.py")
print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMapsResult.nxs"

