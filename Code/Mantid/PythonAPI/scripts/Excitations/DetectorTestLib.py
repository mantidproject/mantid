#########################################################
# Applies two tests to the (white beam vanadium) workspace inputWS
# and marks failing spectra bad by writing to the detector list and
# writing zeros to the histograms associated with those detectors
########################################################
from os import remove
from mantidsimple import *
import CommonFunctions as common
# these are the defaults for different instruments
import ExcitDefaults

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
  MaskDetectors(Workspace=inputWS, SpectraList=FDOL.getPropertyValue('BadSpectraNums') )			#for usage see www.mantidproject.org/MaskDetectors
  numBad = numberFromCommaSeparated(FDOL.getPropertyValue("BadSpectraNums"))
  
  MDT = MedianDetectorTest( InputWorkspace=inputWS, OutputWorkspace=outputWS, SignificanceTest=NumErrorBars, LowThreshold=LowMedian, HighThreshold=HighMedian, OutputFile=MedianTempFile )#for usage see www.mantidproject.org/
  MaskDetectors(Workspace=inputWS, SpectraList=MDT.getPropertyValue('BadSpectraNums'))						#for usage see www.mantidproject.org/MaskDetectors
  numBad += numberFromCommaSeparated(MDT.getPropertyValue('BadSpectraNums'))
  
  fileOutputs = ''
  #--get any file output to add to the main output file
  if oFile != '':
    for algor in (FDOL, MDT):
	    #the algorithms can add a path to filenames so we need to query the algorithms to be sure to get the full path
      file = open(algor.getPropertyValue('OutputFile'), 'r')
      fileOutputs = fileOutputs + file.read()
      file.close()
      os.remove(algor.getPropertyValue('OutputFile'))
    outFile=open(oFile, 'a')
    outFile.write(fileOutputs)
    outFile.close()

  badList = common.stringToList(MDT.getPropertyValue('BadSpectraNums'))
  badList += common.stringToList(FDOL.getPropertyValue('BadSpectraNums'))
  return (badList, numBad)
  
def workspaceExists(workS) :
  allNames = mantid.getWorkspaceNames()
  for aName in allNames :
    if aName == workS : return True
  return False

#names of tempory workspaces that will get overwriten
#??STEVES?? put the string 'temp' at the end of this one
SINGLE_WHITE_WS = '_FindBadDetects WBV1'
OUTPUTWS = '_FindBadDetects OUTPUT'
COMP_WHITE_WS = '_FindBadDetects WBV2'

def single_white_van(wbrf, tiny, huge, median_hi, median_lo, error_bars, hardMask=[], oFile='') :
  THISTEST = 'First white beam test'
  if oFile != '':
    outFile=open(oFile, 'a')
    outFile.write('--'+THISTEST+'--\n')
    outFile.close()

  #emtpy list will be filled with numbers of spectra known to be bad before this analysis started, or left empty if that information is not there
  try:
    common.LoadNexRaw(wbrf, SINGLE_WHITE_WS)
    #----------------Calculations Start------------
    if len(hardMask) > 0:
      MaskDetectors(Workspace=SINGLE_WHITE_WS, SpectraList=hardMask)

    #--the integrated workspace will be much smaller so do this as soon as possible
    Integration(SINGLE_WHITE_WS, SINGLE_WHITE_WS)

    (fileOut, numFound) = SingleWBV( SINGLE_WHITE_WS, OUTPUTWS, huge, tiny,\
      median_hi, median_lo, error_bars, oFile )

#--Calculations End---the rest of this script is about outputing the data and dealing with errors and clearing up
  
    #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
    print 'Created the workspaces\n', OUTPUTWS
    print SINGLE_WHITE_WS
    print THISTEST, 'found:', numFound, 'failed detectors'
    
    return fileOut
 
  except Exception, reason:
    print 'Error in'
    print THISTEST
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == SINGLE_WHITE_WS) : mantid.deleteWorkspace(SINGLE_WHITE_WS)
      if (workspace == OUTPUTWS) : mantid.deleteWorkspace(OUTPUTWS)
    raise
 
def second_white_van(wbrf,tiny,huge,median_lo,median_hi,error_bars,oFile='',previous_wb_results=[],previous_wb_ws=SINGLE_WHITE_WS, r=ExcitDefaults.DetectorEfficiencyVariation_Variation) :

   #--Start with settings
  THISTEST = 'Second white beam test'
      
  if oFile != '':
    outFile=open(oFile, 'a')
    outFile.write('--'+THISTEST+'--\n')
    outFile.close()
    DEVFile = oFile+'_dev'
  else :
    DEVFile = ''

  try:
    common.LoadNexRaw(wbrf, COMP_WHITE_WS)

#--------------------------Calculations Start---
    #--the integrated workspace will be much smaller so do this as soon as possible
    Integration(COMP_WHITE_WS, COMP_WHITE_WS)
  
    #--mask the detectors that were found bad in the previous test
    MaskDetectors(Workspace=COMP_WHITE_WS, SpectraList=previous_wb_results)
  
    (sWBVResults, iiUNUSEDii) = SingleWBV(COMP_WHITE_WS, OUTPUTWS, huge, tiny, \
      median_hi, median_lo, error_bars, oFile)
    #--this will overwrite the OUTPUTWS with the cumulative list of the bad
    DEV = DetectorEfficiencyVariation(previous_wb_ws, COMP_WHITE_WS, OUTPUTWS, Variation=r, OutputFile=DEVFile)				#for details see www.mantidproject.org/DetectorEfficiencyVariation

