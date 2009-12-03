#########################################################
# Does all the tests that can be done on a single white beam vanadium.
# Failing spectra are marked bad by writing to the detector list
# and writing zeros to the histograms associated with those detectors
########################################################
from os import remove
from mantidsimple import *
import CommonFunctions as common
import BadDetectorTestFunctions as detectorTest

#--Start with settings
THISTEST = 'First white beam test'
OUTPUTWS = '_FindBadDetects whitebeam1out'
#--These varibles in | | will be overwriten by MantidPlot with the values entered into the dialog box, so probably don't change them here
WBV1FILE = '|WBVANADIUM1|'
HIGHABSOLUTE = '|HIGHABSOLUTE|'
LOWABSOLUTE = '|LOWABSOLUTE|'
HIGHMEDIAN = '|HIGHMEDIAN|'
LOWMEDIAN = '|LOWMEDIAN|'
NUMBERRORBARS = '|SIGNIFICANCETEST|'
OMASKFILE = '|OUTPUTFILE|'
IMASKFILE = '|INPUTFILE|'

if ( OMASKFILE != '' ) :
    outfile=open(OMASKFILE, 'w')
    outfile.write('--'+THISTEST+'--\n')
	
try:#--Calculations Start---
  #--Import the file and give it an obsure workspace name because we require that a workspace with that name doesn't exist and we'll remove it at the end
  LoadRaw(WBV1FILE, '_FindBadDetects WBV1')
  #--the integrated workspace will be much smaller so do this as soon as possible
  Integration('_FindBadDetects WBV1', '_FindBadDetects WBV1')
  
  if ( IMASKFILE != '' ) :
    hardMask = common.loadMask(IMASKFILE)
    MaskDetectors(Workspace='_FindBadDetects WBV1', SpectraList=hardMask)

  DeadList = detectorTest.SingleWBV( '_FindBadDetects WBV1', OUTPUTWS, \
    HIGHABSOLUTE, LOWABSOLUTE, HIGHMEDIAN, LOWMEDIAN, NUMBERRORBARS, OMASKFILE )

  #--Calculations End---the rest of this script is out outputing the data and dealing with errors and clearing up
 

  #--DeadList is a string of comma separated integers, this gets the number of integers
  numFound = detectorTest.numberFromCommaSeparated(DeadList)
  
  #--SingleWBV writes one file for each of its two tests, merge these into our main output file
  if OMASKFILE != "" :
    detectorTest.appendMaskFile(OMASKFILE+'_swbv_fdol', outfile)
    detectorTest.appendMaskFile(OMASKFILE+'_swbv_mdt', outfile)
    outfile.close()

  #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
  print 'success'
  print THISTEST
  print 'White beam vanadium 1 complete'
  print OUTPUTWS
  print numFound
  print '_FindBadDetects WBV1'
 
#--Make sure that our tempory workspaces aren't left hanging around
except Exception, reason:
  print 'Error'
  print 'Exception ', reason, ' caught'
  print THISTEST	
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == "_FindBadDetects WBV1") : mantid.deleteWorkspace("_FindBadDetects WBV1")
#finally:
#
#  for workspace in mantid.getWorkspaceNames() :
#    if (workspace == '_FindBadDetects Normaliser') : mantid.deleteWorkspace('_FindBadDetects Normaliser')
 
