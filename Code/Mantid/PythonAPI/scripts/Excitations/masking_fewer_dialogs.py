DumpOutput = "Will be deleted when FindBadDetects ends"

def FindDeadInSingleWS( inputWS, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars ) :
  FDOL = FindDetectorsOutsideLimits( inputWS, DumpOutput,  HighMedian, LowMedian, 0.0, 100.0 )
  MaskDetectors(inputWS, FDOL.getPropertyValue("BadDetectorIDs"))
  MDT = MedianDetectorTest(inputWS, DumpOutput, NumErrorBars, LowMedian, HighMedian)
  MaskDetectors(inputWS, MDT.getPropertyValue("BadDetectorIDs"))
  return MDT.getPropertyValue("BadDetectorIDs")

#----Start here
try:
  #--Get user input
  InSettings = DiagScriptInputDialog()
  WBVanadium1 = InSettings.getPropertyValue("WBVanadium1")
  HighAbsolute = InSettings.getPropertyValue("HighAbsolute")
  LowAbsolute = InSettings.getPropertyValue("LowAbsolute")
  HighMedian = InSettings.getPropertyValue("HighMedian")
  LowMedian = InSettings.getPropertyValue("LowMedian")
  NumErrorBars = InSettings.getPropertyValue("SignificanceTest")
  
  #--Do everything that can be done on a single white beam vanadium
  DeadList = FindDeadInSingleWS( WBVanadium1, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars)
  
  #--If there is a second white beam vanadium
  WBVanadium2 = InSettings.getPropertyValue("WBVanadium2")
  if ( WBVanadium2 != "" ) :
    FindDeadInSingleWS( WBVanadium2, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars )
    DEV = DetectorEfficiencyVariation( WBVanadium1, WBVanadium2, DumpOutput )
    DeadList = DEV.getPropertyValue("BadDetectorIDs")

  #--Finally correct the experimental data
  DataWS = InSettings.getPropertyValue("ExperimentWorkspace")
  MaskDetectors( DataWS, DetectorList=DeadList )

  
finally:
  mantid.deleteWorkspace(DumpOutput)
