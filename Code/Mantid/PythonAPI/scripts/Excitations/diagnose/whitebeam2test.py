#########################################################
# Loads a second white beam vanadium, applies the criteria for bad
# detectors to that spectrium and to a comparison between them.
# Failures from the first test are writen to the second white beams
# detector list and histograms are zero but not failure from the comparison
########################################################
import BadDetectorTestFunctions
#--Start with Settings but these are not ones that are interesting to change
THISTEST = 'Second white beam test'
#setup some workspace names (we need that they don't already exist because they will be overwriten) to make things easier to read later on
OUTPUTWS = '_FindBadDetects whitebeam2out'
#--These are the values passed over from the MantidPlot dialog via C++, probably don't change them 
WBV1WS = '|WBV1|'
DOWNSOFAR = '|INPUTMASK|'
WBV2FILE = '|WBVANADIUM2|'
HIGHABSOLUTE = '|HIGHABSOLUTE|'
LOWABSOLUTE = '|LOWABSOLUTE|'
HIGHMEDIAN = '|HIGHMEDIAN|'
LOWMEDIAN = '|LOWMEDIAN|'
NUMBERRORBARS = '|SIGNIFICANCETEST|'
VARIATION = '|CHANGEBETWEEN|'
OMASKFILE = '|OUTPUTFILE|'
  
try:
  WBV2WS = '_FindBadDetects WBV2'
  LoadRaw(WBV2FILE, WBV2WS)
  
  #--pickup bad detectors from earlier tests, good detectors were set to have a value of 0 and bad 100 so 10 works as a cut off
  prevTest = FindDetectorsOutsideLimits(InputWorkspace=DOWNSOFAR, OutputWorkspace=DOWNSOFAR, HighThreshold=10, LowThreshold=-1 )
  downArray = prevTest.getPropertyValue('BadDetectorIDs')
  #--write to the workspace
  MaskDetectors(Workspace=WBV2WS, DetectorList=downArray)
  
  functions.SingleWBV( WBV2WS, OUTPUTWS, HIGHABSOLUTE, LOWABSOLUTE, \
    HIGHMEDIAN, LOWMEDIAN, NUMBERRORBARS, OMASKFILE )
#  #--this will overwrite the OUTPUTWS with the cumulative list of the bad
  DEV = DetectorEfficiencyVariation( WhiteBeamBase=WBV1WS, WhiteBeamCompare=WBV2WS, \
    OutputWorkspace=OUTPUTWS, Variation=VARIATION, OutputFile=OMASKFILE )							#for usage see www.mantidproject.org/DetectorEfficiencyVariation

  DeadList = DEV.getPropertyValue('BadDetectorIDs')

  #--How many were found in just these tests
  numFound = functions.numberFromCommaSeparated(DeadList) \
    - functions.numberFromCommaSeparated(downArray)
  
  #--make a list of only the spectra that were marked bad this time
  Minus(OUTPUTWS, DOWNSOFAR, OUTPUTWS)																#for usage see www.mantidproject.org/Minus
  
  #-- this output is passed back to MantidPlot, it must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
  print 'success'
  print THISTEST
  print 'White beam vanadium comparison complete'
  print OUTPUTWS
  print numFound
  print WBV2WS
	
except Exception, reason:
  print 'Error'
  print 'Exception ', reason, ' caught, aborting'
  print 'Problem with white beam 2 and comparing'
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == '_FindBadDetects WBV2') : mantid.deleteWorkspace('_FindBadDetects WBV2')
finally:
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == '_FindBadDetects ListofBad') : mantid.deleteWorkspace('_FindBadDetects ListofBad')
