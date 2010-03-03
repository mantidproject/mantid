from mantidsimple import *
import CommonFunctions as common
# these are the defaults for different instruments
import defaults
import math

##Estimate the location of the monitor peak, used to help with normalise to monitor
def getPeakTime(Ei, monSpec, WS):
  source = WS.getInstrument().getSource();


#??STEVES?? replace MARI specific code below
#  dets = WS.spectraMap().getDetectors(monSpec);
#  if len(dets) != 1 : raise Exception('Found a grouped monitor, this is not supported')



#!! MARI Specific code!
  det = WS.getDetector(\
  \
  \
    monSpec-1\
  \
  \
  \
  );
  distance = det.getDistance(source)
  
  # convert distance to time, knowingthe kinetic energy:
  # E_KE = mv^2/2, s = vt
  # t = s/v, v = sqrt(2*E_KE/m)
  # t = s/sqrt(2*E_KE/m)

  # convert E_KE to joules kg m^2 s^-2
  NeutronMass = 1.674927211e-27 #kg
  meV = 1.602176487e-22             #joules
  Ei *= meV;
  # return the calculated time in micro-seconds
  return (1e6*distance)/math.sqrt(2*Ei/NeutronMass)

def NormaliseTo(reference, WS, energy=0):
  if (reference == 'monitor-monitor 1') :
    if (energy == 0) : raise Exception('A non-zero energy must be supplied for normalisation by monitor')
    # set the region where the monitor is
    min = getPeakTime(energy, 2, WS)*0.96
    max = getPeakTime(energy, 2, WS)*1.04
    NormaliseToMonitor( InputWorkspace=WS, OutputWorkspace=WS, MonitorSpectrum=2, IntegrationRangeMin=min, IntegrationRangeMax=max)

  elif reference == 'monitor-monitor 2' :
    if (energy == 0) : raise Exception('A non-zero energy must be supplied for normalisation by monitor')
    # set the region where the monitor is
    min = getPeakTime(energy, 3, WS)*0.96
    max = getPeakTime(energy, 3, WS)*1.04
    NormaliseToMonitor( InputWorkspace=WS, OutputWorkspace=WS, MonitorSpectrum=3, IntegrationRangeMin=min, IntegrationRangeMax=max)

  elif reference == 'protons (uAh)' :
    NormaliseByCurrent( InputWorkspace=WS, OutputWorkspace=WS )

  elif reference != 'no normalization' :
    raise Exception('Normalisation scheme ' + reference + ' not found. It must be one of monitor, current, peak or none')

def NormaliseToWhiteBeam(WBRun, toNorm, mapFile, detMask, rebinString) :
  theNorm = "_ETrans_norm_tempory_WS"
  try:
    common.LoadNexRaw(WBRun, theNorm)
    LoadDetectorInfo(theNorm, WBRun)

    ConvertUnits(theNorm, theNorm, "Energy", AlignBins=0)

    #this both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
    Rebin(theNorm, theNorm, rebinString)
   # shouldn't we do the correction? It affects things when the angles are different
  #  DetectorEfficiencyCor(theNorm, theNorm, IncidentE)  
    if detMask != '' :
      MaskDetectors(Workspace=theNorm, SpectraList=detMask)
    
    if mapFile!= "" :
      GroupDetectors( theNorm, theNorm, mapFile, KeepUngroupedSpectra=0)

    Divide(toNorm, theNorm, toNorm)
  
  finally:
    mantid.deleteWorkspace(theNorm)

def NormaliseToWhiteBeamAndLoadMask(WBRun, toNorm, mapFile, rebinString, DetectorMask) :
  theNorm = "_ETrans_norm_tempory_WS"
  try:
    common.LoadNexRaw(WBRun, theNorm)
    LoadDetectorInfo(theNorm, WBRun)

    ConvertUnits(theNorm, theNorm, "Energy", AlignBins = 0)

    #this both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
    Rebin(theNorm, theNorm, rebinString)
   # shouldn't we do the correction? It affects things when the angles are different
  #  DetectorEfficiencyCor(theNorm, theNorm, IncidentE)  

    if DetectorMask != '' :
      FDOL = FindDetectorsOutsideLimits(InputWorkspace=DetectorMask,OutputWorkspace='_ETrans_loading_bad_detector_WS',HighThreshold=10,LowThreshold=-1,OutputFile='')
      badNums = FDOL.getPropertyValue('BadSpectraNums')
      MaskDetectors(Workspace=theNorm, SpectraList=badNums)
      mantid.deleteWorkspace('_ETrans_loading_bad_detector_WS')
      
    if mapFile!= "" :
      GroupDetectors( theNorm, theNorm, mapFile, KeepUngroupedSpectra=0)

    Divide(toNorm, theNorm, toNorm)
    #bad detectors were identified by the last command as infinities or NaN (0/0 = Not a Number)
    ReplaceSpecialValues(toNorm, toNorm, NaNValue=-1e30, InfinityValue=-1e30)
  
  finally:
    mantid.deleteWorkspace(theNorm)