#------------------------Calculations End---the rest of this script is out outputing the data and dealing with errors and clearing up
    if oFile != '' :
      outFile=open(oFile, 'a')
      #--add the file output from the Mantid algorithm that we ran to the output file
      appendMaskFile(DEV.getPropertyValue("OutputFile"), outFile)
      outFile.close()

    DeadList = DEV.getPropertyValue('BadSpectraNums')
    DeadList = common.stringToList(DeadList) +  sWBVResults
    #--How many were found in just this set of tests
    numFound = numberFromCommaSeparated(DeadList)
  	
    #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. Changing any of these values is likely to make it incompatible with the Mantid GUI that ruins this script
    print 'Created the workspaces\n', OUTPUTWS
    print COMP_WHITE_WS
    print THISTEST, 'found:', numFound, 'failed detectors'

    return DeadList

  except Exception, reason:
    print 'Error in'
    print THISTEST
    raise
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == OUTPUTWS) : mantid.deleteWorkspace(OUTPUTWS)
      if (workspace == COMP_WHITE_WS) : mantid.deleteWorkspace(COMP_WHITE_WS)

  finally:
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == '_FindBadDetects ListofBad') : mantid.deleteWorkspace('_FindBadDetects ListofBad')

def bgTest(instru, run_nums, wb_ws, TOFLow, TOFHigh, threshold, rmZeros, error_bars, previous_wb_results=[], wb_ws_comp='', oFile=''):
  THISTEST = 'Background test'
  #setup some workspace names (we need that they don't already exist because they will be overwriten) to make things easier to read later on
  #??STEVES?? add temp to the ends of these names
  SUM = "_FindBadDetects Sum"
  NORMA = OUTPUTWS
  NUMER = '_FindBadDetects Numerator'
  #-- a workspace that we'll reuse for different things, we're reusing to save memory
  TEMPBIG = '_FindBadDetects tempbig'

  if ( oFile != '' ) :
    outFile=open(oFile, 'a')
    outFile.write('--'+THISTEST+'--\n')
    MDTFile = oFile+'_mdt'
  else : MDTFile = ''

  try:#------------Calculations Start---
    # make memory allocations easier by overwriting the workspaces of the same size, although it means that more comments are required here to make the code readable
    instru.loadRun(run_nums[0], TEMPBIG)
    # integrate the counts as soon as possible to reduce the size of the workspace
    Integration(InputWorkspace=TEMPBIG, OutputWorkspace=SUM, RangeLower=TOFLow, RangeUpper=TOFHigh)		#a Mantid algorithm
    if len(run_nums) > 1 :
      for toAdd in run_nums[ 1 : ] :
        # save the memory by overwriting the old workspaces
        instru.loadRun(toAdd, '_FindBadDetects loading')
        Integration('_FindBadDetects loading', '_FindBadDetects loading', TOFLow, TOFHigh)				#a Mantid algorithm
        Plus(SUM, '_FindBadDetects loading', SUM)                                                                                        #for details see www.mantidproject.org/LoadDetectorInfo
      mantid.deleteWorkspace(TEMPBIG)

      #--mask the detectors that were found bad in the previous test
      MaskDetectors(Workspace=SUM, SpectraList=previous_wb_results)
  
    #--prepare to normalise the spectra against the WBV runs
    if wb_ws == '' : raise Exception('The name of a white beam vanadium workspace already into\nloaded into Mantid must be supplied (as wb_ws) for\nnormalisation')
    Integration(InputWorkspace=wb_ws, OutputWorkspace=NORMA)					#a Mantid algorithm
    if ( wb_ws_comp != '' ) :
      #--we have another white beam vanadium we'll combine it with the first white beam
      Integration(InputWorkspace=wb_ws_comp, OutputWorkspace=wb_ws_comp)                #a Mantid algorithm
      #--the equaton is (the harmonic mean) 1/av = (1/Iwbv1 + 1/Iwbv2)/2     av = 2*Iwbv1*Iwbv2/(Iwbv1 + Iwbv2)
      #workspace reuse: NORMA is currently the integral of WBVanadium1
      Multiply(wb_ws_comp, NORMA, NUMER)											#a Mantid algorithm
      Plus(NORMA, wb_ws_comp, NORMA)												#for usage see www.mantidproject.org/Plus
      Divide(NUMER, NORMA, NORMA)												#for usage see www.mantidproject.org/Divide
      #--don't spend time on the factor of two in the harmonic as it will affect all histograms equally and so not affect the results
      mantid.deleteWorkspace(wb_ws_comp)
      mantid.deleteWorkspace(NUMER)
    
    #--we have an integral to normalise against, lets normalise
    Divide(SUM, NORMA, SUM)

    #the default is don't remove low count rates in this background test
    lowThres = -1
    if bool(rmZeros) :
      # the counts are integer numbers of counts that have been normalised and prehaps have a rounding error (is that true?). A very low threshold should reject all zeros but allow anything that was 1 prior to normaliation to get through
      lowThres = 1e-40
  
    #--finally find the detectors! again reusing those workspaces
    MDT = MedianDetectorTest(InputWorkspace=SUM, OutputWorkspace=NORMA, SignificanceTest=error_bars, LowThreshold=lowThres, HighThreshold=threshold, OutputFile=MDTFile )#for usage see www.mantidproject.org/MedianDetectorTest
  
  #----------------Calculations End---the rest of this script is out outputing the data and dealing with errors and clearing up
    if oFile != '' :
      #--pick up the file output from one detector test that we did
      appendMaskFile(MDT.getPropertyValue("OutputFile"), outFile)
      outFile.close()

    #--How many were found in just this set of tests
    DeadList = common.stringToList(MDT.getPropertyValue('BadSpectraNums'))

    #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
    print 'Created the workspaces\n', NORMA
    print
    print THISTEST, 'found:', len(DeadList), 'failed detectors'

    return DeadList
    
  except Exception, reason:
    print 'Error'
    print THISTEST	
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == NORMA) : mantid.deleteWorkspace(NORMA)
    raise
 
  # the C++ that called this needs to look at the output from the print statements and deal with the fact that there was a problem
  finally:
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == TEMPBIG) : mantid.deleteWorkspace(TEMPBIG)
      if (workspace == NUMER) : mantid.deleteWorkspace(NUMER)
      if (workspace == SUM) : mantid.deleteWorkspace(SUM)
      if (workspace == "_FindBadDetects loading") : mantid.deleteWorkspace("_FindBadDetects loading")

