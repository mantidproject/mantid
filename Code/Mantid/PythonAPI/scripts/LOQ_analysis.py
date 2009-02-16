#LOQ data analysis script
path = "C:/MantidInstall/"
#path = "C:/Mantid/Test/"
#LoadNexusProcessedDialog(path+"Data/LOQ sans configuration/solidAngle.nxs","solidangle")
# this makes a list for a rectangular block (wide x high) from pixel startID [with (1,1) at bottom left in usual LOQ sense
# thouhg may need to change for SANS2d ??? ] for square array dim x dim
def detBlock(startID,wide,high,dim=128):
        output = ""
	# no idea why this says zero here rather than one ! - but it works
        for j in range(0,high):
                for i in range(0,wide):
                        output += str(startID+i+(dim*j))+","
        output = output.rstrip(",")
        return output
#
#print detBlock(4,1,5,128)
#######################
#Step 1 - Load the data file 
LoadDataAlg = LoadRaw(path+"Data/LOQ sans configuration/LOQ48098.raw",OutputWorkspace="Monitor",spectrummin="2",spectrummax="2")
data_file = LoadDataAlg.getPropertyValue("Filename")
#Load the small angle bank
# whole det is 3 to 16386, but skip first and last two rows is 130 to 16130, note the crop on flat_cell needs these numbers
firstsmall=130
lastsmall=16130
LoadRaw(Filename = data_file, OutputWorkspace="Small_Angle",spectrummin=str(firstsmall),spectrummax=str(lastsmall))
#Load the high angle bank
#LoadRaw(Filename = data_file, OutputWorkspace="High_Angle",spectrummin="16386",spectrummax="17792")

#######################
#Step 1.1 - Move the sample if incorrect
#MoveInstrumentComponent("High_Angle","some-sample-holder",X="0.001",Y="0.001",Z="0.001",RelativePosition="0")

#######################
#Step 1.2 - Correct the monitor spectrum for flat background and remove prompt spike
# Missing BackgroundCorrection - for flat backgrounds #392
# Interpolation is just linear for now
RemoveBins("Monitor","Monitor","19900","20500",Interpolation="Linear")
FlatBackground("Monitor","Monitor","0","31000","39000")

#######################
#Step 1.3 - Mask out unwanted detetctors
# list for beam stop region
MarkDeadDetectors("Small_Angle",DetectorList="7361,	7362,	7363,	7364,	7365,	7487,	7488,	7489,	7490,	7491,	7492,	7493,	7494,	7495,	7614,	7615,	7616,	7617,	7618,	7619,	7620,	7621,	7622,	7623,	7624,	7741,	7742,	7743,	7744,	7745,	7746,	7747,	7748,	7749,	7750,	7751,	7752,	7753,	7868,	7869,	7870,	7871,	7872,	7873,	7874,	7875,	7876,	7877,	7878,	7879,	7880,	7881,	7996,	7997,	7998,	7999,	8000,	8001,	8002,	8003,	8004,	8005,	8006,	8007,	8008,	8009,	8124,	8125,	8126,	8127,	8128,	8129,	8130,	8131,	8132,	8133,	8134,	8135,	8136,	8137,	8138,	8252,	8253,	8254,	8255,	8256,	8257,	8258,	8259,	8260,	8261,	8262,	8263,	8264,	8265,	8266,	8380,	8381,	8382,	8383,	8384,	8385,	8386,	8387,	8388,	8389,	8390,	8391,	8392,	8393,	8394,	8508,	8509,	8510,	8511,	8512,	8513,	8514,	8515,	8516,	8517,	8518,	8519,	8520,	8521,	8522,	8636,	8637,	8638,	8639,	8640,	8641,	8642,	8643,	8644,	8645,	8646,	8647,	8648,	8649,	8765,	8766,	8767,	8768,	8769,	8770,	8771,	8772,	8773,	8774,	8775,	8776,	8777,	8894,	8895,	8896,	8897,	8898,	8899,	8900,	8901,	8902,	8903,	8904,	9023,	9024,	9025,	9026,	9027,	9028,	9029,	9030,	9031,	9152,	9153,	9154,	9155,	9156,	9157")
# now need take off some corners 
MarkDeadDetectors("Small_Angle",DetectorList=detBlock(1+2,8,8))	
MarkDeadDetectors("Small_Angle",DetectorList=detBlock(121+2,8,8))	
MarkDeadDetectors("Small_Angle",DetectorList=detBlock(120*128+1+2,8,8))	
MarkDeadDetectors("Small_Angle",DetectorList=detBlock(120*128+121+2,8,8))	
# mask right hand column
MarkDeadDetectors("Small_Angle",DetectorList=detBlock(128+2,1,128))	
# Missing - Mask by volume projection (e.g. radius) #383

#######################
#Step 2 - Convert all of the files to wavelength and rebin
# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
wav1=2.2
wav2=10.0
dwav=-0.035
wavbin=str(wav1)+","+str(dwav)+","+str(wav2)
wavname="_"+str(int(wav1))+"_"+str(int(wav2))
#
ConvertUnits("Monitor","Monitor","Wavelength")
Rebin("Monitor","Monitor",wavbin)
ConvertUnits("Small_Angle","Small_Angle","Wavelength")
Rebin("Small_Angle","Small_Angle",wavbin)
#ConvertUnits("High_Angle","High_Angle","Wavelength")
#Rebin("High_Angle","High_Angle",wavbin)

