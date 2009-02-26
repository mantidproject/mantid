#LOQ data analysis script
path = "C:/MantidInstall/"
#path = "C:/Mantid/Test/"
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
#Step 1.2 - Correct the monitor spectrum for flat background and remove prompt spike
# Interpolation is just linear for now
RemoveBins("Monitor","Monitor","19900","20500",Interpolation="Linear")
FlatBackground("Monitor","Monitor","0","31000","39000")

#######################
#Step 1.3 - Mask out unwanted detetctors
# Set the radius range
min_radius = 38
max_radius = 419
# XML description of a cylinder containing detectors to remove
inner = "<infinite-cylinder id='inner'> "
inner +=	"<centre x='0.0' y='0.0' z='0.0' /> " 
inner +=	"<axis x='0.0' y='0.0' z='1' /> "
inner +=	"<radius val='"+str(min_radius/1000.0)+"' /> "
inner +=	"</infinite-cylinder> "
inner +=	"<algebra val='inner' /> "

# Remove the beam stop
MarkDeadDetectorsInShape("Small_Angle",inner)

outer = "<infinite-cylinder id='outer'> "
outer +=	"<centre x='0.0' y='0.0' z='0.0' /> " 
outer +=	"<axis x='0.0' y='0.0' z='1' /> "
outer +=	"<radius val='"+str(max_radius/1000.0)+"' /> "
outer +=	"</infinite-cylinder> "
# The hash in front of the name says that we want to keep everything inside the cylinder
outer +=	"<algebra val='#outer' /> "

# Remove the corners
dets = MarkDeadDetectorsInShape("Small_Angle",outer)

# mask right hand column
MarkDeadDetectors("Small_Angle",DetectorList=detBlock(128+2,1,128))	

#######################
#Step 1.1 - try move detector for beam centre (324.95, 328.02),
#This step has to happen after the detector masking
#RelativePosition="0" is a logical variable, which means an "absolute shift"
# the detector centr is at X=Y=0 so does relative or absolute matter ?
xshift=str((317.5-324.95)/1000.0)
yshift=str((317.5-328.02)/1000.0)
MoveInstrumentComponent("Small_Angle","main-detector-bank",X=xshift,Y=yshift,RelativePosition="1")

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

#Need to mark the workspace as a distribution at this point to get next rebin right
ws = mantid.getMatrixWorkspace("Small_Angle")
ws.isDistribution(True)

# Calculate the solid angle corrections
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
SaveRKH("Small_Angle"+wavname,Filename="Small_Angle"+wavname+".Q",FirstColumnValue="MomentumTransfer")