# returns the background range from the string range, which could be two numbers separated by a coma or could be empty and then the default values in instrument are used. Another possiblity is that background correction isn't going to be done, then we don't have to do anything
# throws if the string range is in the wrong format
def getBackRange(range, instrument) :
  if range == 'noback' : ['noback', 'noback']
  # load the default time of flight values that delimit the background region
  TOFLow = instrument.backgroundRange[0]
  TOFHigh = instrument.backgroundRange[1]
  if range != '' :
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
  instru = defaults.loadDefaults(instrum)
  
  try:
    [TOFLow, TOFHigh] = getBackRange(back, instru)
  except:
    raise Exception('The background range must be specified as a string\n  Two numbers separated by a coma to specify the range\n  noback to disable background correction\n  an empty string or absent for no background correction')
  
  run_nums = common.listToString(runs).split(',')
  
  return (instru, TOFLow, TOFHigh, run_nums)

# a workspace name that is very long and unlikely to have been created by the user before this script is run, it would be replaced  
tempWS = '_ETrans_loading_tempory_WS'

###########################################
# Applies unit conversion, detector convcy and grouping correction to a
# raw file
###########################################
def mono_sample(instrum, runs, Ei, d_rebin, wbrf, wb_low_e=0, wb_high_e=1e8, getEi='true', back='', norma='', det_map='', det_mask='', scaling=1):
  nameInOut = 'mono_sample_temporyWS'
  (instru, TOFLow, TOFHigh, run_nums) = retrieveSettings(instrum, back, runs)

  #----Calculations start------------------
  try:
    #this new workspace is accessed by its name nameInOut = 'mono_sample_temporyWS' or pInOut (a pointer)
    pInOut = instru.loadRunNumber(run_nums[0], nameInOut)
    if pInOut.isGroup() : raise Exception("Workspace groups are not supported here")

    if getEi.lower() != 'fixei' and getEi.lower() != 'false' :
      #?? 2 and 3 below are good for MARI, this needs to be changed in other instruments
      GetEiData = GetEi(pInOut, 2, 3, Ei)
      Ei = GetEiData.getPropertyValue('IncidentEnergy')

    LoadDetectorInfo(pInOut, instru.getFileName(run_nums[0]))

    common.sumWorkspaces(pInOut, run_nums)
  
    if back != 'noback':
      ConvertToDistribution(pInOut)
      FlatBackground(pInOut, pInOut, TOFLow, TOFHigh, '', 'Mean')
      ConvertFromDistribution(pInOut)
  
    # deals with normalize to monitor (e.g.  norm = 'monitor-monitor 1'), current (if norm = 'protons (uAh)'), etc.
    NormaliseTo(getNorm(instru, norma), pInOut, float(Ei))
  
    ConvertUnits(pInOut, pInOut, 'DeltaE', 'Direct', Ei, AlignBins=0)

    if d_rebin != '':
      Rebin(pInOut, pInOut, common.listToString(d_rebin))
 
    DetectorEfficiencyCor(pInOut, pInOut, Ei)
 
    if det_mask != '' :
      MaskDetectors(Workspace=pInOut, SpectraList=common.listToString(det_mask))
    
    if det_map != '':
      GroupDetectors( pInOut, pInOut, det_map, KeepUngroupedSpectra=0)

    ConvertToDistribution(pInOut)
  
    # multiply by the user defined arbitary scaling factor, used because some plotting applications prefer numbers close to 1
    if scaling != 1:
      CreateSingleValuedWorkspace(tempWS, scaling)
      Multiply(pInOut, tempWS, pInOut)
      mantid.deleteWorkspace(tempWS)
    #replaces inifinities and error values with large numbers. Infinity values can be normally be avoided passing good energy values to ConvertUnits
    ReplaceSpecialValues(pInOut, pInOut, 1e40, 1e40, 1e40, 1e40)

    wb_rebin = str(wb_low_e) + ', ' + str(2*float(wb_high_e)) + ', ' + str(wb_high_e)
    NormaliseToWhiteBeam(instru.getFileName(wbrf), pInOut, det_map, det_mask, wb_rebin)
    #bad detectors were identified by the last command as infinities or NaN (0/0 = Not a Number)
    ReplaceSpecialValues(pInOut, pInOut, NaNValue=-1e30, InfinityValue=-1e30)
   
    return pInOut

  except Exception, reason:
    # delete the possibly part finished workspaces
    for workspace in mantid.getWorkspaceNames() :
      if (workspace == nameInOut) : mantid.deleteWorkspace(nameInOut)
      if (workspace == tempWS) : mantid.deleteWorkspace(tempWS)
    print reason
    #best to raise the exception if you know that all your Python environments can take that
    #raise
