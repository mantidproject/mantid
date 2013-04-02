#
# TUBE CALIBRATION DEMONSTRATION PROGRAM - Execute this
#
# Here we run the calibration of WISH panel03
# We explicitly set the ideal tube with values corresponding to Y position in metres
#
from mantid.api import WorkspaceFactory  # For table worskspace of calibrations
from ideal_tube import * # for ideal tube
from tube_calib_fit_params import * # To handle fit parameters
from tube_calib import *  # For tube calibration functions
from tube_spec import * # For tube specification class
   
# Get calibration raw file and integrate it 
path = r"C:/Temp/" # Path name of folder containing input and output files
filename = 'WISH00017701.raw' # Calibration run ( found in \\isis\inst$\NDXWISH\Instrument\data\cycle_11_1 )
rawCalibInstWS = Load(path+filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
CalibInstWS = Integration( rawCalibInstWS, RangeLower=1, RangeUpper=20000 )
DeleteWorkspace(rawCalibInstWS)
print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate" 

#Create Calibration Table
calibrationTable = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
calibrationTable.addColumn(type="int",name="Detector ID")  # "Detector ID" column required by ApplyCalibration
calibrationTable.addColumn(type="V3D",name="Detector Position")  # "Detector Position" column required by ApplyCalibration

# Specify panel 03 of WISH instrument
thisTubeSet = TubeSpec(CalibInstWS)
thisTubeSet.setTubeSpecByString('WISH/panel03')

# Give y-positions of slit points (gotten for converting first tube's slit point to Y)
iTube = IdealTube()
# set positions in Metres of Gaussian peaks (no edges).
iTube.setArray([-0.41,-0.31,-0.21,-0.11,-0.02, 0.09, 0.18, 0.28, 0.39 ])

# Set fitting parameters
eP = [57.5, 107.0, 156.5, 206.0, 255.5, 305.0, 354.5, 404.0, 453.5]
ExpectedHeight = 2000.0 # Expected Height of Gaussian Peaks (initial value of fit parameter)
ExpectedWidth = 32.0 # Expected width of Gaussian peaks in pixels  (initial value of fit parameter)

fitPar = TubeCalibFitParams( eP, ExpectedHeight, ExpectedWidth )

print "Created objects needed for calibration."

# Get the calibration and put it into the calibration table
getCalibration( CalibInstWS, thisTubeSet, calibrationTable, fitPar, iTube)
print "Got calibration (new positions of detectors)"
    
#Apply the calibration
ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
print "Applied calibration"