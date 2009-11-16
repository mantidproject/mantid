inOutWS = "Will be deleted when DetectorEfficiencyCorrection ends"
def ApplyDetectorMask( workspace, filename ):
  maskFile = open(filename, 'r')
  for maskFile in f:
    print maskFile
  maskFile.close()

#----Start here------------------
#--Get user input
InSettings = EfficiencyScriptInputDialog( \
  RawFile = "C:/mantid/Test/Data/MER02257.RAW",\
  BinBoundaries = "-10,0.1,35",\
  MapFile="C:/Users/wht13119/Desktop/docs/Excitations/merlin.map",\
  OutFile = "C:/Users/wht13119/Desktop/docs/mer.spe")

try:
  LoadRaw(InSettings.getPropertyValue("RawFile"), inOutWS\
  #, SpectrumMin = 69600
  )
    
  GetEiData = GetEiDialog(inOutWS, 69634, 69638, 14)
  IncidentE = GetEiData.getPropertyValue("IncidentEnergy")
  
  #LoadDetectorInfo(inOutWS, InputFN)
  LoadDetectorInfo(inOutWS, "C:/mantid/Test/Data/merlin_detector.sca")

  ConvertUnits(inOutWS, inOutWS, "DeltaE", "Direct", IncidentE, 0)
  
  Rebin(inOutWS, inOutWS, InSettings.getPropertyValue("BinBoundaries"))

  DetectorEfficiencyCor(inOutWS, "efficiencies", IncidentE)
#  Divide("efficiencies", inOutWS, "efficiencies")
  
#  ApplyDetectorMask( inOutWS, InSettings.getPropertyValue("MaskFile") )
  
  GroupDetectors( inOutWS, inOutWS, InSettings.getPropertyValue("MapFile") )
  
  #does solid angle work OK with the masked?
  SA = SolidAngle(inOutWS, "Angles")
  ####################
  Divide(inOutWS, SA.getPropertyValue("OutputWorkspace"), inOutWS)
  ####################
  
  # do we do a normalisation agains the monitors?
  
  # -output to a file in ASCII
  SaveSPE(inOutWS, InSettings.getPropertyValue("OutFile"))
  
finally:
  i = 0
  #  mantid.delete(inOutWS)
