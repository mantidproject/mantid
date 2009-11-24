#########################################################
# Loads a second white beam vanadium, applies the criteria for bad
# detectors to that spectrium and to a comparison between them.
# Failures from the first test are writen to the second white beams
# detector list and histograms are zero but not failure from the comparison
########################################################
from os import remove
import BadDetectorTestFunctions
#--Start with settings
THISTEST = 'Second white beam test'
#setup some workspace names (we need that they don't already exist because they will be overwriten) to make things easier to read later on
OUTPUTWS = '_FindBadDetects whitebeam2out'
WBV2WS = '_FindBadDetects WBV2'
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

if ( OMASKFILE != '' ) :
    outfile=open(OMASKFILE, 'a')
    outfile.write('--'+THISTEST+'--\n')
    DEVFile = OMASKFILE+'_dev'
else : DEVFile = ''

	
try:#--Calculations Start---
  LoadRaw(WBV2FILE, WBV2WS)
  #--the integrated workspace will be much smaller so do this as soon as possible
  Integration(WBV2WS, WBV2WS)
  

  #--pickup bad detectors from earlier tests, good detectors were set to have a value of 0 and bad 100 so 10 works as a cut off
  prevTest = FindDetectorsOutsideLimits(InputWorkspace=DOWNSOFAR, OutputWorkspace=DOWNSOFAR, HighThreshold=10, LowThreshold=-1, OutputFile='STEVESCHANGETHIS' )
  downArray = prevTest.getPropertyValue('BadDetectorIDs')
  #--write to the workspace
  MaskDetectors(Workspace=WBV2WS, DetectorList=downArray)
  
  functions.SingleWBV( WBV2WS, OUTPUTWS, HIGHABSOLUTE, LOWABSOLUTE, \
    HIGHMEDIAN, LOWMEDIAN, NUMBERRORBARS, OMASKFILE )
  #--this will overwrite the OUTPUTWS with the cumulative list of the bad
  DEV = DetectorEfficiencyVariation( WhiteBeamBase=WBV1WS, WhiteBeamCompare=WBV2WS, \
    OutputWorkspace=OUTPUTWS, Variation=VARIATION, OutputFile=DEVFile )							#for usage see www.mantidproject.org/DetectorEfficiencyVariation

  #--Calculations End---the rest of this script is out outputing the data and dealing with errors and clearing up
  DeadList = DEV.getPropertyValue('BadDetectorIDs')
  #--How many were found in just these tests
  numFound = functions.numberFromCommaSeparated(DeadList) \
   # - functions.numberFromCommaSeparated(downArray)
  
  #--make a list of only the spectra that were marked bad this time
  # Minus(OUTPUTWS, DOWNSOFAR, OUTPUTWS)																#for usage see www.mantidproject.org/Minus
  
  if OMASKFILE != "" :
    #--SingleWBV writes one file for each of its two tests, merge these in too
    functions.appendMaskFile(OMASKFILE+'_swbv_fdol', outfile)
    functions.appendMaskFile(OMASKFILE+'_swbv_mdt', outfile)
    #--pick up the file output from the Mantid algorithm that we ran
    functions.appendMaskFile(DEVFile, outfile)
    outfile.close()
	
  #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
  print 'success'
  print THISTEST
  print 'White beam vanadium comparison complete'
  print OUTPUTWS
  print numFound
  print WBV2WS
  
except Exception, reason:
  print 'Error'
  print 'Exception ', reason, ' caught'
  print THISTEST	
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == '_FindBadDetects WBV2') : mantid.deleteWorkspace('_FindBadDetects WBV2')
finally:
#  if OMASKFILE != "" :
  # check if it exists and os.remove(DEVFile)	
  # check if it exists and os.remove(OMASKFILE+'.swbv_mdt')	
  # check if it exists and os.remove(OMASKFILE+'.swbv_fdol')
	
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == '_FindBadDetects ListofBad') : mantid.deleteWorkspace('_FindBadDetects ListofBad')
