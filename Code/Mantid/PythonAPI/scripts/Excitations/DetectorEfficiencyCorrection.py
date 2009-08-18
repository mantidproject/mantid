TempWorkspace = "Will be deleted when FindBadDetects ends"
try:
  #--Get user input
  InSettings = EfficiencyScriptInputDialog()
  InputFN = InSettings.getPropertyValue("RawFile")
  IncidentE = InSettings.getPropertyValue("IncidentEnergy")
  RebinBoundaries = InSettings.getPropertyValue("BinBoundaries")
  MaskedDetectIDs = InSettings.getPropertyValue("MaskList")
  OutputFN = InSettings.getPropertyValue("RawFile")
  
  LoadRaw(InputFN, TempWorkspace)
  ConvertUnits(TempWorkspace, TempWorkspace, "DeltaE", "Direct", IncidentE, false)
  rebin(TempWorkspace, TempWorkspace, RebinBoundaries)
  # ??correct for detector efficiency??
  maskdetectors
  # loop around and run groupdetectors depending on what is in their file
  
  #does solid angle work OK with the masked?
  SA = SolidAngle(TempWorkspace, "Angles")
  TempWorkspace = divide(TempWorkspace, SA.getPropertyValue("OutputWorkspace"))
  
  # -output to file OutputFN in ASCII, this is an algorithm like SaveAscii
finally:
mantid.delete(TempWorkspace)