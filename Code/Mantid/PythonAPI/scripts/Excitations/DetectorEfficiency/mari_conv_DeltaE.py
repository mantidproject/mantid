inOutWS = "Will be deleted when DetectorEfficiencyCorrection ends"
def ApplyDetectorMask( workspace, filename ):
  maskFile = open(filename, 'r')
  for maskFile in f:
    print maskFile
  maskFile.close()

#----Start here------------------
#--Get user input
InSettings = EfficiencyScriptInputDialog(
#						      RawFile = "//isis/inst$/NDXMARI/Instrument/data/cycle_09_3/MAR15306.raw",\
						      RawFile = "c:/Mantid/test/Data/MAR11001.raw",\
                                                      BinBoundaries = "-11.0, 0.05, 11.0",\
						      MapFile = "C:/Users/wht13119/Desktop/docs/Excitations/mari_res.map",\
						      OutFile = "C:/Users/wht13119/Desktop/docs/mari.spe")
InputFN = InSettings.getPropertyValue("RawFile")
RebinBoundaries = InSettings.getPropertyValue("BinBoundaries")
DataFN = InSettings.getPropertyValue("OutFile")
  
try:
  LoadRaw(InputFN, inOutWS)
    
  #GetEiData = GetEi(inOutWS, 2, 3, 6.5)
  GetEiData = GetEi(inOutWS, 2, 3, 14)
  IncidentE = GetEiData.getPropertyValue("IncidentEnergy")
  
  LoadDetectorInfo(inOutWS, InputFN)

  ConvertUnits(inOutWS, inOutWS, "DeltaE", "Direct", IncidentE, 0)

  Rebin(inOutWS, inOutWS, RebinBoundaries)

  DetectorEfficiencyCor(inOutWS, "efficiencies", IncidentE)  
#  Divide("efficiencies", inOutWS, "efficiencies")
  
#  ApplyDetectorMask( inOutWS, InSettings.getPropertyValue("MaskFile") )
  
  GroupDetectors( "efficiencies", "efficiencies", InSettings.getPropertyValue("MapFile") )
  GroupDetectors( inOutWS, inOutWS, InSettings.getPropertyValue("MapFile") )
  
  SA = SolidAngle("efficiencies", "Angles")
  Divide("efficiencies", SA.getPropertyValue("OutputWorkspace"), "Angles")
  
  # do we do a normalisation agains the monitors?
  
  # -output to a file in ASCII
  SaveSPE("efficiencies", DataFN)
  
finally:
  i = 0
  #  mantid.delete(inOutWS)
