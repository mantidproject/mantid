#pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MERLIN
#
# This is a simple example for running calibration for and calibration run of MERLIN.
# It uses the CalibrateMerlin Function
#
# Here we run the calibration of MERLIN or selected part of MERLIN
# (excluding short tubes of door 3)
# This calibration is organised by the function CalibrateMerlin,
# which takes a string consisting of the run number of a calibration run number as argument.
# The workspace with calibrated instrument is saved to a Nexus file
#

import tube
from tube_calib_fit_params import TubeCalibFitParams
import numpy

RunNumber = 12024
def CalibrateMerlin(RunNumber):
    # == Set parameters for calibration ==
    previousDefaultInstrument = config['default.instrument']
    config['default.instrument']="MERLIN"
    filename = str(RunNumber) # Name of calibration run.
    rangeLower = 3000 # Integrate counts in each spectra from rangeLower to rangeUpper
    rangeUpper = 20000 #

    # Set parameters for ideal tube.
    Left = 2.0 # Where the left end of tube should be in pixels (target for AP)
    Centre = 512.5 # Where the centre of the tube should be in pixels (target for CP)
    Right = 1023.0 # Where the right of the tube should be in pixels (target for BP)
    ActiveLength = 2.9 # Active length of tube in Metres

    # Set initial parameters for peak finding
    ExpectedHeight = 1000.0 # Expected Height of Gaussian Peaks (initial value of fit parameter)
    ExpectedWidth = 32.0 # Expected width of centre peak in Pixels (initial value of fit parameter)
    ExpectedPositions = [35.0, 512.0, 989.0] # Expected positions of the edges and peak in pixels (initial values of fit parameters)

    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MERLIN'  # Calibrate door 2

    # Get calibration raw file and integrate it
    print filename
    rawCalibInstWS = LoadRaw(filename)
    # 'raw' in 'rawCalibInstWS' means unintegrated.
    print "Integrating Workspace"
    CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
    DeleteWorkspace(rawCalibInstWS)
    print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate"

    # == Create Objects needed for calibration ==

    ## In the merlin case, the positions are usually given in pixels, instead of being given in
    ## meters, to convert to meter and put the origin in the center, we have to apply the following
    ## transformation:
    ##
    ## pos = pixel * length/npixels - length/2 = length (pixel/npixels - 1/2)
    ##
    ## for merlin: npixels = 1024

    knownPos = ActiveLength * (numpy.array([Left, Centre, Right])/1024.0 - 0.5)
    funcForm = 3*[1]

    # Get fitting parameters
    fitPar = TubeCalibFitParams( ExpectedPositions, ExpectedHeight, ExpectedWidth, margin=40)

    print "Created objects needed for calibration."

    # == Get the calibration and put results into calibration table ==
    # also put peaks into PeakFile
    calibrationTable,peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcForm,\
                                      outputPeak=True, fitPar=fitPar, plotTube=range(0,280,20))
    print "Got calibration (new positions of detectors) and put slit peaks into file TubeDemoMerlin01.txt"

    # == Apply the Calibation ==
    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
    print "Applied calibration"

    # == Save workspace ==
    #SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoMerlinResult.nxs',"Result of Running TubeCalibDemoMerlin_Simple.py")
    #print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMerlinResult.nxs"

    # == Reset dafault instrument ==
    config['default.instrument'] = previousDefaultInstrument

    # ==== End of CalibrateMerlin() ====

CalibrateMerlin( RunNumber )
