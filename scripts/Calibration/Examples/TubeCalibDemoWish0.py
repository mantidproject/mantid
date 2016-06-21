#pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM - Execute this
#
# Here we run the calibration of WISH panel03
# We explicitly set the ideal tube with values corresponding to Y position in metres
#
#  The calibration result here is not as good as inside the TubeCalibDemoWish_Simple
#  because we do not consider the upper and lower position of the tubes around the
# WISH instrument.
#
import tube
filename = 'WISH00017701.raw' # Calibration run ( found in \\isis\inst$\NDXWISH\Instrument\data\cycle_11_1 )
rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
CalibInstWS = Integration( rawCalibInstWS, RangeLower=1, RangeUpper=20000 )
DeleteWorkspace(rawCalibInstWS)
print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate"

CalibratedComponent = 'WISH/panel03'

knownPos = [-0.41,-0.31,-0.21,-0.11,-0.02, 0.09, 0.18, 0.28, 0.39 ]
funcForm = 9*[1] # all special points are gaussian peaks

print "Created objects needed for calibration."

# Get the calibration and put it into the calibration table
calibrationTable = tube.calibrate(CalibInstWS, CalibratedComponent,
                                  knownPos, funcForm)
print "Got calibration (new positions of detectors)"

#Apply the calibration
ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
print "Applied calibration"
