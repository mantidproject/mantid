#########################################################
# Applies two tests to the (white beam vanadium) workspace inputWS
# and marks failing spectra bad by writing to the detector list and
# writing zeros to the histograms associated with those detectors
########################################################
from os import remove
from mantidsimple import *

OUT_WS_PREFIX = 'mask_'

def appendMaskFile(inFname, outfile) :
  try:
    file=open(inFname,'r')
    data=file.read()
    file.close()
    outfile.write(data)
  finally:
    if ( os.access(inFname, os.W_OK) ):
	  os.remove(inFname)

def numberFromCommaSeparated(CommaSeparated) :
  num = CommaSeparated.count(',')
  if num == 0 : num = 0
  else : num = num + 1
  return num
  
def SingleWBV( inputWS, outputWS, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, oFile ) :
  if oFile == '' :
    limitsTempFile = ''
    MedianTempFile = ''
  else :
    limitsTempFile = '_DetectorTest_swbv_FDOL_tempfile'
    MedianTempFile = '_DetectorTest_swbv_MDT_tempfile'
  
  FDOL = FindDetectorsOutsideLimits( inputWS, outputWS, HighAbsolute, LowAbsolute, OutputFile=limitsTempFile )	#for usage see www.mantidproject.org/FindDetectorsOutsideLimits
  MaskDetectors(Workspace=inputWS, DetectorList=FDOL.getPropertyValue("BadDetectorIDs") )			#for usage see www.mantidproject.org/MaskDetectors
  
  MDT = MedianDetectorTest( InputWorkspace=inputWS, OutputWorkspace=outputWS, SignificanceTest=NumErrorBars, LowThreshold=LowMedian, HighThreshold=HighMedian, OutputFile=MedianTempFile )#for usage see www.mantidproject.org/
  MaskDetectors(Workspace=inputWS, DetectorList=MDT.getPropertyValue("BadDetectorIDs"))						#for usage see www.mantidproject.org/MaskDetectors
  
  fileOutputs = ''
  #--get any file output to add to the main output file
  if oFile != '':
    for algor in (FDOL, MDT):
	#the algorithms can add a path to filenames so we need to query the algorithms to be sure to get the full path
	file = open(algor.getPropertyValue("OutputFile"), 'r')
        fileOutputs = fileOutputs + file.read()
        file.close()
        os.remove(algor.getPropertyValue("OutputFile"))	

  numBad = numberFromCommaSeparated(MDT.getPropertyValue("BadDetectorIDs"))
  return (fileOutputs, numBad)
  
def workspaceExists(workS) :
  allNames = mantid.getWorkspaceNames()
  for aName in allNames :
    if aName == workS : return True
  return False
