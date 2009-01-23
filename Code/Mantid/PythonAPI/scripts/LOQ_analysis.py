#LOQ data analysis script
#First draft
#######################
#Step 1 - Load the data file 
LoadDataAlg = LoadRawDialog(OutputWorkspace="Monitor",spectrummin="2",spectrummax="2")
data_file = LoadDataAlg.getPropertyValue("Filename")
#Load the small angle bank
LoadRaw(Filename = data_file, OutputWorkspace="Small_Angle",spectrummin="3",spectrummax="16386")
#Load the high angle bank
LoadRaw(Filename = data_file, OutputWorkspace="High_Angle",spectrummin="16386",spectrummax="17792")

#######################
#Step 1.1 - Move the sample if incorrect
# Coming soon....

#######################
#Step 1.2 - Correct the monitor spectrum for flat background and remove prompt spike
# Flat background correction is pending. Interpolation is just linear for now
#RemoveBins("Monitor","19900","20500")

#######################
#Step 1.3 - Mask out unwanted detetctors
MarkDeadDetectors("Small_Angle",DetectorList="126,127")
#Future Missing - Mask by volume projection (e.g. radius)

#######################
#Step 2 - Convert all of the files to wavelength and rebin
# ConvertUnits does have a rebin option, but it's crude. In particular rebins on linear scale.
ConvertUnits("Monitor","Monitor","Wavelength")
Rebin("Monitor","Monitor","2.2,-0.035,10.0")
ConvertUnits("Small_Angle","Small_Angle","Wavelength")
Rebin("Small_Angle","Small_Angle","2.2,-0.035,10.0")
ConvertUnits("High_Angle","High_Angle","Wavelength")
Rebin("High_Angle","High_Angle","2.2,-0.035,10.0")

# FROM NOW ON JUST LOOKING AT MAIN DETECTOR DATA.....

#######################
#step 2.5 remove unwanted Wavelength bins
#Remove non edge bins and linear interpolate #373
#RemoveBins("Small_Angle","1.05", "1.20")
#Future Missing - add cubic interpolation

#######################
#Step 3 - Flat cell correction

#OPTION 1
#Load flood Source file (data only for the Small angle bank)
#LoadRawDialog(OutputWorkspace="Flood_Source_Small_Angle",spectrummin="3",spectrummax="16386")
#Convert to wavelength
#Future - correct for ratio of solid angles for different positions
#Rebin to "Small_Angle" workspace
#?? Normalise the flood source file
#Divide("Small_Angle","Flood_Source_Small_Angle","Small_Angle_norm_flood_source")

#OPTION 2
CorrectToFileDialog("Small_Angle",FirstColumnValue="SpectraNumber",WorkspaceOperation="Divide",OutputWorkspace="Small_Angle")
#LoadRKH with scalar value for wavelength ranges
#data/flat(wavelength)

#OPTION 3
#Correction using instrument geometry

#######################
#Step 4 - Correct by incident beam monitor
Divide("Small_Angle","Monitor","Small_Angle")

#######################
#Step 6 - Correct by transmission
# Load the run with sample
LoadRaw("../../../Test/Data/LOQ trans configuration/LOQ48129.raw","sample")
#LoadRawDialog(OutputWorkspace="sample")
# Change the instrument definition to the correct one
LoadInstrument("sample","../../../Test/Instrument/LOQ_trans_Definition.xml")
# Need to remove prompt spike and, later, flat background
#RemoveBins("sample","sample","19900","20500","Linear")
ConvertUnits("sample","lambdaSample","Wavelength")
Rebin("lambdaSample","lambdaSample","2.2,-0.035,10.0")
# Now load and convert the direct run
LoadRaw("../../../Test/Data/LOQ trans configuration/LOQ48127.raw","direct")
#LoadRawDialog(OutputWorkspace="direct")
LoadInstrument("direct","../../../Test/Instrument/LOQ_trans_Definition.xml")
ConvertUnits("direct","lambdaDirect","Wavelength")
Rebin("lambdaDirect","lambdaDirect","2.2,-0.035,10.0")

CalculateTransmission("lambdaSample","lambdaDirect","trans")

# Now do the correction
Divide("Small_Angle","trans","Small_Angle")

#######################
#Step 7 - Correct for efficiency
CorrectToFileDialog("Small_Angle",FirstColumnValue="Wavelength",WorkspaceOperation="Divide",OutputWorkspace="Small_Angle")

#######################
#Step 8 - Rescale(detector)
# Enter the rescale value here
rescale = 1.0

#######################
#Step 10 - Correct for sample/Can volume
# Could easily move this to be done after the sample-can operation
# Enter the value here
thickness = 1.0
area = 1.0

correction = rescale/(thickness*area)
#Multiply("Small_Angle",correction,"Small_Angle")

#######################
#Step 11 - Convert to Q
#Convert units to Q (MomentumTransfer)
ConvertUnits("Small_Angle","Small_Angle","MomentumTransfer")
#rebin to desired Q bins
Rebin("Small_Angle","Small_Angle","0.008,0.002,0.3")
#Sum all spectra
SumSpectra("Small_Angle","Small_Angle")

#######################
#step 12 - Cross section (remove can scatering)
#Perform steps 1-11 for the sample and can
# sample-can

#######################
#step 12 - Save 1D data
#Missing - save as RKH format #377
