#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MERLIN - Execute this
#
# Here we run the calibration of MERLIN or selected part of MERLIN
# (excluding short tubes of door 3) and 
# put the positions of the three points of each tube into a file
# and possibly recalibrate some tubes by overriding their peaks.
#
from mantid.api import WorkspaceFactory  # For table worskspace of calibrations
from tube_calib_fit_params import * # To handle fit parameters
from ideal_tube import * # For ideal tube
from tube_calib import *  # For tube calibration functions
from tube_spec import * # For tube specification class


# == Set parameters for calibration ==

path = r"C:/Temp/" # Path name of folder containing input and output files
filename = 'MER12024.raw' # Calibration run ( found in \\isis\inst$\NDXMERLIN\Instrument\data\cycle_11_5 )
rangeLower = 3000 # Integrate counts in each spectra from rangeLower to rangeUpper 
rangeUpper = 20000 #

# Set parameters for ideal tube. 
Left = 2.0 # Where the left end of tube should be in pixels (target for AP)
Centre = 512.5 # Where the centre of the tube should be in pixels (target for CP)
Right = 1023.0 # Where the right of the tube should be in pixels (target for BP)
ActiveLength = 2.9 # Active length of tube in Metres

# Set initial parameters for peak finding
ExpectedHeight = 1000.0 # Expected Height of Gaussian Peak at centre (initial value of fit parameter)
ExpectedWidth = 32.0 # Expected width of centre peak (initial value of fit parameter)
ExpectedPositions = [35.0, 512.0, 989.0] #  Expected positions of the edges and centre peak in pixels (initial values of fit parameters)

# Set what we want to calibrate (e.g whole intrument or one door )
CalibratedComponent = 'MERLIN'  # Calibrate whole instrument
#CalibratedComponent = 'MERLIN/door3'  # Calibrate door3
#CalibratedComponent = 'door3'  # Calibrate door3. 
    
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
# Call short cut method to construct ideal tube of the 3 point method format.
iTube.constructTubeFor3PointsMethod ( Left, Right, Centre, ActiveLength )

# Get fitting parameters
fitPar = TubeCalibFitParams( ExpectedPositions, ExpectedHeight, ExpectedWidth, ThreePointMethod=True )

print "Created objects needed for calibration."

# == Get the calibration and put results into calibration table ==
# also put peaks into PeakFile
getCalibration( CalibInstWS, thisTubeSet, calibrationTable,  fitPar, iTube, ExcludeShortTubes=ActiveLength, PeakFile=path+'TubeDemoMerlin01.txt' )
print "Got calibration (new positions of detectors) and put slit peaks into file TubeDemoMerlin01.txt"

# == Apply the Calibation ==
ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
print "Applied calibration"

# == Override Selected Tubes ==
# Now we may override selected tubes
# To do this set Overriding to true, else set it to false (or don't run this part of script)
Overriding = False
if( Overriding ):
   # Here we can override selected tubes. 
   # Values not requiring overriding can be copied from the 
   # PeakFile created by the previous call of getCalibration.
   
   #Specify tube_3_1 and its override
   tube31 = TubeSpec( CalibInstWS)
   tube31.setTubeSpecByString('MERLIN/door3/tube_3_1')
   override31 = [28.821573928922724, 520.0, 997.359428534161]
   #Specify tube_3_2 and its override
   tube32 = TubeSpec( CalibInstWS)
   tube32.setTubeSpecByString('MERLIN/door3/tube_3_2')
   override32 = [29.088837989024192, 510.0, 997.729400488547]
   #Specify tube_3_3 and its override
   tube33 = TubeSpec( CalibInstWS)
   tube33.setTubeSpecByString('MERLIN/door3/tube_3_3')
   override33 = [28.60189163158781, 510.0, 997.4536392249629]
   
   #Create another calibration table for overrides
   ovrCalibrationTable = CreateEmptyTableWorkspace(OutputWorkspace="OvrCalibTable")
   ovrCalibrationTable.addColumn(type="int",name="Detector ID")  # "Detector ID" column required by ApplyCalbration
   ovrCalibrationTable.addColumn(type="V3D",name="Detector Position")  # "Detector Position" column required by ApplyCalbration   
   
   # Put the calibrations of the overrides into the calibration table
   getCalibration( CalibInstWS, tube31, ovrCalibrationTable,  fitPar, iTube, OverridePeaks=override31 )
   getCalibration( CalibInstWS, tube32, ovrCalibrationTable,  fitPar, iTube, OverridePeaks=override32 )
   getCalibration( CalibInstWS, tube33, ovrCalibrationTable,  fitPar, iTube, OverridePeaks=override33 )
   
   #Apply the calibration
   ApplyCalibration( Workspace=CalibInstWS, PositionTable=ovrCalibrationTable)
   print "got and applied override calibrations"

# == Save workspace ==
SaveNexusProcessed( CalibInstWS, path+'TubeCalibDemoMerlinResult.nxs',"Result of Running TCDemoMerlin.py")
print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMerlinResult.nxs"

