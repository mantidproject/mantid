from mantidsimple import *
import CommonFunctions as common
# these are the defaults for different instruments
import ExcitDefaults
import math

def NormaliseTo(reference, WS, fromTOF, instrument=common.defaults()):
  if (reference == 'monitor-monitor 1') :
    if instrument.monitor1_integr[0] <= -1e5 or instrument.monitor1_integr[1] <= -1e5 :
      raise Exception('Can\'t normalize to monitor1 without instrument defaults information')
    min = instrument.monitor1_integr[0]-fromTOF
    max = instrument.monitor1_integr[1]-fromTOF
    NormaliseToMonitor( InputWorkspace=WS, OutputWorkspace=WS, MonitorSpectrum=1, IntegrationRangeMin=min, IntegrationRangeMax=max)

  elif reference == 'monitor-monitor 2' :

    PEAKWIDTH = 4.0/100
    # the origin of time was set to the location of the peak so to get it's area integrate around the origin, PEAKWIDTH fraction of total time of flight
    min = (-PEAKWIDTH/2)*20000
    max = (PEAKWIDTH/2)*20000

    NormaliseToMonitor( InputWorkspace=WS, OutputWorkspace=WS, MonitorSpectrum=2, IntegrationRangeMin=min, IntegrationRangeMax=max)

  elif reference == 'protons (uAh)' :
    NormaliseByCurrent( InputWorkspace=WS, OutputWorkspace=WS )

  elif reference != 'no normalization' :
    raise Exception('Normalisation scheme ' + reference + ' not found. It must be one of monitor, current, peak or none')

def NormaliseToWhiteBeam(instr, WBRun, toNorm, mapFile, detMask, prevNorm) :
  theNorm = "_ETrans_norm_tempory_WS"
  try:
    pNorm = common.LoadNexRaw(instr.getFileName(WBRun), theNorm)
    NormaliseTo(prevNorm, pNorm, 0, instr)

    ConvertUnits(pNorm, pNorm, "Energy", AlignBins=0)
    
    #this both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
    #    wb_rebin = str(wb_low_e) + ', ' + str(2.0*float(wb_high_e)) + ', ' + str(wb_high_e)
    low = instr.white_beam_integr[0]
    hi = instr.white_beam_integr[1]
    delta = 2.0*(hi - low)
    Rebin(pNorm, pNorm, str(low)+','+str(delta)+','+str(hi)  )

    if detMask != '' :
      MaskDetectors(pNorm, SpectraList=detMask)
    
    if mapFile!= "" :
      GroupDetectors(pNorm, pNorm, mapFile, KeepUngroupedSpectra=0)

    toNorm /= pNorm                               # we need '/=' here '/' by itself would do something different (create a new workspace)
  
  finally:
    mantid.deleteWorkspace(theNorm)

# returns the background range from the string range, which could be two numbers separated by a comma or could be empty and then the default values in instrument are used. Another possiblity is that background correction isn't going to be done, then we don't have to do anything
# throws if the string range is in the wrong format
def getBackRange(range, instrument) :
  if range == 'noback' :  return ['noback', 'noback']
  # load the default time of flight values that delimit the background region
  TOFLow = instrument.background_range[0]
  TOFHigh = instrument.background_range[1]
  if range != '' :
    range = common.listToString(range)
    strings = range.split(',')
    TOFLow = float(strings[0])
    TOFHigh = float(strings[1])
  return [TOFLow, TOFHigh]
  
## return specified normalisation method or the default for the instrument if '' was passed
def getNorm(instrument, normMethod):
  if normMethod == '' : return instrument.normalization
  return normMethod

###--Read in user set parameters or defaults values---------------
def retrieveSettings(instrum, back, runs):
  instru = ExcitDefaults.loadDefaults(instrum)
  
  run_nums = common.listToString(runs).split(',')
  try:
    [TOFLow, TOFHigh] = getBackRange(back, instru)
  except:
    raise Exception('The background range must be specified as a string\n of two numbers separated by a comma specifing the range\nor noback to disable background correction')
  
  return (instru, TOFLow, TOFHigh, run_nums)

