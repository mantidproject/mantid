from mantidsimple import *

def NormaliseToWhiteBeam(WS, WBV_RAW, detMask, reference):
  WBTempWS = "Temp workspace to be deleted soon"
  LoadRaw(WBV_RAW, WBTempWS)
  #?? do the whole convert units thing and integerate within the energy range
  Integration(WBTempWS, WBTempWS)
  # if we are integrating by peak area we need to be careful of when the integration happens
  if reference == 'peak' : raise Exception('Normalization by peak area not implemented yet')
  MaskDetectors(Workspace=WBTempWS, SpectraList=detMask)
  Divide(WS, WBTempWS, WS)
  mantid.deleteWorkspace(WBTempWS)
  ReplaceSpecialValues(WS, WS, 0, 0, 0)

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
  filename = path.rsplit('/')[0]
  # and the last \
  filename = filename.rsplit('\\')[0]
  # prepare to remove the section after the _last_ '.' allowing for more than one '.' in the name
  rootDotExtension = filename.rpartition('.')
  return rootDotExtension[2]
