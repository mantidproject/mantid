TempOutput = "Will be deleted when FindBadDetects ends"

def FindDeadInSingleWS( inputWS, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, MaskFile ) :
  FDOL = FindDetectorsOutsideLimits( inputWS, TempOutput, HighAbsolute, LowAbsolute )	#for usage see www.mantidproject.org/FindDetectorsOutsideLimits
  MaskDetectors( inputWS, FDOL.getPropertyValue("BadDetectorIDs") )				#for usage see www.mantidproject.org/MaskDetectors
  MDT = MedianDetectorTest( InputWorkspace=inputWS, OutputWorkspace=TempOutput, SignificanceTest=NumErrorBars, LowThreshold=LowMedian, HighThreshold=HighMedian, OutputFile=MaskFile )#for usage see www.mantidproject.org/
  MaskDetectors(inputWS, MDT.getPropertyValue("BadDetectorIDs"))					#for usage see www.mantidproject.org/MaskDetectors
  return MDT.getPropertyValue("BadDetectorIDs")

def BackgroundTest( Counts, DeadList, BackgroundAccept, BackWinLower, BackWinUpper, RemoveZero, MaskExper, MaskFile ) :
  #--remove any spectra that are already known to be bad
  Integration(Counts, TempOutput, BackWinLower, BackWinUpper) 					#a Mantid algorithm
  MaskDetectors(TempOutput, DeadList)										#for usage see www.mantidproject.org/MaskDetectors
  try:
    if RemoveZero :
      # Set the low threshold so that it accepts zero but nothing else, the output workspace isn't used so I've set it to a workspace that I'll over write later -"Normaliser"
      FDOL = FindDetectorsOutsideLimits( InputWorkspace=TempOutput, OutputWorkspace="Normaliser",  LowThreshold=1e-200)#for usage see www.mantidproject.org/FindDetectorsOutsideLimits
##the line above doesn't work yet, a bin with zero counts would be accepted 	  
      MaskDetectors(Counts, FDOL.getPropertyValue("BadDetectorIDs"))				#for usage see www.mantidproject.org/MaskDetectors

    # --prepare to normalise the spectra against the WBV runs
    Integration(InputWorkspace=WBVanadium1, OutputWorkspace="Normaliser", RangeLower=BackWinLower, RangeUpper=BackWinUpper)#a Mantid algorithm
    if ( WBVanadium2 != "" ) :
      try:
        #--we have another white beam vanadium we'll combine it with the first white beam
        Integration(InputWorkspace=WBVanadium2, OutputWorkspace="Iwbv2", RangeLower=BackWinLower, RangeUpper=BackWinUpper)#a Mantid algorithm
        #--the equaton is (the harmonic mean) 1/av = (1/Iwbv1 + 1/Iwbv2)/2     av = 2*Iwbv1*Iwbv2/(Iwbv1 + Iwbv2)
        #--remember "Normaliser" is currently the integral of WBVanadium1. I'm re-using workspaces for memory efficiency although it makes this less clear to the eye
        Multiply("Iwbv2", "Normaliser", "numerator")								#a Mantid algorithm
        Plus("Normaliser", "Iwbv2", "Normaliser")									#for usage see www.mantidproject.org/Plus
        Divide("numerator", "Normaliser", "Normaliser")								#for usage see www.mantidproject.org/Divide
        #--don't spend time on the factor of two as it will affect all histograms equally and so not affect the results
      finally:
        mantid.deleteWorkspace("Iwbv2")
        mantid.deleteWorkspace("numerator")
    
    #--we have an integral to normalise against, lets normalise
    Divide(TempOutput, "Normaliser", TempOutput)
    
    MDT = MedianDetectorTest( InputWorkspace=TempOutput, OutputWorkspace="Normaliser", SignificanceTest=NumErrorBars, LowThreshold=0, HighThreshold=BackgroundAccept, OutputFile=MaskFile )#for usage see www.mantidproject.org/MedianDetectorTest
    #--lastly correct the experimental data that we used, if requested
    if MaskExper :
      MaskDetectors(Counts, MDT.getPropertyValue("BadDetectorIDs"))						#for usage see www.mantidproject.org/MaskDetectors

  finally:
    mantid.deleteWorkspace("Normaliser")
  
#----Start here------------------
#--Get user input
InSettings = DiagScriptInputDialog(HighAbsolute=1e8, SignificanceTest=3.3)							#for usage see www.mantidproject.org/

#--Do everything that can be done on a single white beam vanadium  
WBVanadium1 = InSettings.getPropertyValue("WBVanadium1")
HighAbsolute = InSettings.getPropertyValue("HighAbsolute")
LowAbsolute = InSettings.getPropertyValue("LowAbsolute")
HighMedian = InSettings.getPropertyValue("HighMedian")
LowMedian = InSettings.getPropertyValue("LowMedian")
NumErrorBars = InSettings.getPropertyValue("SignificanceTest")
MaskFile = InSettings.getPropertyValue("OutputFile")
try:
  #--Find the dead detectors and write them to file because this might be the only thing we do in this script.  Otherwise it will get overwritten
  DeadList = FindDeadInSingleWS( WBVanadium1, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, MaskFile )
  
  #--if there is a second white beam vanadium we'll use that
  WBVanadium2 = InSettings.getPropertyValue("WBVanadium2")
  if ( WBVanadium2 != "" ) :
    FindDeadInSingleWS( WBVanadium2, HighAbsolute, LowAbsolute, HighMedian, LowMedian, NumErrorBars, MaskFile )
    #-- again this might be the last time we look for bad detectors so will write out all we've found so far
    DEV = DetectorEfficiencyVariation( WhiteBeamBase=WBVanadium1, WhiteBeamCompare=WBVanadium2,\
      OutputWorkspace=TempOutput, Variation=InSettings.getPropertyValue("Variation"),\
      OutputFile=MaskFile )												#for usage see www.mantidproject.org/DetectorEfficiencyVariation
    DeadList = DEV.getPropertyValue("BadDetectorIDs")
  
  #--if we have a sample exerimental data run, do the background tests
  DataWS = InSettings.getPropertyValue("Experimental")
  if ( DataWS != "" ) :
    BackgroundTest( DataWS, DeadList, InSettings.getPropertyValue("BackgroundAccept"), \
	InSettings.getPropertyValue("RangeLower"), \
	InSettings.getPropertyValue("RangeUpper"), \
	InSettings.getPropertyValue("RemoveZero"), \
	InSettings.getPropertyValue("MaskExper"), \
        MaskFile )

  
finally:
  mantid.deleteWorkspace(TempOutput)
