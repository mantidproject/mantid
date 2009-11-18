inOutWS = "Will be deleted when DetectorEfficiencyCorrection ends"

#----Start here------------------
#--Get user input
#RawFile = "//isis/inst$/NDXMARI/Instrument/data/cycle_09_3/MAR15306.raw",\
InSettings = EfficiencyScriptInputDialog(Message="Enter input and output file names and settings",\
						      RawFile = "?c:/Mantid/test/Data/MAR11001.raw",\
                                                      BinBoundaries = "?-11.0, 0.05, 11.0",\
						      MapFile = "?C:/Users/wht13119/Desktop/docs/Excitations/mari_res.map",\
						      OutFile = "?C:/Users/wht13119/Desktop/docs/mari.spe"
						      )
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

#  ApplyDetectorMask( inOutWS, InSettings.getPropertyValue("MaskFile") )

  DetectorEfficiencyCor(inOutWS, "efficiencies", IncidentE)  
    
  GroupDetectors( "efficiencies", "efficiencies", InSettings.getPropertyValue("MapFile") , KeepUngroupedSpectra=0)
  GroupDetectors( inOutWS, inOutWS, InSettings.getPropertyValue("MapFile") , KeepUngroupedSpectra=0)
  
  SA = SolidAngle("efficiencies", "Angles")
  Divide("efficiencies", SA.getPropertyValue("OutputWorkspace"), "Angles")
  
  # do we do a normalisation agains the monitors?
  
  # -output to a file in ASCII
  SaveSPE(inOutWS, "MAR11001 uncorrected.spe")
  SaveSPE("efficiencies", "MAR11001 b4 angles.spe")
  SaveSPE("Angles", "MAR11001 All.spe")
  
finally:
  i = 0
  #  mantid.delete(inOutWS)
