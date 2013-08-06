#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MERLIN 
#
# This is example that allows some calibration slit peaks to be adjusted.
# First execute the part from thwe beginning to the end of the CalibrateMerlin function
# then the later parts.
#
# Here we run the calibration of MERLIN or selected part of MERLIN
# (excluding short tubes of door 3) 
# This calibration is organised by the function CalibrateMerlin,
# which takes a string consisting of the run number of a calibration run number as argument.
# The workspace with calibrated instrument is saved to a Nexus file
#
import tube
from tube_calib_fit_params import * # To handle fit parameters
import os

def CalibrateMerlin( RunNumber, UsePeakFile=False ):
# Run number must include any leading zeros that appear in the file name of the run.

   # == Set parameters for calibration ==

   # File details
   filename = 'MER'+str(RunNumber) # Name of calibration run
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
   CalibratedComponent = 'MERLIN/door8'  # Calibrate whole instrument 
    
   # Get calibration raw file and integrate it    
   rawCalibInstWS = LoadRaw(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
   print "Integrating Workspace"
   CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
   DeleteWorkspace(rawCalibInstWS)
   print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate" 

   # == Create Objects needed for calibration ==

   # Get ideal tube
   # the known positions are given in pixels inside the tubes and transformed to provide the positions
   # with the center of the tube as the origin
   knownPositions = ActiveLength*(numpy.array([ Left, Right, Centre])/1024 - 0.5)
   funcForm = [1,1,1]

   print "Created objects needed for calibration."

   # == Get the calibration and put results into calibration table ==
   # also put peaks into PeakFile
   saveDirectory = config['defaultsave.directory']
   peakFileName = "TubeCalibDemoMerlin_Peaks.txt"

   # pass more parameters to tube.calibrate by name
   extra_options = dict()
   extra_options['excludeShortTubes']=ActiveLength
   extra_options['outputPeak']=True
   if(not UsePeakFile):
      # Get fitting parameters
      fitPar = TubeCalibFitParams( ExpectedPositions, ExpectedHeight, ExpectedWidth )
      fitPar.setAutomatic(True)
      extra_options['fitPar'] = fitPar


   calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, knownPositions, funcForm,
                                        **extra_options)   

   peakFileName = "TubeCalibDemoMerlin_Peaks.txt"
   if UsePeakFile:
      tube.savePeak(peakTable, peakFileName)
      print " Put slit peaks into file",peakFileName, "in save directory",saveDirectory,"." 
     
   print "Got calibration (new positions of detectors)"

   # == Apply the Calibation ==
   ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
   print "Applied calibration"

   # == Save workspace ==
   SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoMerlinResult.nxs',"Result of Running TubeCalibDemoMerlin_Adjustable.py")
   print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMerlinResult.nxs in save directory",saveDirectory,"."
   
   # ==== End of CalibrateMerlin() ====
   # INITIALLY EXECUTE THE CODE FROM THE BEGINNING TO HERE, THEN EACH OF THE TWO CALLS BELOW IN ORDER SEPARATELY

# Run this one first
CalibrateMerlin( 12024 )  

#Then edit the'TubeCalibDemoMerlin_Peaks.txt' file, then run 
CalibrateMerlin( 12024, True)
