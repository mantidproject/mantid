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

def NormaliseToWhiteBeam(WBRun, toNorm, mapFile, start, end) :
  theNorm = "_ETrans_norm_tempory_WS"
  common.LoadNexRaw(WBRun, theNorm)
 #comment the next line out?
  LoadDetectorInfo(theNorm, WBRun)
  
  ConvertUnits(theNorm, theNorm, "Energy", AlignBins = 0)

  #this both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
  Rebin(theNorm, theNorm, str(start)+', ' + str(end-start)+', ' + str(end))
 # shouldn't we do the correction? It affects things when the angles are different
#  DetectorEfficiencyCor(theNorm, theNorm, IncidentE)  
    
  if mapFile!= "" :
    GroupDetectors( theNorm, theNorm, mapFile, KeepUngroupedSpectra=0)

  Divide(toNorm, theNorm, toNorm)
  mantid.deleteWorkspace(theNorm)

# a workspace name that is very long and unlikely to have been created by the user before this script is run, it would be replaced  
tempWS = "_ETrans_loading_tempory_WS"