def diagnose(instrum='',maskFile='',wbrf='',wbrf2='',runs='',tiny=1e-10,huge=1e10,median_lo=0.1,median_hi=3.0,sv_sig=3.3,bmedians=5.0,bmin=-1e8,bmax=-1e8,zero='False',out_asc='') :
  
  #convert the run numbers, if one was passed into a run name
  instru = ExcitDefaults.loadDefaults(instrum)

  try :
    #--load input workspace and files and get the names of the output workspace and files
    mask = []
    if maskFile != '':
      mask = common.loadMask(maskFile)
      outputWSName = OUT_WS_PREFIX+maskFile
    #---the white beam vanadium file takes precendence over the mask file for naming
    if wbrf != '':
      #--Import the file and give it an obsure workspace name because we require that a workspace with that name doesn't exist and we'll remove it at the end
      outputWSName = OUT_WS_PREFIX+'1WBV_'+common.getRunName(wbrf)
    #--the outfile name has the highest precendence
    if out_asc != '':
      outputWSName = OUT_WS_PREFIX+'1WBV_'+common.getRunName(out_asc)
      outFile=open(out_asc, 'w')
      if len(mask) > 0:
        outFile.write('--Hard Mask File List--\n')
        outFile.write(mask)
        outFile.write('\n')
      outFile.close()

    found = []
    if wbrf != '':
      wbrf = instru.getFileName(wbrf)
      found = single_white_van(wbrf, tiny=tiny, huge=huge, median_lo=median_lo, median_hi=median_hi, error_bars=sv_sig, hardMask=mask, oFile=out_asc)

      comp_white_beam = ''
      if wbrf2 != '':
	wbrf2 = instru.getFileName(wbrf2)
	found += second_white_van(wbrf2, tiny=tiny, huge=huge, median_lo=median_lo, median_hi=median_hi, error_bars=sv_sig, oFile=out_asc,\
	  previous_wb_results=common.listToString(found), previous_wb_ws=SINGLE_WHITE_WS, r=ExcitDefaults.DetectorEfficiencyVariation_Variation)
	comp_white_beam = COMP_WHITE_WS
      if runs != '':
        if bmin < -9.9e7:
	  #this means that (the start of the background range) wasn't specified, use the default
          bmin = instru.backgroundRange[0]
        if bmax < -9.9e7:
          bmax = instru.backgroundRange[1]

        run_nums = common.listToString(runs).split(',')
	found += bgTest(instru, run_nums, wb_ws=SINGLE_WHITE_WS, TOFLow=bmin, TOFHigh=bmax, threshold=bmedians, rmZeros=zero, error_bars=sv_sig,
	  previous_wb_results=common.listToString(found), wb_ws_comp=comp_white_beam,  oFile=out_asc)

      RenameWorkspace(OUTPUTWS, outputWSName)
      
      return found

  except Exception, reason:
    print 'Exception ', reason, ' caught'
    raise
 
#below is a quick test/example command to run this library, it must be commented or the library won't work!
#badSpectra = diagnose(instrum='MAR',wbrf='11060',wbrf2='11060',runs='15537',tiny=1e-10,huge=1e10,median_lo=0.1,median_hi=3.0,sv_sig=3.3,bmedians=5.0,zero='False', out_asc='C:/Users/wht13119/Desktop/docs/Excitations/test/11060.msk',maskFile='')
#print badSpectra