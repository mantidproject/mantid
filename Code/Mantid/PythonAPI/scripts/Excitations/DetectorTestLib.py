#########################################################
# Applies two tests to the (white beam vanadium) workspace inputWS
# and marks failing spectra bad by writing to the detector list and
# writing zeros to the histograms associated with those detectors
########################################################
from os import remove

from mantidsimple import *
import CommonFunctions as common

OUT_WS_PREFIX = 'mask_'
DET_EFF_VAR = 1.1

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
  if num == 0 : return 0
  else : return num + 1
  
def SingleWBV( inputWS, outputWS, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, oFile ) :
  if oFile == '' :
    limitsTempFile = ''
    MedianTempFile = ''
  else :
    limitsTempFile = '_DetectorTest_swbv_FDOL_tempfile'
    MedianTempFile = '_DetectorTest_swbv_MDT_tempfile'
  
  FDOL = FindDetectorsOutsideLimits( inputWS, outputWS, HighAbsolute, LowAbsolute, OutputFile=limitsTempFile )
  MaskDetectors(Workspace=inputWS, SpectraList=FDOL.getPropertyValue('BadSpectraNums') )
  
  MDT = MedianDetectorTest( InputWorkspace=inputWS, OutputWorkspace=outputWS, SignificanceTest=NumErrorBars, LowThreshold=LowMedian, HighThreshold=HighMedian, OutputFile=MedianTempFile )
  MaskDetectors(Workspace=inputWS, SpectraList=MDT.getPropertyValue('BadSpectraNums'))
  
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

  badList = MDT.getPropertyValue('BadSpectraNums') + ',' + FDOL.getPropertyValue('BadSpectraNums')

  return (badList, len(badList))
  
#names of tempory workspaces that will get overwriten
SINGLE_WHITE_WS = '_FindBadDets_WBV1_tempory'
OUTPUTWS = '_FindBadDets_tempory'
COMP_WHITE_WS = '_FindBadDets_WBV2_tempory'

#-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
def printSummary(outputWS, inputWS, testName, numFound):
  print 'Created the workspaces:'
  print outputWS
  print inputWS
  print testName, 'failed this number of detectors:'
  print numFound
  
def printProblem(reason, testName) :
    print 'Error:'
    print reason
    print 'found in ' + testName

def single_white_van(wbrf, tiny, huge, median_hi, median_lo, error_bars, prevList='', oFile='', outWS=OUTPUTWS) :
  THISTEST = 'First white beam test'
  if oFile != '':
    outFile=open(oFile, 'a')
    outFile.write('--'+THISTEST+'--\n')
    outFile.close()

  #emtpy list will be filled with numbers of spectra known to be bad before this analysis started, or left empty if that information is not there
  try:
    data_ws = common.load_run(wbrf, 'white-beam')
    #----------------Calculations Start------------
    #--the integrated workspace will be much smaller so do this as soon as possible
    Integration(data_ws, SINGLE_WHITE_WS)                                                                 
    (fileOut, numFound) = SingleWBV( SINGLE_WHITE_WS, outWS, huge, tiny,
                                     median_hi, median_lo, error_bars, oFile )

    if len(prevList) > 0:
      MD = MaskDetectors(Workspace=SINGLE_WHITE_WS, SpectraList=prevList)
      #the algorithm expands out any ranges specified with '-'
      prevList = MD.getPropertyValue('SpectraList')

#--Calculations End---the rest of this script is about outputing the data and dealing with errors and clearing up

    #use the set class to remove duplicates
    sWBWDead = set(common.stringToList(fileOut))
    #--How many were found in just this set of tests
    prev = set(common.stringToList(prevList))
    dead = sWBWDead | prev
    found = sWBWDead - prev
    numFound = len(found)
    
    #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting. It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
    printSummary(outWS, SINGLE_WHITE_WS, THISTEST, numFound)
        
    return common.listToString(list(dead))
 
  except Exception, reason:
    printProblem(reason, THISTEST)
    
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == SINGLE_WHITE_WS) : mantid.deleteWorkspace(SINGLE_WHITE_WS)
      if (workspace == outWS) : mantid.deleteWorkspace(outWS)
    
    raise Exception('A discription of the error has been printed')
 
def second_white_van(wbrf,tiny,huge,median_lo,median_hi,error_bars,previous_wb_ws,prevList=[], oFile='',r=DET_EFF_VAR, outWS=OUTPUTWS) :

   #--Start with settings
  THISTEST = 'Second white beam test'
      
  if oFile != '':
    outFile=open(oFile, 'a')
    outFile.write('--'+THISTEST+'--\n')
    outFile.close()
    DEVFile = oFile+'_dev'
  else :
    DEVFile = ''

  # Python 2.4 does not support try...except...finally blocks so resort to nested try...except
  try:
    try:
      data_ws = common.load_run(wbrf)

