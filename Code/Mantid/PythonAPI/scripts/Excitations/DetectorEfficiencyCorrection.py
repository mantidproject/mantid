inOutWS = "Will be deleted when FindBadDetects ends"
def ApplyDetectorMask( workspace, filename )
  maskFile = open(filename, 'r')
  for maskFile in f:
print maskFile
  maskFile.close()

#----Start here------------------
#--Get user input
InSettings = EfficiencyScriptInputDialog()
InputFN = InSettings.getPropertyValue("RawFile")
IncidentE = InSettings.getPropertyValue("IncidentEnergy")
RebinBoundaries = InSettings.getPropertyValue("BinBoundaries")
DataFN = InSettings.getPropertyValue("RawFile")
  
try:
  LoadRaw(InputFN, inOutWS)
  LoadDetectorInfo(inOutWS, InputFN)
  
  IncidentE = getEiDialog(InputWorkspace = inOutWS)

  ConvertUnits(inOutWS, inOutWS, "DeltaE", "Direct", IncidentE, false)
  
  rebin(inOutWS, inOutWS, RebinBoundaries)

  DetectorEfficiencyCor(inOutWS, inputWS, IncidentE, pressure of He)#default is 10 atm partial pressure of He
  
  ApplyDetectorMask( inOutWS, InSettings.getPropertyValue("MaskFile") )
  
  GroupDetectors( inOutWS, inOutWS, InSettings.getPropertyValue("MapFile") )
  
  #does solid angle work OK with the masked?
  SA = SolidAngle(inOutWS, "Angles")
  inOutWS = divide(inOutWS, SA.getPropertyValue("OutputWorkspace"))
  
  # -output to file OutputFN in ASCII, see Stuartis algorithm
  
finally:
  mantid.delete(inOutWS)