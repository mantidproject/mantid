from mantidsimple import *
import CommonFunctions as common

def NormaliseTo(reference, WS):
  if reference == 'monitor-monitor peak1' :
    NormaliseToMonitor( WS, WS, MonitorSpectrum=1, IntegrationRangeMin=1000, IntegrationRangeMax=2000)

  elif reference == 'protons (uAh)' :
    NormaliseByCurrent( InputWorkspace=WS, OutputWorkspace=WS )

  elif reference == 'monitor-peak2 area' : raise Exception('Normalization by peak area not implemented yet')

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
