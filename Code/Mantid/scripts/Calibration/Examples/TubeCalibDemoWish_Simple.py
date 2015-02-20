#pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR WISH
#
# Here we run the calibration of WISH panel03 using a simple CalibrateWish function.
#
import numpy
import tube
from mantid.simpleapi import *

from tube_calib_fit_params import TubeCalibFitParams

def CalibrateWish( RunNumber, PanelNumber):
    '''
    :param RunNumber: is the run number of the calibration.
    :param PanelNumber: is a string of two-digit number of the panel being calibrated
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

    # Give y-positions of slit points (gotten for converting first tube's slit point to Y)

    # WISH instrument has a particularity. It is composed by a group of upper tubes and lower tubes,
    # they are disposed 3 milimiters in difference one among the other
    lower_tube = numpy.array([-0.41,-0.31,-0.21,-0.11,-0.02, 0.09, 0.18, 0.28, 0.39 ])
    upper_tube = numpy.array(lower_tube+0.003)
    funcForm = 9*[1] # 9 gaussian peaks
    print "Created objects needed for calibration."

    # Get the calibration and put it into the calibration table

    #calibrate the lower tubes
    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, lower_tube, funcForm,\
                                                  rangeList = range(0,76), outputPeak=True)

    #calibrate the upper tubes
    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, upper_tube, funcForm,\
                                    rangeList = range(76,152),\
                                    calibTable=calibrationTable, #give the calibration table to append data\
                                    outputPeak = peakTable #give peak table to append data\
                                                 )

    print "Got calibration (new positions of detectors)"

    #Apply the calibration
    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
    print "Applied calibration"

    # == Save workspace ==
    #uncomment these lines to save the workspace
    #nexusName = "TubeCalibDemoWish"+PanelNumber+"Result.nxs"
    #SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoWishResult.nxs',"Result of Running TubeCalibWishMerlin_Simple.py")
    #print "saved calibrated workspace (CalibInstWS) into Nexus file",nexusName

    # == Reset dafault instrument ==
    config['default.instrument'] = previousDefaultInstrument

    # ==== End of CalibrateWish() ====
if __name__ == "__main__":
  # this file is found on cycle_11_1
    RunNumber = 17701
    PanelNumber = '03'
    CalibrateWish(RunNumber, PanelNumber)
