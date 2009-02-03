#LOQ data analysis script
#path = "C:/MantidInstall/"
path = "C:/Mantid/Test/"
#path="C:/Documents and Settings/wmx35332/Mantid/Test/"
#LoadNexusProcessedDialog(path+"Data/LOQ sans configuration/solidAngle.nxs","solidangle")

#######################
#Step 1 - Load the data file 
LoadDataAlg = LoadRawDialog(path+"Data/LOQ sans configuration/LOQ48094.raw",OutputWorkspace="Monitor",spectrummin="2",spectrummax="2")
data_file = LoadDataAlg.getPropertyValue("Filename")
#Load the small angle bank
LoadRaw(Filename = data_file, OutputWorkspace="Small_Angle",spectrummin="3",spectrummax="16386")
#Load the high angle bank
LoadRaw(Filename = data_file, OutputWorkspace="High_Angle",spectrummin="16386",spectrummax="17792")

#######################
#Step 1.1 - Move the sample if incorrect
MoveInstrumentComponent("High_Angle","some-sample-holder",X="1",Y="1",Z="1",RelativePosition="0")

#######################
#Step 1.2 - Correct the monitor spectrum for flat background and remove prompt spike
# Missing BackgroundCorrection - for flat backgrounds #392
# Interpolation is just linear for now
RemoveBins("Monitor","Monitor","19900","20500","Linear")

#######################
#Step 1.3 - Mask out unwanted detetctors
MarkDeadDetectors("Small_Angle",DetectorList="7361,	7362,	7363,	7364,	7365,	7487,	7488,	7489,	7490,	7491,	7492,	7493,	7494,	7495,	7614,	7615,	7616,	7617,	7618,	7619,	7620,	7621,	7622,	7623,	7624,	7741,	7742,	7743,	7744,	7745,	7746,	7747,	7748,	7749,	7750,	7751,	7752,	7753,	7868,	7869,	7870,	7871,	7872,	7873,	7874,	7875,	7876,	7877,	7878,	7879,	7880,	7881,	7996,	7997,	7998,	7999,	8000,	8001,	8002,	8003,	8004,	8005,	8006,	8007,	8008,	8009,	8124,	8125,	8126,	8127,	8128,	8129,	8130,	8131,	8132,	8133,	8134,	8135,	8136,	8137,	8138,	8252,	8253,	8254,	8255,	8256,	8257,	8258,	8259,	8260,	8261,	8262,	8263,	8264,	8265,	8266,	8380,	8381,	8382,	8383,	8384,	8385,	8386,	8387,	8388,	8389,	8390,	8391,	8392,	8393,	8394,	8508,	8509,	8510,	8511,	8512,	8513,	8514,	8515,	8516,	8517,	8518,	8519,	8520,	8521,	8522,	8636,	8637,	8638,	8639,	8640,	8641,	8642,	8643,	8644,	8645,	8646,	8647,	8648,	8649,	8765,	8766,	8767,	8768,	8769,	8770,	8771,	8772,	8773,	8774,	8775,	8776,	8777,	8894,	8895,	8896,	8897,	8898,	8899,	8900,	8901,	8902,	8903,	8904,	9023,	9024,	9025,	9026,	9027,	9028,	9029,	9030,	9031,	9152,	9153,	9154,	9155,	9156,	9157")
# Missing - Mask by volume projection (e.g. radius) #383

#######################
#Step 2 - Convert all of the files to wavelength and rebin
# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
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
RemoveBins("High_Angle","High_Angle","3.00","3.50","Linear")
#Future Missing - add cubic interpolation

#######################
#Step 3 - Flat cell correction

#OPTION 1
#calculate from raw flood source file
#maybe later!

#OPTION 2
#LoadRKH with scalar value for wavelength ranges
#data/flat(wavelength)
LoadRKHDialog(path+"Data/LOQ sans configuration/FLAT_CELL.061","flat","SpectraNumber")
CropWorkspace("flat","flat",StartSpectrum="3",EndSpectrum="16386")
#optional correct for diffrernt detector positions
#SolidAngle("flat","flatsolidangle")
#SolidAngle("Small_Angle","solidangle")
#Divide("flatsolidangle","solidangle","SAcorrection")
#Divide("flat","SAcorrection","flat")
#RETRY
Divide("Small_Angle","flat","Small_Angle")

#OPTION 3
#Correction using instrument geometry
#SolidAngle("Small_Angle","solidangle")
#Divide("Small_Angle","solidangle","Small_Angle corrected by solidangle")

#######################
#Step 4 - Correct by incident beam monitor
Divide("Small_Angle","Monitor","Small_Angle")

#######################
#Step 6 - Correct by transmission
# Load the run with sample
LoadRawDialog(path+"Data/LOQ trans configuration/LOQ48129.raw","sample")
# Change the instrument definition to the correct one
LoadInstrumentDialog("sample",path+"Instrument/LOQ_trans_Definition.xml")
# Need to remove prompt spike and, later, flat background
RemoveBins("sample","sample","19900","20500","Linear")
ConvertUnits("sample","sample","Wavelength")
Rebin("sample","sample","2.2,-0.035,10.0")
# Now load and convert the direct run
LoadRawDialog(path+"Data/LOQ trans configuration/LOQ48127.raw","direct")
LoadInstrument("direct",path+"Instrument/LOQ_trans_Definition.xml")
RemoveBins("direct","direct","19900","20500","Linear")
ConvertUnits("direct","direct","Wavelength")
Rebin("direct","direct","2.2,-0.035,10.0")

CalculateTransmission("sample","direct","transmission")

# Now do the correction
Divide("Small_Angle","transmission","Small_Angle")

#######################
#Step 7 - Correct for efficiency
#CorrectToFileDialog("Small_Angle",FirstColumnValue="Wavelength",WorkspaceOperation="Divide",OutputWorkspace="Small_Angle")
CorrectToFileDialog("Small_Angle",path+"Data/LOQ sans configuration/DIRECT.041","Small_Angle","Wavelength","Divide")

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

CreateSingleValuedWorkspace("scalar",str(correction),"0.0")
Multiply("Small_Angle","scalar","Small_Angle")

#######################
#Step 11 - Convert to Q
#Convert units to Q (MomentumTransfer)
ConvertUnits("Small_Angle","Small_Angle","MomentumTransfer")
SolidAngleDialog("Small_Angle","solidangle")

#rebin to desired Q bins
Rebin("Small_Angle","Small_Angle","0.008,0.002,0.3")
RebinPreserveValue("solidangle","solidangle","0.008,0.002,0.3")
#Sum all spectra
SumSpectra("Small_Angle","Small_Angle")
SumSpectra("solidangle","solidangle")
#correct for solidangle
Divide("Small_Angle","solidangle","Small_Angle")

#######################
#step 12 - Cross section (remove can scatering)
#Perform steps 1-11 for the sample and can
# sample-can

#######################
#step 12 - Save 1D data
SaveRKHDialog("Small_Angle",FirstColumnValue="MomentumTransfer")