# FROM NOW ON JUST LOOKING AT MAIN DETECTOR DATA.....

#######################
#step 2.5 remove unwanted Wavelength bins
#Remove non edge bins and linear interpolate #373
#RemoveBins("High_Angle","High_Angle","3.00","3.50",Interpolation="Linear")
#Future Missing - add cubic interpolation

#######################
#Step 3 - Flat cell correction

#OPTION 1
#calculate from raw flood source file
#maybe later!

#OPTION 2
#LoadRKH with scalar value for wavelength ranges
#data/flat(wavelength)
#LoadRKH(path+"Data/LOQ sans configuration/FLAT_CELL.061","flat","SpectraNumber")
#CropWorkspace("flat","flat",StartSpectrum=str(firstsmall),EndSpectrum=str(lastsmall))
#optional correct for diffrernt detector positions
#SolidAngle("flat","flatsolidangle")
#SolidAngle("Small_Angle","solidangle")
#Divide("flatsolidangle","solidangle","SAcorrection")
#Divide("flat","SAcorrection","flat")
#RETRY
#Divide("Small_Angle","flat","Small_Angle")

#OPTION 3
#Correction using instrument geometry
#SolidAngle("Small_Angle","solidangle")
#Divide("Small_Angle","solidangle","Small_Angle corrected by solidangle")

#######################
#Step 4 - Correct by incident beam monitor
Divide("Small_Angle","Monitor","Small_Angle")
mantid.deleteWorkspace("Monitor")

#######################
#Step 6 - Correct by transmission
# Load the run with sample
#LoadRawDialog(path+"Data/LOQ48130.raw","sample")
LoadRaw(Filename=path+"Data/LOQ trans configuration/LOQ48130.raw",OutputWorkspace="sample")
# Change the instrument definition to the correct one
LoadInstrument("sample",path+"Instrument/LOQ_trans_Definition.xml")
# Need to remove prompt spike and, later, flat background
RemoveBins("sample","sample","19900","20500",Interpolation="Linear")
FlatBackground("sample","sample","1,2","31000","39000")
ConvertUnits("sample","sample","Wavelength")
Rebin("sample","sample",wavbin)
# Now load and convert the direct run
#LoadRawDialog(path+"Data/LOQ48127.raw","direct")
LoadRaw(Filename=path+"Data/LOQ trans configuration/LOQ48127.raw",OutputWorkspace="direct")
LoadInstrument("direct",path+"Instrument/LOQ_trans_Definition.xml")
RemoveBins("direct","direct","19900","20500",Interpolation="Linear")
FlatBackground("direct","direct","1,2","31000","39000")
ConvertUnits("direct","direct","Wavelength")
Rebin("direct","direct",wavbin)

CalculateTransmission("sample","direct","transmission")
mantid.deleteWorkspace("sample")
mantid.deleteWorkspace("direct")

# Now do the correction
Divide("Small_Angle","transmission","Small_Angle")
mantid.deleteWorkspace("transmission")

#######################
#Step 7 - Correct for efficiency
CorrectToFile("Small_Angle",path+"Data/LOQ sans configuration/DIRECT.041","Small_Angle","Wavelength","Divide")

#######################
#Step 8 - Rescale(detector)
# Enter the rescale value here
rescale = 1.508*100.0

#######################
#Step 10 - Correct for sample/Can volume
# Could easily move this to be done after the sample-can operation
# Enter the value here
thickness = 1.0
area = 3.1414956*8*8/4

correction = rescale/(thickness*area)

CreateSingleValuedWorkspace("scalar",str(correction),"0.0")
Multiply("Small_Angle","scalar","Small_Angle")
mantid.deleteWorkspace("scalar")

#######################
#Step 11 - Convert to Q
#Convert units to Q (MomentumTransfer)
ConvertUnits("Small_Angle","Small_Angle","MomentumTransfer")
# remove Dialog to stoip the pop up boxes
SolidAngle("Small_Angle","solidangle")

#rebin to desired Q bins
Rebin("Small_Angle","Small_Angle","0.008,0.002,0.28")
RebinPreserveValue("solidangle","solidangle","0.008,0.002,0.28")
#Sum all spectra
SumSpectra("Small_Angle","Small_Angle"+wavname)
SumSpectra("solidangle","solidangle"+wavname)
#correct for solidangle
Divide("Small_Angle"+wavname,"solidangle"+wavname,"Small_Angle"+wavname)

#######################
#step 12 - Cross section (remove can scatering)
#Perform steps 1-11 for the sample and can
# sample-can

#######################
#step 12 - Save 1D data
# this writes to the /bin directory - why ?
#SaveRKHDialog("Small_Angle"+wavname,FirstColumnValue="MomentumTransfer")
SaveRKH("Small_Angle"+wavname,Filename="Small_Angle"+wavname+".Q",FirstColumnValue="MomentumTransfer")
