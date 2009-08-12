DumpOutput = "Will be deleted when FindBadDetects ends"

def FindDeadInSingleWS( inputWS, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, RangeLower, RangeUpper, MaskFile ) :
  FDOL = FindDetectorsOutsideLimits( inputWS, DumpOutput,  HighMedian, LowMedian, 0.0, 100.0, RangeLower, RangeUpper )
  MaskDetectors( inputWS, FDOL.getPropertyValue("BadDetectorIDs") )
  MDT = MedianDetectorTest( InputWorkspace=inputWS, OutputWorkspace=DumpOutput, SignificanceTest=NumErrorBars, LowThreshold=LowMedian, HighThreshold=HighMedian, RangeLower=RangeLower, RangeUpper=RangeUpper, OutputFile=MaskFile )
  MaskDetectors(inputWS, MDT.getPropertyValue("BadDetectorIDs"))
  return MDT.getPropertyValue("BadDetectorIDs")

#----Start here
try:
  #--Get user input
  InSettings = DiagScriptInputDialog()
  WBVanadium1 = InSettings.getPropertyValue("WBVanadium1")
  RangeLower = InSettings.getPropertyValue("RangeLower")
  RangeUpper = InSettings.getPropertyValue("RangeUpper")
  HighAbsolute = InSettings.getPropertyValue("HighAbsolute")
  LowAbsolute = InSettings.getPropertyValue("LowAbsolute")
  HighMedian = InSettings.getPropertyValue("HighMedian")
  LowMedian = InSettings.getPropertyValue("LowMedian")
  NumErrorBars = InSettings.getPropertyValue("SignificanceTest")
  MaskFile = InSettings.getPropertyValue("OutputFile")

  #--Do everything that can be done on a single white beam vanadium
  DeadList = FindDeadInSingleWS( WBVanadium1, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, RangeLower, RangeUpper, MaskFile )
  
  #--If there is a second white beam vanadium
  WBVanadium2 = InSettings.getPropertyValue("WBVanadium2")
  if ( WBVanadium2 != "" ) :
    FindDeadInSingleWS( WBVanadium2, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, RangeLower, RangeUpper, MaskFile )
    DEV = DetectorEfficiencyVariation( WhiteBeamBase=WBVanadium1, WhiteBeamCompare=WBVanadium2, OutputWorkspace=DumpOutput, OutputFile=MaskFile )
    DeadList = DEV.getPropertyValue("BadDetectorIDs")

#  #--Finally correct the experimental data
#  DataWS = InSettings.getPropertyValue("ExperimentWorkspace")
#  MaskDetectors( DataWS, DetectorList=DeadList )

  
finally:
  mantid.deleteWorkspace(DumpOutput)
