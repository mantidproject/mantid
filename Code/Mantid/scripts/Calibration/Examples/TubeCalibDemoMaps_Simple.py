#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MAPS 
#
# This is a simple example for running calibration for and calibration run of MAPS.
# It uses the CalibrateMaps function 

#
import tube
from tube_calib_fit_params import TubeCalibFitParams

def CalibrateMaps( RunNumber ):
   '''
   RunNumber is the run number for the calibration.
   '''

   # == Set parameters for calibration ==
   previousDefaultInstrument = config['default.instrument']
   config['default.instrument']="MAPS"
   filename = str(RunNumber) # Name of calibration run
   print "Filename",filename
   rangeLower = 2000 # Integrate counts in each spectra from rangeLower to rangeUpper 
   rangeUpper = 10000 #
   
   # Set initial parameters for peak finding
   ExpectedHeight = -1000.0 # Expected Height of Gaussian Peaks  (initial value of fit parameter)
   ExpectedWidth = 8.0 # Expected width of Gaussian peaks in pixels (initial value of fit parameter)
   ExpectedPositions = [4.0, 85.0, 128.0, 165.0, 252.0] #Expected positions of the edges and Gaussian peaks in pixels (initial values of fit parameters)

   # Set what we want to calibrate (e.g whole intrument or one door )
   CalibratedComponent = 'MAPS'  # Calibrate all
 
    
   # Get calibration raw file and integrate it    
   rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
   print "Integrating Workspace"
   CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
   DeleteWorkspace(rawCalibInstWS)
   print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate" 

   # == Create Objects needed for calibration ==

   # First array gives positions in Metres and second array gives type 1=Gaussian peak 2=edge.
   # An intelligent guess is used here that is not correct for all tubes.
   knownPos, funcForm = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2] 

   # Get fitting parameters
   fitPar = TubeCalibFitParams( ExpectedPositions, ExpectedHeight, ExpectedWidth, margin=15 )
   fitPar.setAutomatic(True)

   print "Created objects needed for calibration."

   # == Get the calibration and put results into calibration table ==
   calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, 
                                                knownPos, funcForm, fitPar = fitPar, 
                                                outputPeak=True)
   print "Got calibration (new positions of detectors) "

   # == Apply the Calibation ==
   ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
   print "Applied calibration"


   # == Save workspace ==
   SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoMapsResult.nxs',"Result of Running MAPS Calibration")
   print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMapsResult.nxs"
   
   # == Reset dafault instrument ==
   config['default.instrument'] = previousDefaultInstrument

   # ==== End of CalibrateMaps() ====

CalibrateMaps( 14919 ) #found at \\isis\inst$\NDXMAPS\Instrument\data\cycle_09_5
