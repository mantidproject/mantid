# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import numpy as np
import systemtesting
from mantid.simpleapi import *
from mantid.api import FileFinder

import os

# Expected densities for PG3AbsorptionCorrection and PG3InfoFromLogs
SI_NUMBER_DENSITY = 0.049960265513165146
SI_NUMBER_DENSITY_EFFECTIVE = 0.02498013275658258


def _skip_test():
    """Helper function to determine if we run the test"""
    import platform

    # Only runs on RHEL6 at the moment
    return "Linux" not in platform.platform()


def getSaveDir():
    """determine where to save - the current working directory"""
    return os.path.abspath(os.path.curdir)


def do_cleanup():
    Files = [
        "PG3_9829.getn",
        "PG3_9829.gsa",
        "PG3_9829.py",
        "sum_PG3_9829.gsa",
        "sum_PG3_9829.py",
        "PG3_9830.gsa",
        "PG3_9830.py",
        "PG3_4844-1.dat",
        "PG3_4844.getn",
        "PG3_4844.gsa",
        "PG3_4844.py",
        "PG3_4866.gsa",
        "PG3_46577.nxs",
        "PG3_46577.py",
        "PP_absorption_PG3_46577.nxs",
        "PP_absorption_PG3_46577.py",
    ]

    for filename in Files:
        absfile = FileFinder.getFullPath(filename)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True


class PG3Analysis(systemtesting.MantidSystemTest):
    ref_file = "PG3_4844_reference.gsa"
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2011_08_31-HR.txt"

    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        return do_cleanup()

    def requiredFiles(self):
        files = [self.ref_file, self.cal_file, self.char_file]
        files.append("PG3_4844_event.nxs")  # /SNS/PG3/IPTS-2767/0/
        files.append("PG3_4866_event.nxs")  # /SNS/PG3/IPTS-2767/0/
        files.append("PG3_5226_event.nxs")  # /SNS/PG3/IPTS-2767/0/
        return files

    def runTest(self):
        savedir = getSaveDir()

        # run the actual code
        SNSPowderReduction(
            Filename="PG3_4844",
            PreserveEvents=True,
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=self.char_file,
            LowResRef=15000,
            RemovePromptPulseWidth=50,
            Binning=-0.0004,
            BinInDspace=True,
            FilterBadPulses=95,
            SaveAs="gsas and fullprof and pdfgetn",
            OutputDirectory=savedir,
            FinalDataUnits="dSpacing",
        )

        # load output gsas file and the golden one
        LoadGSS(Filename="PG3_4844.gsa", OutputWorkspace="PG3_4844")
        LoadGSS(Filename=self.ref_file, OutputWorkspace="PG3_4844_golden")

    def validateMethod(self):
        self.tolerance = 2.0e-2
        self.tolerance_is_rel_err = True
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return ("PG3_4844", "PG3_4844_golden")


class PG3AbsorptionCorrection(systemtesting.MantidSystemTest):
    cal_file = "PG3_PAC_HR_d46168_2020_05_06.h5"
    char_files = ["PG3_char_2020_01_04_PAC_limit_1.4MW.txt", "PG3_char_2020_05_06-HighRes-PAC_1.4_MW.txt"]

    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        return do_cleanup()

    def requiredFiles(self):
        files = [self.cal_file]
        files.append(self.char_files[0])
        files.append(self.char_files[1])
        files.append("PG3_46577.nxs.h5")  # /SNS/PG3/IPTS-2767/nexus/
        files.append("PG3_46190.nxs.h5")
        files.append("PG3_46199.nxs.h5")
        files.append("PG3_46202.nxs.h5")
        return files

    def runTest(self):
        savedir = getSaveDir()

        charfile = ",".join(self.char_files)

        # Get the result without any absorption correction first
        SNSPowderReduction(
            "PG3_46577.nxs.h5",
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=charfile,
            Binning=-0.001,
            SaveAs="nexus",
            OutputDirectory=savedir,
        )

        assert not mtd["PG3_46577"].sample().getMaterial().name().strip()

        # Silicon Full Paalman-Pings test
        SNSPowderReduction(
            "PG3_46577.nxs.h5",
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=charfile,
            Binning=-0.001,
            SaveAs="nexus",
            TypeOfCorrection="FullPaalmanPings",
            SampleFormula="Si",
            MeasuredMassDensity=1.165,
            ContainerShape="PAC06",
            OutputFilePrefix="PP_absorption_",
            OutputDirectory=savedir,
        )

        # Check name, number density, effective density
        assert mtd["PG3_46577"].sample().getMaterial().name() == "Si"
        assert mtd["PG3_46577"].sample().getMaterial().numberDensity == SI_NUMBER_DENSITY
        assert mtd["PG3_46577"].sample().getMaterial().numberDensityEffective == SI_NUMBER_DENSITY_EFFECTIVE

        # Check volume using height value from log - pi*(r^2)*h, r and h in meters
        assert mtd["PG3_46577"].sample().getShape().volume() == np.pi * np.square(0.00295) * 0.040

        LoadNexus(Filename="PG3_46577.nxs", OutputWorkspace="PG3_46577")
        LoadNexus(Filename="PP_absorption_PG3_46577.nxs", OutputWorkspace="PP_46577")

        self.tolerance = 0.5
        Power(InputWorkspace="PG3_46577", OutputWorkspace="bottom", Exponent=2)
        Integration(InputWorkspace="bottom", OutputWorkspace="bottom")

        Subtract(LHSWorkspace="PG3_46577", RHSWorkspace="PP_46577", OutputWorkspace="diff")
        Power(InputWorkspace="diff", OutputWorkspace="top", Exponent=2)
        Integration(InputWorkspace="top", OutputWorkspace="top")

        Divide(LHSWorkspace="top", RHSWorkspace="bottom", OutputWorkspace="Rval")
        Rval = mtd["Rval"].dataY(0)

        self.assertLessThan(Rval, self.tolerance)