def findPeaksOffSetTimeAndEi(workS, E_guess, getEi=True, detInfoFile=''):
  if detInfoFile != '' :
    LoadDetectorInfo(workS, detInfoFile)             				                  #for details see www.mantidproject.org/LoadDetectorInfo

  runGetEi = str(getEi).lower() != 'fixei' and str(getEi).lower() != 'false'
  if not runGetEi :
    mtd.sendLogMessage('Getting peak times (the GetEi() energy will be overriden by '+str(E_guess)+' meV)')
    
  # Get incident energy
  GetEiData = GetEi(workS, 2, 3, E_guess)

  if runGetEi : Ei = GetEiData.getPropertyValue('IncidentEnergy')
  else : Ei = E_guess
  
  offSetTime=float(GetEiData.getPropertyValue('FirstMonitorPeak'))             # set the origin of time to be when the neutrons arrive at the first GetEi monitor (often called monitor 2)
  ChangeBinOffset(workS, workS, -offSetTime)

  #??STEVES?? MARI specfic code GetEiData.FirstMonitor ?
  p0=workS.getDetector(1).getPos()                                              # set the source to be where the neutrons were at the origin of time (that first GetEi monitor)
  mtd.sendLogMessage('Adjusting the X-values so that the zero of time is when the peak was registered at the new origin , which monitor 2')
  src = workS.getInstrument().getSource().getName()                             #this name could be 'Moderator',  'undulator', etc.
  MoveInstrumentComponent(workS, src, X=p0.getX() , Y=p0.getY() ,Z=p0.getZ(), RelativePosition=False)
  return Ei, offSetTime
  
# a workspace name that is very long and unlikely to have been created by the user before this script is run, it would be replaced  
tempWS = '_ETrans_loading_tempory_WS'

###########################################
# Applies unit conversion, detector convcy and grouping correction to a
# raw file
###########################################
def mono_sample(instrum, runs, Ei, d_rebin, wbrf, getEi=True, back='', norma='', det_map='', det_mask='', nameInOut = 'mono_sample_temporyWS') :
  (instru, TOFLow, TOFHigh, run_nums) = retrieveSettings(instrum, back, runs)

  #----Calculations start------------------
  try:
    pInOut = instru.loadRun(run_nums[0], nameInOut)                             #this new workspace is accessed by its name nameInOut = 'mono_sample_temporyWS' or pInOut (a pointer)
    if pInOut.isGroup() : raise Exception("Workspace groups are not supported here")

    common.sumWorkspaces(pInOut, instru, run_nums)                              #the final workspace will take the X-values and instrument from the first workspace and so we don't need to rerun ChangeBinOffset(), MoveInstrumentComponent(), etc.
    Ei, offSet = findPeaksOffSetTimeAndEi(pInOut, Ei, getEi, instru.getFileName(run_nums[0]))
 
    if back != 'noback':                                                        #remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined a region
      ConvertToDistribution(pInOut)                                             #deal correctly with changing bin widths
      FlatBackground(pInOut, pInOut, TOFLow-offSet, TOFHigh-offSet, '', 'Mean') #the TOFs in the workspace were offset in a command above so we must take care referring to times now 
      ConvertFromDistribution(pInOut)                                           #some algorithms later can't deal with distributions
  
    # deals with normalize to monitor (e.g.  norm = 'monitor-monitor 1'), current (if norm = 'protons (uAh)'), etc.
    NormaliseTo(getNorm(instru, norma), pInOut, offSet, instru)
  
    ConvertUnits(pInOut, pInOut, 'DeltaE', 'Direct', Ei, AlignBins=0)

    if d_rebin != '':
      Rebin(pInOut, pInOut, common.listToString(d_rebin))
 
    DetectorEfficiencyCor(pInOut, pInOut, Ei)
 
    if det_mask != '' :
      MaskDetectors(Workspace=pInOut, SpectraList=common.listToString(det_mask))
    
    if det_map != '':
      GroupDetectors(pInOut, pInOut, det_map, KeepUngroupedSpectra=0)

    ConvertToDistribution(pInOut)
    pInOut *= instru.scale_factor 
    
    #replaces inifinities and error values with large numbers. Infinity values can be normally be avoided passing good energy values to ConvertUnits
    ReplaceSpecialValues(pInOut, pInOut, 1e40, 1e40, 1e40, 1e40)

    NormaliseToWhiteBeam(instru, wbrf, pInOut, det_map, det_mask, norma)
    pInOut /= instru.white_beam_scale
    
    return pInOut

  except Exception, reason:
    # delete the possibly part finished workspaces
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == nameInOut) : mantid.deleteWorkspace(nameInOut)
      if (workspace == tempWS) : mantid.deleteWorkspace(tempWS)
    print reason
    #best to raise the exception if you know that all your Python environments can take that
    #raise

#below is a quick test/example command to run this library, it must be commented or the library won't work!
#badSpectra = diagnose(instrum='MAR',wbrf='11060',wbrf2='11060',runs='15537',tiny=1e-10,huge=1e10,median_lo=0.1,median_hi=3.0,sv_sig=3.3,bmedians=5.0,zero='False', out_asc='',maskFile='')
#getBackRange('('+'18000'+','+'19000'+')', ExcitDefaults.loadDefaults('MAR'))
#(instru, TOFLow, TOFHigh, run_nums) = retrieveSettings('MAR', 'noback', '11001')
#pRes = mono_sample('MAR', '15537', 82.541, '-10,0.1, 70', 11060, getEi='false', back='('+'18000'+','+'19000'+')', norma='monitor-monitor 1', det_map='mari_res.map', det_mask=[])
