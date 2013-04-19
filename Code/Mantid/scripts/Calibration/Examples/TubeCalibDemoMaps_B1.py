#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MAPS - Execute this
#
# Here we run the calibration of a selected part of MAPS 

#
from mantid.api import WorkspaceFactory  # For table worskspace of calibrations
from tube_calib_fit_params import * # To handle fit parameters
from ideal_tube import * # For ideal tube
from tube_calib import *  # For tube calibration functions
from tube_spec import * # For tube specification class


# == Set parameters for calibration ==

path = r"C:/Temp/" # Path name of folder containing input and output files
filename = 'MAPS14919.raw' # Name of calibration run
rangeLower = 2000 # Integrate counts in each spectra from rangeLower to rangeUpper 
rangeUpper = 10000 #


# Set initial parameters for peak finding
ExpectedHeight = -1000.0 # Expected Height of Peaks (initial value of fit parameter)
ExpectedWidth = 8.0 # Expected width of centre peak (initial value of fit parameter)
ExpectedPositions = [4.0, 85.0, 128.0, 165.0, 252.0] # Expected positions of the edges and peak (initial values of fit parameters)

# Set what we want to calibrate (e.g whole intrument or one door )
CalibratedComponent = 'B1_window'  # Calibrate B1 window
 
    
# Get calibration raw file and integrate it    
rawCalibInstWS = Load(path+filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
print "Integrating Workspace"
CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
DeleteWorkspace(rawCalibInstWS)
print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate" 

# == Create Objects needed for calibration ==

#Create Calibration Table
calibrationTable = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
calibrationTable.addColumn(type="int",name="Detector ID")  # "Detector ID" column required by ApplyCalbration
calibrationTable.addColumn(type="V3D",name="Detector Position")  # "Detector Position" column required by ApplyCalbration

# Specify component to calibrate
thisTubeSet = TubeSpec(CalibInstWS)
thisTubeSet.setTubeSpecByString(CalibratedComponent)

# Get ideal tube
iTube = IdealTube()
# The positions of the shadows and ends here are an intelligent guess.
iTube.setPositionsAndForm([-0.50,-0.165,-0.00, 0.165, 0.50 ],[2,1,1,1,2])

# Get fitting parameters
fitPar = TubeCalibFitParams( ExpectedPositions, ExpectedHeight, ExpectedWidth )

print "Created objects needed for calibration."

# == Get the calibration and put results into calibration table ==
# also put peaks into PeakFile
getCalibration( CalibInstWS, thisTubeSet, calibrationTable,  fitPar, iTube, PeakFile=path+'TubeDemoMaps01.txt' )
print "Got calibration (new positions of detectors) "

# == Apply the Calibation ==
ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
print "Applied calibration"


# == Save workspace ==
SaveNexusProcessed( CalibInstWS, path+'TubeCalibDemoMapsResult.nxs',"Result of Running TCDemoMaps.py")
print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMapsResult.nxs"

