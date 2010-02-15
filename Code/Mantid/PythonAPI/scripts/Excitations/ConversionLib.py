from mantidsimple import *
import CommonFunctions as common
import math

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
      detIDs = FDOL.getPropertyValue('BadDetectorIDs')
      MaskDetectors(Workspace=theNorm, DetectorList=detIDs)
      mantid.deleteWorkspace('_ETrans_loading_bad_detector_WS')
      
    if mapFile!= "" :
      GroupDetectors( theNorm, theNorm, mapFile, KeepUngroupedSpectra=0)

    Divide(toNorm, theNorm, toNorm)
    #bad detectors were identified by the last command as infinities or NaN (0/0 = Not a Number)
    ReplaceSpecialValues(toNorm, toNorm, NaNValue=-1e30, InfinityValue=-1e30)
  
  finally:
    mantid.deleteWorkspace(theNorm)

# a workspace name that is very long and unlikely to have been created by the user before this script is run, it would be replaced  
tempWS = "_ETrans_loading_tempory_WS"
