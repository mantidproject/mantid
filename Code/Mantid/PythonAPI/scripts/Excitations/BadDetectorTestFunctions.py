#########################################################
# Applies two tests to the (white beam vanadium) workspace inputWS
# and marks failing spectra bad by writing to the detector list and
# writing zeros to the histograms associated with those detectors
########################################################
from mantidsimple import *

def appendMaskFile(inFname, outfile) :
  try:
    file=open(inFname,'r')
    data=file.read()
    file.close()
    outfile.write(data)
  finally:
    if ( os.access(inFname, os.W_OK) ):
	  os.remove(inFname)

def SingleWBV( inputWS, outputWS, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, oFile ) :
  if oFile != "" : tempFile = oFile+'_swbv_fdol'
  else : tempFile = ''
  FDOL = FindDetectorsOutsideLimits( inputWS, outputWS, HighAbsolute, LowAbsolute, OutputFile=tempFile )	#for usage see www.mantidproject.org/FindDetectorsOutsideLimits
  MaskDetectors(Workspace=inputWS, DetectorList=FDOL.getPropertyValue("BadDetectorIDs") )					#for usage see www.mantidproject.org/MaskDetectors
  
  if oFile != "" : tempFile = oFile+'_swbv_mdt'
  else : tempFile = ''
  MDT = MedianDetectorTest( InputWorkspace=inputWS, OutputWorkspace=outputWS, SignificanceTest=NumErrorBars, LowThreshold=LowMedian, HighThreshold=HighMedian, OutputFile=tempFile )#for usage see www.mantidproject.org/
  MaskDetectors(Workspace=inputWS, DetectorList=MDT.getPropertyValue("BadDetectorIDs"))						#for usage see www.mantidproject.org/MaskDetectors
  
  return MDT.getPropertyValue("BadDetectorIDs")

def numberFromCommaSeparated(CommaSeparated) :
  num = CommaSeparated.count(',')
  if num == 0 : num = 0
  else : num = num + 1
  return num
  
def workspaceExists(workS) :
  allNames = mantid.getWorkspaceNames()
  for aName in allNames :
    if aName == workS : return True
  return False
