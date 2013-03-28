#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MERLIN 
#
# This is a simple example for running calibration for and calibration run of MERLIN. Execute all.
#
# Here we run the calibration of MERLIN or selected part of MERLIN
# (excluding short tubes of door 3) 
# This calibration is organised by the function CalibrateMerlin,
# which takes a string consisting of the run number of a calibration run number as argument.
# The workspace with calibrated instrument is saved to a Nexus file
#
from mantid.api import WorkspaceFactory  # For table worskspace of calibrations
from mantid.kernel import config  # To set default instrument to MERLIN
from tube_calib_fit_params import * # To handle fit parameters
from ideal_tube import * # For ideal tube
from tube_calib import *  # For tube calibration functions
from tube_spec import * # For tube specification class

def CalibrateMerlin( RunNumber ):
   '''
   RunNumber is the run number of the calibration.
   '''

   # == Set parameters for calibration ==
   previousDefaultInstrument = config['default.instrument']
   config['default.instrument']="MERLIN"
   filename = str(RunNumber) # Name of calibration run. 
   rangeLower = 3000 # Integrate counts in each spectra from rangeLower to rangeUpper 
   rangeUpper = 20000 #

   # Set parameters for ideal tube. 
   Left = 2.0 # Where the left end of tube should be in pixels (target for AP)
   Centre = 512.5 # Where the centre of the tube should be in pixels (target for CP)
   Right = 1023.0 # Where the right of the tube should be in pxels (target for BP)
   ActiveLength = 2.9 # Active length of tube in Metres

   # Set initial parameters for peak finding
   ExpectedHeight = 1000.0 # Expected Height of Peaks (initial value of fit parameter)
   ExpectedWidth = 32.0 # Expected width of centre peak (initial value of fit parameter)
   ExpectedPositions = [35.0, 512.0, 989.0] # Expected positions of the edges and peak (initial values of fit parameters)

   # Set what we want to calibrate (e.g whole intrument or one door )
   CalibratedComponent = 'MERLIN'  # Calibrate whole instrument 
    
   # Get calibration raw file and integrate it    
   rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
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
   iTube.constructTubeFor3PointsMethod ( Left, Right, Centre, ActiveLength )

   # Get fitting parameters
   fitPar = TubeCalibFitParams( ExpectedPositions, ExpectedHeight, ExpectedWidth, ThreePointMethod=True )

   print "Created objects needed for calibration."

   # == Get the calibration and put results into calibration table ==
   # also put peaks into PeakFile
   getCalibration( CalibInstWS, thisTubeSet, calibrationTable,  fitPar, iTube, ExcludeShortTubes=ActiveLength )
   print "Got calibration (new positions of detectors) and put slit peaks into file TubeDemoMerlin01.txt"

   # == Apply the Calibation ==
   ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
   print "Applied calibration"

   # == Save workspace ==
   SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoMerlinResult.nxs',"Result of Running TubeCalibDemoMerlin_Simple.py")
   print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMerlinResult.nxs"
   
   # == Reset dafault instrument ==
   config['default.instrument'] = previousDefaultInstrument
   
   # ==== End of CalibrateMerlin() ====

CalibrateMerlin( 12024 )