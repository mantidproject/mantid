#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR WISH
#
# Here we run the calibration of WISH panel03 using a simple CalibrateWish function.
#
from mantid.api import WorkspaceFactory  # For table worskspace of calibrations
from mantid.kernel import config  # To set default instrument to WISH
from ideal_tube import * # for ideal tube
from tube_calib_fit_params import * # To handle fit parameters
from tube_calib import *  # For tube calibration functions
from tube_spec import * # For tube specification class

def CalibrateWish( RunNumber, PanelNumber ):
   '''
   RunNumber is the run number of the calibration.
   PanelNumber is a string of two-digit number of the panel being calibrated
   '''
   # == Set parameters for calibration ==
   previousDefaultInstrument = config['default.instrument']
   config['default.instrument']="WISH"
   filename = str(RunNumber)
   CalibratedComponent = 'WISH/panel'+PanelNumber

   
   # Get calibration raw file and integrate it
   print "Loading",filename 
   rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
   CalibInstWS = Integration( rawCalibInstWS, RangeLower=1, RangeUpper=20000 )
   DeleteWorkspace(rawCalibInstWS)
   print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate" 

   #Create Calibration Table
   calibrationTable = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
   calibrationTable.addColumn(type="int",name="Detector ID")  # "Detector ID" column required by ApplyCalibration
   calibrationTable.addColumn(type="V3D",name="Detector Position")  # "Detector Position" column required by ApplyCalibration

   # Specify panel of WISH instrument
   thisTubeSet = TubeSpec(CalibInstWS)
   print "Calibrating",CalibratedComponent
   thisTubeSet.setTubeSpecByString(CalibratedComponent)

   # Give y-positions of slit points (gotten for converting first tube's slit point to Y)
   iTube = IdealTube()
   iTube.setArray([-0.41,-0.31,-0.21,-0.11,-0.02, 0.09, 0.18, 0.28, 0.39 ])

   # Set fitting parameters
   eP = [57.5, 107.0, 156.5, 206.0, 255.5, 305.0, 354.5, 404.0, 453.5]
   fitPar = TubeCalibFitParams( eP, 2000, 32 )

   print "Created objects needed for calibration."

   # Get the calibration and put it into the calibration table
   getCalibration( CalibInstWS, thisTubeSet, calibrationTable, fitPar, iTube)
   print "Got calibration (new positions of detectors)"
    
   #Apply the calibration
   ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
   print "Applied calibration"
   
   # == Save workspace ==
   nexusName = "TubeCalibDemoWish"+PanelNumber+"Result.nxs"
   SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoWishResult.nxs',"Result of Running TubeCalibWishMerlin_Simple.py")
   print "saved calibrated workspace (CalibInstWS) into Nexus file",nexusName
      
   # == Reset dafault instrument ==
   config['default.instrument'] = previousDefaultInstrument

   # ==== End of CalibrateWish() ====
   
CalibrateWish( 17701, '03' )