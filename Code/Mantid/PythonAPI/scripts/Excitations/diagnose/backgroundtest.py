from mantidsimple import *
import BadDetectorTestFunctions as functions

#--these settings are read in from the MantidPlot GUI so probably don't change these first lines
THISTEST = 'Background test'
#setup some workspace names (we need that they don't already exist because they will be overwriten) to make things easier to read later on
SUM = "_FindBadDetects Sum"
NORMA = '_FindBadDetects Normaliser'                          				#this is only makes things easier to read later on, there is no magic
NUMER = '_FindBadDetects Numerator'
#-- a workspace that we'll reuse for different things, we're reusing to save memory
TEMPBIG = '_FindBadDetects tempbig'
#--These are the values passed over from the MantidPlot dialog via C++, probably don't change them
DOWNSOFAR1 = '|MASK1|'
DOWNSOFAR2 = '|MASK2|'
DATAFILENAMES = '|EXPFILES|'
ACCEPT = '|BACKGROUNDACCEPT|'
NUMERRORBARS = '|ERRORBARS|'
REMOVEZERO = '|REMOVEZEROS|'
WBV1WS = '|WBV1|'
WBV2WS = '|WBV2|'
OMASKFILE = '|OUTPUTFILE|'

if ( OMASKFILE != '' ) :
  outfile=open(OMASKFILE, 'a')
  outfile.write('--'+THISTEST+'--\n')
  MDTFile = OMASKFILE+'_mdt'
else : MDTFile = ''

try:#--Calculations Start---
  #--load in the experimental run data  
  dataFiles = DATAFILENAMES.split(',')
  # make memory allocations easier by overwriting the workspaces of the same size, although it means that more comments are required here to make the code readable
  LoadRaw(dataFiles[0], TEMPBIG)											#for usage see www.mantidproject.org/LoadRaw
  # integrate the counts as soon as possible to reduce the size of the workspace
  Integration(InputWorkspace=TEMPBIG, OutputWorkspace=SUM |TOFWINDOWBLOCK|)                       #a Mantid algorithm
  if len(dataFiles) > 1 :
    for toAdd in dataFiles[ 1 : ] :
      # save the memory by overwriting the old workspaces
      LoadRaw(toAdd, "_FindBadDetects loading")
      Integration(InputWorkspace="_FindBadDetects loading", OutputWorkspace="_FindBadDetects loading" |TOFWINDOWBLOCK|)# |TOFWINDOWBLOCK|)#a Mantid algorithm
      Plus(SUM, "_FindBadDetects loading", SUM)
    mantid.deleteWorkspace(TEMPBIG)
	  
  #--pickup bad detectors from earlier tests, reusing a workspace again rather than spending more memory
  downSoFar = "_FindBadDetects loading"
  if ( DOWNSOFAR2 != "" ) : Plus(DOWNSOFAR1, DOWNSOFAR2, downSoFar)
  else : downSoFar = DOWNSOFAR1
  # good detectors were set to have a value of 0 and bad 100 so 10 works as a cut off
  prevTest = FindDetectorsOutsideLimits(InputWorkspace=downSoFar, OutputWorkspace=downSoFar, HighThreshold=10, LowThreshold=-1, OutputFile='STEVESCHANGETHIS' )
  downArray = prevTest.getPropertyValue('BadDetectorIDs')
  MaskDetectors(Workspace=SUM, DetectorList=downArray)
  
  #--prepare to normalise the spectra against the WBV runs
  Integration(InputWorkspace=WBV1WS, OutputWorkspace=NORMA)					#a Mantid algorithm
  if ( WBV2WS != '' ) :
    #--we have another white beam vanadium we'll combine it with the first white beam
    Integration(InputWorkspace=WBV2WS, OutputWorkspace=WBV2WS)                #a Mantid algorithm
    #--the equaton is (the harmonic mean) 1/av = (1/Iwbv1 + 1/Iwbv2)/2     av = 2*Iwbv1*Iwbv2/(Iwbv1 + Iwbv2)
    #workspace reuse: NORMA is currently the integral of WBVanadium1
    Multiply(WBV2WS, NORMA, NUMER)											#a Mantid algorithm
    Plus(NORMA, WBV2WS, NORMA)												#for usage see www.mantidproject.org/Plus
    Divide(NUMER, NORMA, NORMA)												#for usage see www.mantidproject.org/Divide
    #--don't spend time on the factor of two as it will affect all histograms equally and so not affect the results
    mantid.deleteWorkspace(WBV2WS)
    mantid.deleteWorkspace(NUMER)
    
  #--we have an integral to normalise against, lets normalise
  Divide(SUM, NORMA, SUM)

  #the default is don't remove low count rates in this background test
  lowThres = -1
  if REMOVEZERO == 'true' :
  # the counts are integer numbers of counts that have been normalised and prehaps have a rounding error (is that true?). A very low threshold should reject all zeros but allow anything that was 1 prior to normaliation to get through
    lowThres = 1e-40
  
  #--finally find the detectors! again reusing those workspaces
  MDT = MedianDetectorTest( InputWorkspace=SUM, OutputWorkspace=NORMA, SignificanceTest=NUMERRORBARS, LowThreshold=lowThres, HighThreshold=ACCEPT, OutputFile=MDTFile )#for usage see www.mantidproject.org/MedianDetectorTest
  
  #--Calculations End---the rest of this script is out outputing the data and dealing with errors and clearing up
  #--How many were found in just these tests
  numFound = functions.numberFromCommaSeparated(MDT.getPropertyValue('BadDetectorIDs')) \
  #  - functions.numberFromCommaSeparated(downArray)
  
  if OMASKFILE != "" :
    #--pick up the file output from one detector test that we did
	functions.appendMaskFile(MDTFile, outfile)
	outfile.close()

  #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
  print 'success'
  print THISTEST
  print 'Tests on low flux background complete'
  print NORMA
  print numFound
  print 'not applicable'
  
except Exception, reason:
  print 'Error'
  print 'Exception ', reason, ' caught'
  print THISTEST	
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == NORMA) : mantid.deleteWorkspace(NORMA)
  # the C++ that called this needs to look at the output from the print statements and deal with the fact that there was a problem
finally:
  for workspace in mantid.getWorkspaceNames() :
    if (workspace == TEMPBIG) : mantid.deleteWorkspace(TEMPBIG)
    if (workspace == NUMER) : mantid.deleteWorkspace(NUMER)
    if (workspace == SUM) : mantid.deleteWorkspace(SUM)
    if (workspace == WBV1WS) : mantid.deleteWorkspace(WBV1WS)
    if (workspace == WBV2WS) : mantid.deleteWorkspace(WBV2WS)
    if (workspace == "_FindBadDetects loading") : mantid.deleteWorkspace("_FindBadDetects loading")

#  if OMASKFILE != "" :
# check if it exists and os.remove(MDTFile)	