class PG3StripPeaks(systemtesting.MantidSystemTest):
    ref_file = "PG3_4866_reference.gsa"
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"

    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        return do_cleanup()

    def requiredFiles(self):
        files = [self.ref_file, self.cal_file]
        files.append("PG3_4866_event.nxs")  # vanadium
        return files

    def runTest(self):
        # determine where to save
        savedir = os.path.abspath(os.path.curdir)

        LoadEventNexus(Filename="PG3_4866_event.nxs", OutputWorkspace="PG3_4866", Precount=True)
        FilterBadPulses(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866")
        RemovePromptPulse(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Width=50)
        CompressEvents(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Tolerance=0.01)
        SortEvents(InputWorkspace="PG3_4866")
        CropWorkspace(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", XMax=16666.669999999998)
        LoadCalFile(InputWorkspace="PG3_4866", CalFilename=self.cal_file, WorkspaceName="PG3")
        MaskDetectors(Workspace="PG3_4866", MaskedWorkspace="PG3_mask")
        ApplyDiffCal(InstrumentWorkspace="PG3_4866", OffsetsWorkspace="PG3_offsets")
        ConvertUnits(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Target="dSpacing")
        ApplyDiffCal(InstrumentWorkspace="PG3_4866", ClearCalibration=True)
        ConvertUnits(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Target="TOF")
        UnwrapSNS(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", LRef=62)
        RemoveLowResTOF(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", ReferenceDIFC=1500)
        ConvertUnits(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Target="dSpacing")
        Rebin(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Params=(0.1, -0.0004, 2.2))
        SortEvents(InputWorkspace="PG3_4866")
        DiffractionFocussing(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", GroupingWorkspace="PG3_group")
        EditInstrumentGeometry(Workspace="PG3_4866", PrimaryFlightPath=60, SpectrumIDs=[1], L2=[3.2208], Polar=[90.8074], Azimuthal=[0])
        ConvertUnits(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Target="TOF")
        Rebin(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Params=[-0.0004])
        ConvertUnits(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Target="dSpacing")
        StripVanadiumPeaks(
            InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", PeakPositionTolerance=0.05, FWHM=8, BackgroundType="Quadratic"
        )
        ConvertUnits(InputWorkspace="PG3_4866", OutputWorkspace="PG3_4866", Target="TOF")
        SaveGSS(
            InputWorkspace="PG3_4866",
            Filename=os.path.join(savedir, "PG3_4866.gsa"),
            SplitFiles=False,
            Append=False,
            Format="SLOG",
            MultiplyByBinWidth=False,
            ExtendedHeader=True,
        )
        # load output gsas file and the golden one
        LoadGSS(Filename="PG3_4866.gsa", OutputWorkspace="PG3_4866")
        LoadGSS(Filename=self.ref_file, OutputWorkspace="PG3_4866_golden")

    def validateMethod(self):
        self.tolerance = 1.0e-1
        self.tolerance_is_rel_err = True
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance = 1.0e-1
        self.tolerance_is_rel_err = True
        return ("PG3_4866", "PG3_4866_golden")


class SeriesAndConjoinFilesTest(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    ref_files = ["PG3_9829_reference.gsa", "PG3_9830_reference.gsa"]
    data_files = ["PG3_9829_event.nxs", "PG3_9830_event.nxs"]

    def cleanup(self):
        return do_cleanup()

    def requiredMemoryMB(self):
        """Requires 3Gb"""
        return 3000

    def requiredFiles(self):
        files = [self.cal_file, self.char_file]
        files.extend(self.ref_files)
        files.extend(self.data_files)
        return files

    def runTest(self):
        savedir = getSaveDir()

        # reduce a series of runs
        SNSPowderReduction(
            Filename="PG3_9829,PG3_9830",
            PreserveEvents=True,
            VanadiumNumber=-1,
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=self.char_file,
            LowResRef=15000,
            RemovePromptPulseWidth=50,
            Binning=-0.0004,
            BinInDspace=True,
            FilterBadPulses=25,
            SaveAs="gsas",
            OutputDirectory=savedir,
            FinalDataUnits="dSpacing",
        )

        # needs to be set for ConjoinFiles to work
        config["default.facility"] = "SNS"
        config["default.instrument"] = "POWGEN"

        # load back in the resulting gsas files
        ConjoinFiles(RunNumbers=[9829, 9830], OutputWorkspace="ConjoinFilesTest", Directory=savedir)
        # convert units makes sure the geometry was picked up
        ConvertUnits(InputWorkspace="ConjoinFilesTest", OutputWorkspace="ConjoinFilesTest", Target="dSpacing")

        # prepare for validation
        LoadGSS(Filename="PG3_9829.gsa", OutputWorkspace="PG3_9829")
        LoadGSS(Filename=self.ref_files[0], OutputWorkspace="PG3_9829_golden")
        LoadGSS(Filename="PG3_9830.gsa", OutputWorkspace="PG3_9830")
        LoadGSS(Filename=self.ref_files[1], OutputWorkspace="PG3_9830_golden")

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        # these are an ordered pair
        return ("PG3_9829", "PG3_9829_golden", "PG3_9830", "PG3_9830_golden")


class SumFilesTest(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    ref_file = "PG3_9829_sum_reference.gsa"
    data_file = "PG3_9829_event.nxs"

    def cleanup(self):
        return True  # do_cleanup()

    def requiredMemoryMB(self):
        """Requires 3Gb"""
        return 3000

    def requiredFiles(self):
        files = [self.cal_file, self.char_file, self.ref_file, self.data_file]
        return files

    def runTest(self):
        savedir = getSaveDir()

        # reduce a sum of runs - and drop it
        SNSPowderReduction(
            Filename="PG3_9829,9830",
            Sum=True,
            OutputFilePrefix="sum_",
            PreserveEvents=True,
            VanadiumNumber=-1,
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=self.char_file,
            LowResRef=15000,
            RemovePromptPulseWidth=50,
            Binning=-0.0004,
            BinInDspace=True,
            FilterBadPulses=25,
            SaveAs="gsas",
            OutputDirectory=savedir,
            FinalDataUnits="dSpacing",
        )

        # prepare for validation
        LoadGSS(Filename="sum_PG3_9829.gsa", OutputWorkspace="PG3_9829")
        LoadGSS(Filename=self.ref_file, OutputWorkspace="PG3_9829_golden")

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return ("PG3_9829", "PG3_9829_golden")


class ToPDFgetNTest(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = "PG3_9829_event.nxs"
    getn_file = "PG3_9829.getn"

    def cleanup(self):
        return do_cleanup()

    def requiredMemoryMB(self):
        """Requires 3Gb"""
        return 3000

    def requiredFiles(self):
        files = [self.cal_file, self.char_file, self.data_file]
        return files

    def runTest(self):
        savedir = getSaveDir()
        PDToPDFgetN(
            Filename=self.data_file,
            FilterBadPulses=25,
            OutputWorkspace=self.data_file,
            PDFgetNFile=os.path.join(savedir, self.getn_file),
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=self.char_file,
            RemovePromptPulseWidth=50,
            Binning=-0.0004,
        )

    def validateMethod(self):
        return None  # it running is all that we need

    def validate(self):
        pass


class PG3InfoFromLogs(systemtesting.MantidSystemTest):
    cal_file = "PG3_PAC_HR_d46168_2020_05_06.h5"
    char_files = ["PG3_char_2020_01_04_PAC_limit_1.4MW.txt", "PG3_char_2020_05_06-HighRes-PAC_1.4_MW.txt"]

    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        return do_cleanup()

    def requiredFiles(self):
        files = [self.cal_file]
        files.append(self.char_files[0])
        files.append(self.char_files[1])
        files.append("PG3_46577.nxs.h5")  # /SNS/PG3/IPTS-2767/nexus/
        files.append("PG3_46190.nxs.h5")
        files.append("PG3_46199.nxs.h5")
        files.append("PG3_46202.nxs.h5")
        return files

    def runTest(self):
        savedir = getSaveDir()

        charfile = ",".join(self.char_files)

        # Get the result without any absorption correction first
        SNSPowderReduction(
            "PG3_46577.nxs.h5",
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=charfile,
            Binning=-0.001,
            SaveAs="nexus",
            NumWavelengthBins=1,
            OutputDirectory=savedir,
        )

        assert not mtd["PG3_46577"].sample().getMaterial().name().strip()

        # Silicon Full Paalman-Pings test
        SNSPowderReduction(
            "PG3_46577.nxs.h5",
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=charfile,
            Binning=-0.001,
            SaveAs="nexus",
            TypeOfCorrection="FullPaalmanPings",
            SampleFormula="Si",
            NumWavelengthBins=1,
            MeasuredMassDensity=1.165,
            ContainerShape="",
            OutputFilePrefix="PP_absorption_",
            OutputDirectory=savedir,
        )

        # Check name, number density, effective density
        assert mtd["PG3_46577"].sample().getMaterial().name() == "Si"
        assert mtd["PG3_46577"].sample().getMaterial().numberDensity == SI_NUMBER_DENSITY
        assert mtd["PG3_46577"].sample().getMaterial().numberDensityEffective == SI_NUMBER_DENSITY_EFFECTIVE

        # Check volume using height value from log - pi*(r^2)*h, r and h in meters
        assert mtd["PG3_46577"].sample().getShape().volume() == np.pi * np.square(0.00295) * 0.040

        # Try manually setting sample geometry, height to 2cm
        SNSPowderReduction(
            "PG3_46577.nxs.h5",
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=charfile,
            Binning=-0.001,
            SaveAs="nexus",
            TypeOfCorrection="FullPaalmanPings",
            NumWavelengthBins=1,
            SampleFormula="Si",
            SampleGeometry={"Height": 2.0},
            MeasuredMassDensity=1.165,
            ContainerShape="",
            OutputFilePrefix="PP_absorption_",
            OutputDirectory=savedir,
        )

        # Check name, number density, effective density
        assert mtd["PG3_46577"].sample().getMaterial().name() == "Si"
        assert mtd["PG3_46577"].sample().getMaterial().numberDensity == SI_NUMBER_DENSITY
        assert mtd["PG3_46577"].sample().getMaterial().numberDensityEffective == SI_NUMBER_DENSITY_EFFECTIVE

        # Check volume using height value from log - pi*(r^2)*h, r and h in meters
        assert mtd["PG3_46577"].sample().getShape().volume() == np.pi * np.square(0.00295) * 0.020


class LogarithmicTest(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    ref_file = "PG3_9829_sum_reference.gsa"
    data_file = "PG3_9829_event.nxs"
    expected_ws_name = "sns_powder_expected"

    def cleanup(self):
        return True

    def requiredMemoryMB(self):
        """Requires 2Gb"""
        return 2000

    def requiredFiles(self):
        files = [self.cal_file, self.char_file, self.ref_file, self.data_file]
        return files

    def runTest(self):
        savedir = getSaveDir()

        results = PDLoadCharacterizations(Filename=self.char_file, OutputWorkspace="characterizations")

        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            FilterBadPulses=95.0,
            OutputWorkspace=self.expected_ws_name,
            Characterizations="characterizations",
            CalFileName=self.cal_file,
            Params=0.01,
            CompressTolerance=-0.001,
            CompressBinningMode="Logarithmic",
            PrimaryFlightPath=results.PrimaryFlightPath,
            SpectrumIDs=results.SpectrumIDs,
            L2=results.L2,
            Polar=results.Polar,
            Azimuthal=results.Azimuthal,
        )
        CompressEvents(
            InputWorkspace=self.expected_ws_name, OutputWorkspace=self.expected_ws_name, Tolerance=-0.001, BinningMode="Logarithmic"
        )
        ConvertUnits(InputWorkspace=self.expected_ws_name, OutputWorkspace=self.expected_ws_name, Target="dSpacing")

        SNSPowderReduction(
            Filename="PG3_9829",
            VanadiumNumber=-1,
            CalibrationFile=self.cal_file,
            CharacterizationRunsFile=self.char_file,
            Binning=0.01,
            CompressTOFTolerance=-0.001,
            CompressBinningMode="Logarithmic",
            OutputDirectory=savedir,
            NormalizeByCurrent=False,
        )

    def validateMethod(self):
        self.tolerance = 1.0e-5
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return ("PG3_9829", self.expected_ws_name)
