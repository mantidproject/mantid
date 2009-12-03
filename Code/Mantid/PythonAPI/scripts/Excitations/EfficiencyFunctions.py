from mantidsimple import *

def NormaliseToWhiteBeam(WS, WBV_RAW, detMask):
  WBTempWS = "Temp workspace to be deleted soon"
  LoadRaw(WBV_RAW, WBTempWS)
  Integration(WBTempWS, WBTempWS)
  MaskDetectors(Workspace=WBTempWS, SpectraList=detMask)
  Divide(WS, WBTempWS, WS)
  mantid.deleteWorkspace(WBTempWS)
  ReplaceSpecialValues(WS, WS, 0, 0, 0)
  
def NormaliseTo(WS, reference)
  if reference == 'monitor' :
    NormaliseToMonitor( WS, WS, MonitorSpectrum=1, IntegrationRangeMin=1000, IntegrationRangeMax=2000)

  elif reference == 'current' :
    NormaliseToCurrent( InputWorkspace=WS, OutputWorkspace=WS )

  elif reference != 'none' :
    raise Exception('Normalisation scheme ' + reference + ' not found. It must be one of monitor, current, peak or none')
  
