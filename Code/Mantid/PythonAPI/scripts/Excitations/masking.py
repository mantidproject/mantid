DumpOutput = "Will be deleted when FindBadDetects ends"

try:
  FirstAlg = FindDeadDetectorsDialog(message="Enter the name of the first white beam vanadium workspace", OutputWorkspace=DumpOutput,disable="LiveValue, DeadValue")
  InputWBV1 = FirstAlg.getPropertyValue("InputWorkspace")
  RangeL = FirstAlg.getPropertyValue("RangeLower")
  RangeU = FirstAlg.getPropertyValue("RangeUpper")

  SecondAlg = FindDeadDetectorsDialog(message="Enter the name of the second white beam vanadium workspace", OutputWorkspace=DumpOutput, RangeLower=RangeL, RangeUpper=RangeU, disable="LiveValue, DeadValue")
  InputWBV2 = SecondAlg.getPropertyValue("InputWorkspace")
  MaskDetectors( InputWBV1, DetectorList=FirstAlg.getPropertyValue("FoundBad")  )
  MaskDetectors( InputWBV2, DetectorList=SecondAlg.getPropertyValue("FoundBad")  )

  Median1 = WBVMedianTestDialog( message="Optionally change high and low thresholds", WhiteBeamWorkspace=InputWBV1, OutputWorkspace=DumpOutput, LowThreshold="?0.1", HighThreshold="?1.5", RangeLower=RangeL, RangeUpper=RangeU, disable="StartSpectrum, EndSpectrum" )
  MaskDetectors( InputWBV1, Median1.getPropertyValue("FoundBad") )

  Median2 = WBVMedianTest( WhiteBeamWorkspace=InputWBV2, OutputWorkspace=DumpOutput, LowThreshold=Median1.getPropertyValue("LowThreshold"), HighThreshold=Median1.getPropertyValue("HighThreshold"), RangeLower=RangeL, RangeUpper=RangeU )
  MaskDetectors( InputWBV2, Median2.getPropertyValue("FoundBad") )

  EfficiencyVar = DetectorEfficiencyVariationDialog( message="Settings for efficiency variation check", WhiteBeamBase=InputWBV1, WhiteBeamCompare=InputWBV2, OutputWorkspace=DumpOutput, RangeLower=RangeL, RangeUpper=RangeU, disable="StartSpectrum, EndSpectrum" )

  MaskDetectorsDialog( DetectorList=EfficiencyVar.getPropertyValue("BadIds"),disable="SpectraList, WorkspaceIndexList" )

 finally:
  mantid.deleteWorkspace(DumpOutput)
