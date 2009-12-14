# Used by MantidPlot if the script in is in Code\Mantid\PythonAPI\scripts\Excitations make a
# copy of the script if you want to change it or risk the MantidPlot interface not working
###########################################
# applies unit conversion, detector efficiency and grouping correction to a
# raw file and writes the result to a SPE file
###########################################
from mantidsimple import *
import CommonFunctions as common
import EfficiencyFunctions as efficiency

inOutWS = "Last output from mari_conv_DeltaE.py"

#----Start here------------------
#--Get user input
InSettings = EfficiencyScriptInputDialog(Message="Enter input and output file names and settings",\
						      RawFile = "?c:/Mantid/test/Data/MAR11001.raw",\
                              BinBoundaries = "?-10.0, 0.1, 13.0",\
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
    
  ##this needs to depend on what the user selects
  efficiency.NormaliseTo('monitor', inOutWS)
  
  #--mask detectors that have failed tests run previously
  badDets = []
  maskFilename = InSettings.getPropertyValue("DetectorMask") 
  if maskFilename != "":
    badDets = common.loadMask(maskFilename)
    MaskDetectors(Workspace=inOutWS, SpectraList=badDets)
  
  ConvertUnits(inOutWS, inOutWS, "DeltaE", "Direct", IncidentE, 0)

  Rebin(inOutWS, inOutWS, InSettings.getPropertyValue("BinBoundaries"))
  
  DetectorEfficiencyCor(inOutWS, inOutWS, IncidentE)  
    
  GroupDetectors( inOutWS, inOutWS, InSettings.getPropertyValue("MapFile") , KeepUngroupedSpectra=0)

  #####remove this line###
  ReplaceSpecialValues(inOutWS, inOutWS, 0, 0, 0)
  # -output to a file in ASCII
  SaveSPE(inOutWS, InSettings.getPropertyValue("OutFile"))
  
finally:
 #  mantid.delete(inOutWS)
  # the line below does nothing and should be replaced by the one above
  i = 0
