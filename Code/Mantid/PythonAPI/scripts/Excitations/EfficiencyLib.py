from mantidsimple import *
import CommonFunctions as common

def NormaliseTo(reference, WS):
  if reference == 'monitor' :
    NormaliseToMonitor( WS, WS, MonitorSpectrum=1, IntegrationRangeMin=1000, IntegrationRangeMax=2000)

  elif reference == 'current' :
    NormaliseToCurrent( InputWorkspace=WS, OutputWorkspace=WS )

  elif reference == 'peak' : raise Exception('Normalization by peak area not implemented yet')

  elif reference != 'none' :
    raise Exception('Normalisation scheme ' + reference + ' not found. It must be one of monitor, current, peak or none')
	
def getRunName( path):
  # get the string after the last /
  filename = path.split('/')
  filename = filename[len(filename)-1]
  # and the last \
  filename = filename.split('\\')
  filename = filename[len(filename)-1]
  # remove the last '.' and everything after it i.e. the extension. If there is not extension this just returns the whole thing
  return filename.rpartition('.')[0]

def NormaliseToWhiteBeam(WBRun, toNorm, mapFile, start, end) :
  theNorm = "Temp workspace to be deleted soon"
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
tempWS = "_ConvertToETrans_loading_tempory_workspace"