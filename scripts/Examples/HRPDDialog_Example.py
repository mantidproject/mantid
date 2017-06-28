# pylint: disable=invalid-name
# First a function definition for the Loading algorithms which loads the data and immediately aligns the detectors
import mantid.simpleapi as mantid


def LoadAndAlign(_type, outputArea):
    mantid.LoadRawDialog(OutputWorkspace=outputArea, Message="Enter path to the file containing the " + _type)
    mantid.AlignDetectorsDialog(InputWorkspace=outputArea, OutputWorkspace=outputArea,
                                Message="Enter path to calibration file")


# ========== The script starts here ============
dataWorkspace = "Vanadium"
LoadAndAlign("data", dataWorkspace)
LoadAndAlign("empty run", "Empty")

# ==== Just want pure Vanadium so subtract the empty instrument ===
mantid.Minus(LHSWorkspace=dataWorkspace, RHSWorkspace="Empty", OutputWorkspace=dataWorkspace)
# Calculate absorption and correct for it
transWorkspace = "Transmission"
# The input workspace needs to be in units of wavelength for the CylinderAbsorption algorithm
mantid.ConvertUnits(InputWorkspace=dataWorkspace, OutputWorkspace=dataWorkspace, Target="Wavelength")
mantid.CylinderAbsorptionDialog(InputWorkspace=dataWorkspace, OutputWorkspace=transWorkspace,
                                SampleNumberDensity="0.072",
                                ScatteringXSection="5.08", AttenuationXSection="5.1", Message="Enter size parameters")
mantid.Divide(dataWorkspace, transWorkspace, OutputWorkspace=dataWorkspace)

# === Save as a Nexus file ===
# SaveNexusProcessDialog(InputWorkspace=sampleWorkspace, message="Enter path to save Nexus file")

# === Load the sample data file ===
sampleWorkspace = "Sample"
LoadAndAlign("sample", sampleWorkspace)

# === Focus the data ===
mantid.ConvertUnits(dataWorkspace, dataWorkspace, 'dSpacing')
alg = mantid.DiffractionFocussingDialog(InputWorkspace=dataWorkspace, OutputWorkspace=dataWorkspace)
calibFile = alg.getPropertyValue("GroupingFileName")
mantid.StripPeaks(InputWorkspace=dataWorkspace, OutputWorkspace=dataWorkspace)

# === Focus the sample ===
mantid.DiffractionFocussing(InputWorkspace=sampleWorkspace, OutputWorkspace=sampleWorkspace, GroupingFileName=calibFile)

# === The result is the sample corrected by the vandium ===
resultSpace = "Result"
mantid.Divide(sampleWorkspace, dataWorkspace, OutputWorkspace=resultSpace)
