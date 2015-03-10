#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

# Simply tests that our LoadRaw and LoadISISNexus algorithms produce the same workspace
class HRPDPowderDiffraction(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ["HRP39191.RAW", "hrpd_new_072_01_corr.cal", "HRP39187.RAW", 'HRPDPowderDiffraction.nxs']

    def runTest(self):
        #Load the Vanadium
        LoadRaw(Filename="HRP39191.RAW",OutputWorkspace="Vanadium")
        #mask out the vanadium peaks
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="19970",XMax="20140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="39970",XMax="40140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="59970",XMax="60140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="79970",XMax="80140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="99970",XMax="100140")
        #align vanadium detectors
        AlignDetectors(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",CalibrationFile="hrpd_new_072_01_corr.cal")
        #normalise by current
        NormaliseByCurrent(InputWorkspace="Vanadium",OutputWorkspace="Vanadium")
        #correct for solid angle
        SolidAngle(InputWorkspace="Vanadium",OutputWorkspace="Corr")
        Divide(LHSWorkspace="Vanadium",RHSWorkspace="Corr",OutputWorkspace="Vanadium")
        #Multiply the solid angle by the integrated vanadium flux between 1.4 and 3 Angstrom
        ConvertUnits(InputWorkspace="Vanadium",OutputWorkspace="flux",Target="Wavelength")
        Integration(InputWorkspace="flux",OutputWorkspace="flux",RangeLower="1.4",RangeUpper="3")
        Multiply(LHSWorkspace="Corr",RHSWorkspace="flux",OutputWorkspace="Corr")
        #adjust the correction down by a factor of 1000
        CreateSingleValuedWorkspace(OutputWorkspace="Sc",DataValue="1000")
        Divide(LHSWorkspace="Corr",RHSWorkspace="Sc",OutputWorkspace="Corr")
        #Load the Vanadium - a second time
        LoadRaw(Filename="HRP39191.RAW",OutputWorkspace="Vanadium")
        #mask out the vanadium peaks
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="19970",XMax="20140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="39970",XMax="40140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="59970",XMax="60140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="79970",XMax="80140")
        MaskBins(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",XMin="99970",XMax="100140")
        #align vanadium detectors
        AlignDetectors(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",CalibrationFile="hrpd_new_072_01_corr.cal")
        #normalise by current
        NormaliseByCurrent(InputWorkspace="Vanadium",OutputWorkspace="Vanadium")
        #correct by accumulated correction - solid angle/(1000*flux(1.4 - 3 Angstrom))
        Divide(LHSWorkspace="Vanadium",RHSWorkspace="Corr",OutputWorkspace="Vanadium")
        #Load the vanadium empty
        LoadRaw(Filename="HRP39187.RAW",OutputWorkspace="VEmpty")
        #mask out the vanadium peaks
        MaskBins(InputWorkspace="VEmpty",OutputWorkspace="VEmpty",XMin="19970",XMax="20140")
        MaskBins(InputWorkspace="VEmpty",OutputWorkspace="VEmpty",XMin="39970",XMax="40140")
        MaskBins(InputWorkspace="VEmpty",OutputWorkspace="VEmpty",XMin="59970",XMax="60140")
        MaskBins(InputWorkspace="VEmpty",OutputWorkspace="VEmpty",XMin="79970",XMax="80140")
        MaskBins(InputWorkspace="VEmpty",OutputWorkspace="VEmpty",XMin="99970",XMax="100140")
        #align vanadium empty detectors
        AlignDetectors(InputWorkspace="VEmpty",OutputWorkspace="VEmpty",CalibrationFile="hrpd_new_072_01_corr.cal")
        #correct by accumulated correction - solid angle/(1000*flux(1.4 - 3 Angstrom))
        Divide(LHSWorkspace="VEmpty",RHSWorkspace="Corr",OutputWorkspace="VEmpty")
        #normalise by current
        NormaliseByCurrent(InputWorkspace="VEmpty",OutputWorkspace="VEmpty")
        #Subtract Vanadium empty from the Vanadium
        Minus(LHSWorkspace="Vanadium",RHSWorkspace="VEmpty",OutputWorkspace="Vanadium")
        #Convert to wavelength
        ConvertUnits(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",Target="Wavelength")
        #Correct for cylinderAbsorption
        CylinderAbsorption(InputWorkspace="Vanadium",OutputWorkspace="Transmission",CylinderSampleHeight="2",
                            CylinderSampleRadius="0.4",AttenuationXSection="5.1",ScatteringXSection="5.08",
                            SampleNumberDensity="0.072",NumberOfSlices="10",NumberOfAnnuli="10",NumberOfWavelengthPoints="100")
        Divide(LHSWorkspace="Vanadium",RHSWorkspace="Transmission",OutputWorkspace="Vanadium")
        #convert to dspacing and focuss
        ConvertUnits(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",Target="dSpacing")
        DiffractionFocussing(InputWorkspace="Vanadium",OutputWorkspace="Vanadium",GroupingFileName="hrpd_new_072_01_corr.cal")

    def validate(self):
        # Fitting parameters not saved to ParameterMap
        self.disableChecking.append("Instrument")
        return 'Vanadium','HRPDPowderDiffraction.nxs'
