#########################################################
# Does all the tests that can be done on a single white beam vanadium.
# Failing spectra are marked bad by writing to the detector list
# and writing zeros to the histograms associated with those detectors
########################################################
import BadDetectorTestFunctions as functions
#--Start with Settings but these are not ones that are interesting to change
THISTEST = 'First white beam test'
try:
  #--Start with Settings but these are not ones that are interesting to change
  OUTPUTWS = '_FindBadDetects whitebeam1out'
  #--These varibles in | | will be overwriten by MantidPlot with the values entered into the dialog box
  WBV1FILE = '|WBVANADIUM1|'
  HIGHABSOLUTE = '|HIGHABSOLUTE|'
  LOWABSOLUTE = '|LOWABSOLUTE|'
  HIGHMEDIAN = '|HIGHMEDIAN|'
  LOWMEDIAN = '|LOWMEDIAN|'
  NUMBERRORBARS = '|SIGNIFICANCETEST|'
  OMASKFILE = '|OUTPUTFILE|'
  IMASKFILE = '|INPUTFILE|'
  
  #--Import the file and give it an obsure workspace name because we require that a workspace with that name doesn't exist and we'll remove it at the end
  LoadRaw(WBV1FILE, '_FindBadDetects WBV1')
  
  if ( IMASKFILE != "" ) :
	IMASKFILE = ""   # NOT IMPLEMENTED load the spectra numbers that have already been masked as the sequencies of numbers and ranges

  DeadList = functions.SingleWBV( '_FindBadDetects WBV1', OUTPUTWS, \
    HIGHABSOLUTE, LOWABSOLUTE, HIGHMEDIAN, LOWMEDIAN, NUMBERRORBARS, OMASKFILE )

  #--DeadList is a string of comma separated integers, this gets the number of integers
  numFound = functions.numberFromCommaSeparated(DeadList)
  
  print 'success'
  print THISTEST
  print 'White beam vanadium 1 complete'
  print OUTPUTWS
  print numFound
  print '_FindBadDetects WBV1'
 
#--Make sure that our tempory workspaces aren't left hanging around
except Exception, reason:
  print 'Error'
  print 'Exception ', reason, ' caught, aborting'
  print 'Problem analysising first white beam vanadium'
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == "_FindBadDetects WBV1") : mantid.deleteWorkspace("_FindBadDetects WBV1")
#finally:
#  for workspace in mantid.getWorkspaceNames() :
#    if (workspace == '_FindBadDetects Normaliser') : mantid.deleteWorkspace('_FindBadDetects Normaliser')
 