#--------------------------Calculations Start---
      #--the integrated workspace will be much smaller so do this as soon as possible
      Integration(data_ws, COMP_WHITE_WS)
  
      if len(prevList) > 0 :                                                                                                                                                                        
        MD = MaskDetectors(Workspace=COMP_WHITE_WS, SpectraList=prevList)
        prevList = MD.getPropertyValue('SpectraList')                                                                                                            

      (sWBVResults, iiUNUSEDii) = SingleWBV(COMP_WHITE_WS, outWS, huge, tiny,
                                            median_hi, median_lo, error_bars, oFile)
      #--this will overwrite the outWS with the cumulative list of bad detectors
      DEV = DetectorEfficiencyVariation(previous_wb_ws, COMP_WHITE_WS, outWS, Variation=r, OutputFile=DEVFile)

#------------------------Calculations End---the rest of this script is out outputing the data and dealing with errors and clearing up
      if oFile != '' :
        outFile=open(oFile, 'a')
        #--add the file output from the Mantid algorithm that we ran to the output file
        appendMaskFile(DEV.getPropertyValue("OutputFile"), outFile)
        outFile.close()

      # remove any duplicates using the set class
      DevDead = DEV.getPropertyValue('BadSpectraNums')
      DevDead = set(common.stringToList(DevDead))
      SWBWDead = set(common.stringToList(sWBVResults))
      prev = set(common.stringToList(prevList))
      DeadList = DevDead | SWBWDead | prev
      #--How many were found in just this set of tests
      found = DeadList - prev
      numFound = len(found)

      #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting.
      # It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
      printSummary(outWS, COMP_WHITE_WS, THISTEST, numFound)

      return common.listToString(list(DeadList))

    except Exception, reason:
      printProblem(reason, THISTEST)

      for workspace in mantid.getWorkspaceNames() :
        if (workspace == outWS) : mantid.deleteWorkspace(outWS)
        if (workspace == COMP_WHITE_WS) : mantid.deleteWorkspace(COMP_WHITE_WS)
        
      raise Exception('A discription of the error has been printed')
  finally:
    for workspace in mantid.getWorkspaceNames():
      if (workspace == '_FindBadDetects ListofBad'): mantid.deleteWorkspace('_FindBadDetects ListofBad')

def bgTest(inst_prefix, run_nums, wb_ws, TOFLow, TOFHigh, threshold, rmZeros, error_bars, prevList=[], wb_ws_comp='', oFile='', outWS = OUTPUTWS):
  THISTEST = 'Background test'

  #setup some workspace names (we need that they don't already exist because they will be overwriten) to make things easier to read later on
  SUM = "_FindBadDets_Sum_tempory"
  NORMA = outWS
  NUMER = '_FindBadDets_Numerator_tempory'
  #-- a workspace that we'll reuse for different things, we're reusing to save memory
  TEMPBIG = '_FindBadDets_big_tempory'

  if ( oFile != '' ) :
    outFile=open(oFile, 'a')
    outFile.write('--'+THISTEST+'--\n')
    MDTFile = oFile+'_mdt'
  else : MDTFile = ''

  # Python 2.4 does not support try...except...finally blocks so resort to nested try...except
  try:
    try:
      # make memory allocations easier by overwriting the workspaces of the same size, although it means that more comments are required here to make the code readable
      data_ws = common.load_run(run_nums[0], 'mono-sample')
      instrument = data_ws.getInstrument()
      if TOFLow < -9.9e7:
        #this means that (the start of the background range) wasn't specified, use the default
        bmin = float(instrument.getNumberParameter('bkgd-range-min')[0])
      if TOFHigh < -9.9e7:
        bmax = float(instrument.getNumberParameter('bkgd-range-max')[0])

      # integrate the counts as soon as possible to reduce the size of the workspace
      Integration(InputWorkspace=data_ws, OutputWorkspace=SUM, RangeLower=TOFLow, RangeUpper=TOFHigh)
      if len(run_nums) > 1 :
        for toAdd in run_nums[ 1 : ] :
            # save the memory by overwriting the old workspaces
            temp = common.load_run(toAdd)
            Integration(temp, temp, TOFLow, TOFHigh)
            Plus(SUM, temp, SUM)
            mantid.deleteWorkspace(temp)

      if len(prevList) > 0 :
        MD = MaskDetectors(Workspace=SUM, SpectraList=prevList)
        prevList = MD.getPropertyValue('SpectraList')
  
      #--prepare to normalise the spectra against the WBV runs
      if wb_ws == '' : raise Exception('The name of a white beam vanadium workspace already into\nloaded into Mantid must be supplied (as wb_ws) for\nnormalisation')
      Integration(InputWorkspace=wb_ws, OutputWorkspace=NORMA)
      if ( wb_ws_comp != '' ) :
        #--we have another white beam vanadium we'll combine it with the first white beam
        Integration(InputWorkspace=wb_ws_comp, OutputWorkspace=wb_ws_comp)
        #--the equaton is (the harmonic mean) 1/av = (1/Iwbv1 + 1/Iwbv2)/2       av = 2*Iwbv1*Iwbv2/(Iwbv1 + Iwbv2)
        #workspace reuse: NORMA is currently the integral of WBVanadium1
        Multiply(wb_ws_comp, NORMA, NUMER)
        Plus(NORMA, wb_ws_comp, NORMA)
        Divide(NUMER, NORMA, NORMA)
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
      DeadList = MDT.getPropertyValue('BadSpectraNums')

      # remove any duplicates using the set class
      DeadList = set(common.stringToList(DeadList))
      prev = set(common.stringToList(prevList))
      DeadList = DeadList | prev
      #--How many were found in just this set of tests
      found = DeadList - prev
      numFound = len(found)
      
      #-- this output is passed back to the calling MantidPlot application and must be executed last so not to interfer with any error reporting.
      #It must start with success (for no error), the next lines are the workspace name and number of detectors found bad this time
      printSummary(NORMA, '', THISTEST, numFound)

      return common.listToString(list(DeadList))
      
    except Exception, reason:
      printProblem(reason, THISTEST)
      
      for workspace in mantid.getWorkspaceNames() :
        if (workspace == NORMA) : mantid.deleteWorkspace(NORMA)
        
      raise Exception('A discription of the error has been printed')
  finally:
    # the C++ that called this needs to look at the output from the print statements and deal with the fact that there was a problem
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == TEMPBIG) : mantid.deleteWorkspace(TEMPBIG)
      if (workspace == NUMER) : mantid.deleteWorkspace(NUMER)
      if (workspace == SUM) : mantid.deleteWorkspace(SUM)
      if (workspace == "_FindBadDetects loading") : mantid.deleteWorkspace("_FindBadDetects loading")

