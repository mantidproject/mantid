#pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MAPS - Execute this
#
# Here we run the calibration of a selected part of MAPS

#
import tube
from tube_calib_fit_params import TubeCalibFitParams
# == Set parameters for calibration ==

filename = 'MAP14919.raw' # Calibration run ( found in \\isis\inst$\NDXMAPS\Instrument\data\cycle_09_5 )
rangeLower = 2000 # Integrate counts in each spectra from rangeLower to rangeUpper
rangeUpper = 10000 #

# Set initial parameters for peak finding
ExpectedHeight = -1000.0 # Expected Height of Gaussian Peaks (initial value of fit parameter)
ExpectedWidth = 8.0 # Expected width of Gaussian peaks in pixels  (initial value of fit parameter)
ExpectedPositions = [4.0, 85.0, 128.0, 161.0, 252.0]
# Expected positions of the edges and Gaussian peaks in pixels (initial values of fit parameters)

# Set what we want to calibrate (e.g whole intrument or one door )
CalibratedComponent = 'B1_window'  # Calibrate B1 window


# Get calibration raw file and integrate it
rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
print "Integrating Workspace"
CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
DeleteWorkspace(rawCalibInstWS)
print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate"

# == Create Objects needed for calibration ==
# The positions of the shadows and ends here are an intelligent guess.
# First array gives positions in Metres and second array gives type 1=Gaussian peak 2=edge.
# See http://www.mantidproject.org/IdealTube for details.
knownPos = [-0.50,-0.165,-0.00, 0.165, 0.50 ]
funcForm = [2,1,1,1,2]

# Get fitting parameters
fitPar = TubeCalibFitParams( ExpectedPositions, ExpectedHeight, ExpectedWidth )
fitPar.setAutomatic(True)

print "Created objects needed for calibration."

# == Get the calibration and put results into calibration table ==
# also put peaks into PeakFile
calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcForm,\
    fitPar=fitPar, outputPeak=True)
print "Got calibration (new positions of detectors) "

# == Apply the Calibation ==
ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
print "Applied calibration"


# == Save workspace ==
SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoMapsResult.nxs',"Result of Running TCDemoMaps_B1.py")
print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMapsResult.nxs"

# == Save Peak File ==
tube.savePeak(peakTable, 'TubeDemoMaps01.txt')
