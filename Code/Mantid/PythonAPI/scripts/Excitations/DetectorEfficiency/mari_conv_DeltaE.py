inOutWS = "Will be deleted when DetectorEfficiencyCorrection ends"
def NormaliseToWhiteBeam(WBV, MapFile, DataWS):
    tempWS = "Temp workspace to be deleted soon"
    LoadRaw(WBV, tempWS)
    GroupDetectors( tempWS, tempWS, MapFile , KeepUngroupedSpectra=0)
    Integration(tempWS, tempWS)
    Divide(inOutWS, tempWS, DataWS)
  
#----Start here------------------
#--Get user input
InSettings = EfficiencyScriptInputDialog(Message="Enter input and output file names and settings",\
						      RawFile = "?c:/Mantid/test/Data/MAR11001.raw",\
                                                      BinBoundaries = "?-12.0, 0.05, 12.0",\
						      WhiteBeamVan = "?c:/Mantid/test/Data/MAR11060.raw",\
						      MapFile = "?C:/Users/wht13119/Desktop/docs/Excitations/mari_res.map",\
						      OutFile = "?C:/Users/wht13119/Desktop/docs/MAR11001_MANTID.spe"
						      )
InputFN = InSettings.getPropertyValue("RawFile")
RebinBoundaries = InSettings.getPropertyValue("BinBoundaries")
DataFN = InSettings.getPropertyValue("OutFile")
  
try:
  LoadRaw(InputFN, inOutWS)
    
  GetEiData = GetEi(inOutWS, 2, 3, 14)
  IncidentE = GetEiData.getPropertyValue("IncidentEnergy")
  # HOMER gets 12.973 meV for IncidentE GetEi() returns 12.9739 meV
  
  LoadDetectorInfo(inOutWS, InputFN)

  ConvertUnits(inOutWS, inOutWS, "DeltaE", "Direct", IncidentE, 0)

  Rebin(inOutWS, inOutWS, RebinBoundaries)

#  ApplyDetectorMask( inOutWS, InSettings.getPropertyValue("MaskFile") )

  DetectorEfficiencyCor(inOutWS, inOutWS, IncidentE)  
    
  GroupDetectors( inOutWS, inOutWS, InSettings.getPropertyValue("MapFile") , KeepUngroupedSpectra=0)
  
  NormaliseToWhiteBeam(InSettings.getPropertyValue("WhiteBeamVan"), InSettings.getPropertyValue("MapFile"), inOutWS)
  
  # -output to a file in ASCII
  SaveSPE(inOutWS, InSettings.getPropertyValue("OutFile"))
  
finally:
  i = 0
  #  mantid.delete(inOutWS)