def diagnose(inst_prefix='',maskFile='',wbrf='',wbrf2='',runs='',zero='False',out_asc='', prevList='', tiny=1e-10,huge=1e10,median_lo=0.1,median_hi=3.0,sv_sig=3.3,bmedians=5.0,bmin=-1e8,bmax=-1e8, previousWS='', outWS = OUTPUTWS) :
  
  # Set the default instrument so that Mantid can search for the runs correctly
  old_inst = mtd.settings["default.instrument"]
  mtd.settings["default.instrument"] = inst_prefix

  try:
    failed = common.listToString(prevList)                                                      #failed will be a running total of all the bad detectors
    if (failed != '') and (failed[len(failed)-1] != ',') : failed += ','                #numbers are separated by commas, but avoid having two commas next to one another (from empty lists)
    
    #--load input workspace and files and get the names of the output workspace and files
    if maskFile != '':
      failed = common.load_mask(maskFile)
      if (failed != '') and (failed[len(failed)-1] != ',') : failed += ','                #numbers are separated by commas, but avoid having two commas next to one another (from empty lists)
      outputWSName = OUT_WS_PREFIX+maskFile
    #---the white beam vanadium file takes precendence over the mask file for naming
    if wbrf != '':
      #--Import the file and give it an obsure workspace name because we require that a workspace with that name doesn't exist and we'll remove it at the end
      outputWSName = OUT_WS_PREFIX+'1WBV_'+ common.create_resultname(wbrf, inst_prefix)
    #--the outfile name has the highest precendence
    if out_asc != '':
      outputWSName = OUT_WS_PREFIX+'1WBV_' + common.create_resultname(out_asc, inst_prefix)
      outFile=open(out_asc, 'w')
      if len(failed) > 0:
        outFile.write('--Hard Mask File List--\n')
        data = common.stringToList(failed)
        for datum in data : 
          outFile.write(str(datum) + ' ')
        outFile.write('\n')
      outFile.close()

    #of the three detector functioning tests run the ones that there are data for
    if str(wbrf) != '':
      failed = single_white_van(wbrf, tiny=tiny, huge=huge, median_lo=median_lo, median_hi=median_hi, error_bars=sv_sig, prevList=failed, oFile=out_asc, outWS=outWS)
      if (failed != '') and (failed[len(failed)-1] != ',') : failed += ','                #numbers are separated by commas, but avoid having two commas next to one another (from empty lists)
    
    if str(wbrf2) != '':
      failed = second_white_van(wbrf2, tiny=tiny, huge=huge, median_lo=median_lo, median_hi=median_hi, error_bars=sv_sig, oFile=out_asc,\
          prevList=failed, previous_wb_ws=SINGLE_WHITE_WS, outWS=outWS)
      if (failed != '') and (failed[len(failed)-1] != ',') : failed += ','                #numbers are separated by commas, but avoid having two commas next to one another (from empty lists)

    if runs != '':
      run_nums = common.listToString(runs).split(',')
      failed = bgTest(inst_prefix, run_nums, wb_ws=SINGLE_WHITE_WS, TOFLow=bmin, TOFHigh=bmax, threshold=bmedians, rmZeros=zero, error_bars=sv_sig, \
            prevList=failed, wb_ws_comp=previousWS,  oFile=out_asc, outWS=outWS)

    #pass back the list of those that failed but do nice, if tedious, things with the commas
    mtd.settings["default.instrument"] = old_inst
    return common.stringToList(failed)
  except Exception, reason:
    mtd.settings["default.instrument"] = old_inst
    if reason != 'A discription of the error has been printed' :
      printProblem(reason, 'diagnose()')
