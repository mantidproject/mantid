###########################################
# applies unit conversion, detector efficiency and grouping correction to a
# raw file and writes the result to a SPE file
###########################################
from mantidsimple import *

inOutWS = "Last output from mari_conv_DeltaE.py"
def NormaliseToWhiteBeam(WBV, MapFile, DataWS):
    tempWS = "Temp workspace to be deleted soon"
    LoadRaw(WBV, tempWS)
    GroupDetectors( tempWS, tempWS, MapFile , KeepUngroupedSpectra=0)
    Integration(tempWS, tempWS)
    Divide(inOutWS, tempWS, DataWS)
    mantid.deleteWorkspace(tempWS)

def loadMask(MaskFilename):
  inFile = open(MaskFilename)
  spectraList = ""
  for line in inFile:
    if line[0] != '-':
      if line[0] != '#':
        numbers = line.split()
        for specNumber in numbers:
          spectraList = spectraList + ", " + specNumber
  #get rid of the inital coma and space
  return spectraList[2:]

#----Start here------------------
#--Get user input
InSettings = EfficiencyScriptInputDialog(Message="Enter input and output file names and settings",\
						      RawFile = "?c:/Mantid/test/Data/MAR11001.raw",\
                                                      BinBoundaries = "?-12.0, 0.05, 12.0",\
						      WhiteBeamVan = "?c:/Mantid/test/Data/MAR11060.raw",\
						      DetectorMask = "?C:/Users/wht13119/Desktop/docs/MAR11001.mask",\
						      MapFile = "?C:/Users/wht13119/Desktop/docs/Excitations/mari_res.map",\
						      OutFile = "?C:/Users/wht13119/Desktop/docs/MAR11001_MANTID.spe"
						      )  
try:#-now load the data and do the conversions
  LoadRaw(InSettings.getPropertyValue("RawFile"), inOutWS)
    
  GetEiData = GetEi(inOutWS, 2, 3, 14)
  IncidentE = GetEiData.getPropertyValue("IncidentEnergy")

  LoadDetectorInfo(inOutWS, InSettings.getPropertyValue("RawFile"))

  maskFilename = InSettings.getPropertyValue("DetectorMask") 
  if maskFilename != "":
    badDets = loadMask(maskFilename)
    MaskDetectors(Workspace=inOutWS, SpectraList=badDets)
    
  ConvertUnits(inOutWS, inOutWS, "DeltaE", "Direct", IncidentE, 0)

  Rebin(inOutWS, inOutWS, InSettings.getPropertyValue("BinBoundaries"))

  DetectorEfficiencyCor(inOutWS, inOutWS, IncidentE)  
    
  GroupDetectors( inOutWS, inOutWS, InSettings.getPropertyValue("MapFile") , KeepUngroupedSpectra=0)
  
  NormaliseToWhiteBeam(InSettings.getPropertyValue("WhiteBeamVan"), InSettings.getPropertyValue("MapFile"), inOutWS)
  
  # -output to a file in ASCII
  SaveSPE(inOutWS, InSettings.getPropertyValue("OutFile"))
  
finally:
 #  mantid.delete(inOutWS)
  # the line below does nothing and should be replaced by the one above
  i = 0
