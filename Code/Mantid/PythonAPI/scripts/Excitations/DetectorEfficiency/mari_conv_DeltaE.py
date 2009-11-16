inOutWS = "Will be deleted when DetectorEfficiencyCorrection ends"
def ApplyDetectorMask( workspace, filename ):
  maskFile = open(filename, 'r')
  for maskFile in f:
    print maskFile
  maskFile.close()

#----Start here------------------
#--Get user input
InSettings = EfficiencyScriptInputDialog(RawFile = "C:\mantid\Test\Data\MAP10266.RAW",\
                                                      BinBoundaries = "-11.0, 0.05, 11.0",\
						      MapFile = "C:\Users\wht13119\Desktop\docs\Excitations\mari_res.map",\
						      OutFile = "C:\Users\wht13119\Desktop\docs\mari.spe")
InputFN = InSettings.getPropertyValue("RawFile")
RebinBoundaries = InSettings.getPropertyValue("BinBoundaries")
DataFN = InSettings.getPropertyValue("OutFile")
  
try:
  LoadRaw(InputFN, inOutWS)
    
  GetEiData = GetEiDialog(inOutWS, 2, 3, 12.98)
  IncidentE = GetEiData.getPropertyValue("IncidentEnergy")
  
  try:
    LoadDetectorInfo(inOutWS, InputFN)
  except:
    print "Skipping the corrections in LoadDetectorInfo"

  ConvertUnits(inOutWS, inOutWS, "DeltaE", "Direct", IncidentE, 0)
  
  Rebin(inOutWS, "efficiencies", RebinBoundaries)

  DetectorEfficiencyCor("efficiencies", inOutWS, IncidentE)  
#  Divide("efficiencies", inOutWS, "efficiencies")
  
#  ApplyDetectorMask( inOutWS, InSettings.getPropertyValue("MaskFile") )
  
  GroupDetectors( "efficiencies", "grouped", InSettings.getPropertyValue("MapFile") )
  
  #does solid angle work OK with the masked?
  SA = SolidAngle(inOutWS, "Angles")
  Divide(inOutWS, SA.getPropertyValue("OutputWorkspace"), inOutWS)
  
  # do we do a normalisation agains the monitors?
  
  # -output to a file in ASCII
  SaveSPEDialog(inOutWS, DataFN)
  
finally:
  i = 0
  #  mantid.delete(inOutWS)
