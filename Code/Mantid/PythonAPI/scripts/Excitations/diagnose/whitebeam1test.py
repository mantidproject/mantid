#########################################################
# Does all the tests that can be done on a single white beam vanadium.
# Failing spectra are marked bad by writing to the detector list
# and writing zeros to the histograms associated with those detectors
########################################################
from mantidsimple import *
import CommonFunctions as common
import DetectorTestLib as detLib

#--Start with settings
THISTEST = 'First white beam test'
#--These varibles in | | will be overwriten by MantidPlot with the values entered into the dialog box, so probably don't change them here
WBV1FILE = |WBVANADIUM1|
HIGHABSOLUTE = |HIGHABSOLUTE|
LOWABSOLUTE = |LOWABSOLUTE|
HIGHMEDIAN = |HIGHMEDIAN|
LOWMEDIAN = |LOWMEDIAN|
NUMBERRORBARS = |SIGNIFICANCETEST|
OMASKFILE = |OUTPUTFILE|
IMASKFILE = |INPUTFILE|

hardMask = []
try:
  #--load input workspace and files and get the names of the output workspace and files
  if IMASKFILE != '':
    hardMask = common.loadMask(IMASKFILE)
    OUTPUTWS = detLib.OUT_WS_PREFIX+IMASKFILE
  #---the white beam vanadium file takes precendence over the mask file for naming
  if WBV1FILE != '':
    #--Import the file and give it an obsure workspace name because we require that a workspace with that name doesn't exist and we'll remove it at the end
    common.LoadNexRaw(WBV1FILE, '_FindBadDetects WBV1')
    OUTPUTWS = detLib.OUT_WS_PREFIX+'1WBV_'+common.getRunName(WBV1FILE)
  #--the outfile name has the highest precendence
  if OMASKFILE != '':
    OUTPUTWS = detLib.OUT_WS_PREFIX+'1WBV_'+common.getRunName(OMASKFILE)
    outfile=open(OMASKFILE, 'w')
    if len(hardMask) > 0:
      outfile.write('--Hard Mask File List--\n')
      outfile.write(hardMask)
      outfile.write('\n')
    outfile.write('--'+THISTEST+'--\n')

 #----------------Calculations Start------------
  if len(hardMask) > 0:
    MaskDetectors(Workspace='_FindBadDetects WBV1', SpectraList=hardMask)

  #--the integrated workspace will be much smaller so do this as soon as possible
  Integration('_FindBadDetects WBV1', '_FindBadDetects WBV1')
    
  (fileOut, numFound) = detLib.SingleWBV( '_FindBadDetects WBV1', OUTPUTWS, \
    HIGHABSOLUTE, LOWABSOLUTE, HIGHMEDIAN, LOWMEDIAN, NUMBERRORBARS, OMASKFILE )

#--Calculations End---the rest of this script is about outputing the data and dealing with errors and clearing up
 

  #--DeadList is a string of comma separated integers, this gets the number of integers
#  numFound = detLib.numberFromCommaSeparated(DeadList)
  
  #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
  print 'success'
  print THISTEST
  print 'White beam vanadium 1 complete'
  print OUTPUTWS
  print numFound
  print '_FindBadDetects WBV1'
 
# Make sure that our tempory workspaces aren't left hanging around
except Exception, reason:
  print 'Error'
  print 'Exception ', reason, ' caught'
  print THISTEST
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == "_FindBadDetects WBV1") : mantid.deleteWorkspace("_FindBadDetects WBV1")
  # the C++ that called this needs to look at the output from the print statements and deal with the fact that there was a problem

 
