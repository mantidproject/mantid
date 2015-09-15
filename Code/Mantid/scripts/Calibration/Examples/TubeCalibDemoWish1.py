#pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM - Execute this
#
# Here we run the calibration of WISH panel03
# We base the ideal tube on one tube of this door.
#
import tube
reload(tube)
from tube_spec import TubeSpec
import tube_calib #from tube_calib import constructIdealTubeFromRealTube
from tube_calib_fit_params import TubeCalibFitParams

filename = 'WISH00017701.raw' # Calibration run ( found in \\isis\inst$\NDXWISH\Instrument\data\cycle_11_1 )
rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
CalibInstWS = Integration( rawCalibInstWS, RangeLower=1, RangeUpper=20000 )
DeleteWorkspace(rawCalibInstWS)
print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate"

CalibratedComponent = 'WISH/panel03/tube038'

# Set fitting parameters
eP = [65.0, 113.0, 161.0, 209.0, 257.0, 305.0, 353.0, 401.0, 449.0]
ExpectedHeight = 2000.0 # Expected Height of Gaussian Peaks (initial value of fit parameter)
ExpectedWidth = 32.0 # Expected width of Gaussian peaks in pixels  (initial value of fit parameter)
fitPar = TubeCalibFitParams( eP, ExpectedHeight, ExpectedWidth )
fitPar.setAutomatic(True)
print "Created objects needed for calibration."
func_form = 9*[1]

# Use first tube as ideal tube
tube1 = TubeSpec(CalibInstWS)
tube1.setTubeSpecByString('WISH/panel03/tube038')
iTube = tube_calib.constructIdealTubeFromRealTube( CalibInstWS, tube1, fitPar, func_form)

known_pos = iTube.getArray()
print known_pos

# Get the calibration and put it into the calibration table
calibrationTable = tube.calibrate( CalibInstWS, 'WISH/panel03', known_pos, func_form, fitPar=fitPar)
print "Got calibration (new positions of detectors)"

#Apply the calibration
ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
print "Applied calibration"
