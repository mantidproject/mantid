# First some function definitions for the Loading algorithms
def LoadData(outputarea):
	LoadRawDialog(OutputWorkspace=outputarea,message="Enter path to data file")

def LoadEmpty(outputarea):
	LoadRawDialog(OutputWorkspace=outputarea, message="Enter path to empty data")

def LoadSample(outputarea):
	LoadRawDialog(OutputWorkspace=outputarea,message="Enter path to sample file")

# ========== The script starts here ============
dataWorkspace = "Vanadium"
LoadData(dataWorkspace)
LoadEmpty("Empty")

# ==== Just want pure Vanadium so subtract the empty instrument ===
Minus(InputWorkspace1=dataWorkspace,InputWorkspace2="Empty",OutputWorkspace=dataWorkspace)
# Calculate absorption and correct for it
transWorkspace="Transmission"
CorrectForAttenuationDialog(InputWorkspace=dataWorkspace, OutputWorkspace=transWorkspace,SampleNumberDensity="0.072",ScatteringXSection="5.08",AttenuationXSection="5.1",message="Enter size parameters")
Divide(InputWorkspace1=dataWorkspace, InputWorkspace2=transWorkspace, OutputWorkspace=dataWorkspace)

# === Save as a Nexus file ===
SaveNexusProcessDialog(InputWorkspace=sampleWorkspace, message="Enter path to save Nexus file")

# === Load the sample data file ===
sampleWorkspace="Sample"
LoadSample(sampleWorkspace)

# === Align and focus the data ===
alignAlg = AlignDetectorsDialog(InputWorkspace=dataWorkspace,OutputWorkspace=dataWorkspace,message="Enter path to calibration file")
calibFile = alignAlg.getPropertyValue("CalibrationFile")
DiffractionFocussing(InputWorkspace=dataWorkspace,OutputWorkspace=dataWorkspace, GroupingFileName=calibFile)
StripPeaks(InputWorkspace=dataWorkspace, OutputWorkspace=dataWorkspace)

# === Align and focus the sample ===
AlignDetectors(InputWorkspace=sampleWorkspace,OutputWorkspace=sampleWorkspace, CalibrationFile=calibFile)
DiffractionFocussing(InputWorkspace=sampleWorkspace,OutputWorkspace=sampleWorkspace, GroupingFileName=calibFile)

# === The result is the sample corrected by the vandium ===
resultSpace="Result"
Divide(InputWorkspace1=sampleWorkspace, InputWorkspace2=dataWorkspace, OutputWorkspace=resultSpace)

