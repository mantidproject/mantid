# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import systemtesting
from mantid.api import mtd, AnalysisDataService, WorkspaceFactory
from mantid.kernel import PropertyManagerDataService
from mantid.simpleapi import (
    AlignAndFocusPowder,
    AlignAndFocusPowderFromFiles,
    ConvertUnits,
    CreateGroupingWorkspace,
    CylinderAbsorption,
    DeleteWorkspace,
    Divide,
    FilterBadPulses,
    LoadDiffCal,
    LoadEventAndCompress,
    LoadEventNexus,
    NormaliseByCurrent,
    PDDetermineCharacterizations,
    PDLoadCharacterizations,
    RebinRagged,
    SetSample,
    SortEvents,
)
import numpy as np
import os
import time


def do_cleanup(cacheDir):
    if os.path.exists(cacheDir) and os.path.isdir(cacheDir):
        print("Deleting cache: {}".format(cacheDir))
        for filename in os.listdir(cacheDir):
            filename = os.path.join(cacheDir, filename)
            os.unlink(filename)
        os.rmdir(cacheDir)
    return True


def getCacheDir():
    """determine where to save - the current working directory"""
    direc = os.path.abspath(os.path.curdir)
    return os.path.join(direc, "alignandfocuscache")


class SimplestCompare(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = "PG3_9829_event.nxs"

    def cleanup(self):
        return do_cleanup(self.cacheDir)

    def requiredMemoryMB(self):
        return 3 * 1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        self.cacheDir = getCacheDir()

        PDLoadCharacterizations(
            Filename=self.char_file, OutputWorkspace="characterizations", SpectrumIDs="1", L2="3.18", Polar="90", Azimuthal="0"
        )

        self.wksp_mem = os.path.basename(self.data_file).split(".")[0]
        self.wksp_mem, self.wksp_file = self.wksp_mem + "_mem", self.wksp_mem + "_file"

        # load then process
        LoadEventAndCompress(
            Filename=self.data_file, OutputWorkspace=self.wksp_mem, MaxChunkSize=16, FilterBadPulses=0, CompressTOFTolerance=0
        )
        LoadDiffCal(Filename=self.cal_file, InputWorkspace=self.wksp_mem, WorkspaceName="PG3")
        PDDetermineCharacterizations(
            InputWorkspace=self.wksp_mem, Characterizations="characterizations", ReductionProperties="__snspowderreduction_inner"
        )
        AlignAndFocusPowder(
            InputWorkspace=self.wksp_mem,
            OutputWorkspace=self.wksp_mem,
            GroupingWorkspace="PG3_group",
            CalibrationWorkspace="PG3_cal",
            MaskWorkspace="PG3_mask",
            Params=-0.0002,
            CompressTolerance=0.01,
            PrimaryFlightPath=60,
            SpectrumIDs="1",
            L2="3.18",
            Polar="90",
            Azimuthal="0",
            ReductionProperties="__snspowderreduction_inner",
        )
        NormaliseByCurrent(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem)
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target="dSpacing")

        # everything inside the algorithm
        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.wksp_file,
            GroupingWorkspace="PG3_group",
            CalibrationWorkspace="PG3_cal",
            MaskWorkspace="PG3_mask",
            Params=-0.0002,
            CompressTolerance=0.01,
            PrimaryFlightPath=60,
            SpectrumIDs="1",
            L2="3.18",
            Polar="90",
            Azimuthal="0",
            ReductionProperties="__snspowderreduction_inner",
        )
        NormaliseByCurrent(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file)
        ConvertUnits(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file, Target="dSpacing")

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.wksp_mem, self.wksp_file)


class LogarithmicCompression(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = "PG3_9829_event.nxs"

    def cleanup(self):
        return do_cleanup(self.cacheDir)

    def requiredMemoryMB(self):
        return 3 * 1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        self.cacheDir = getCacheDir()

        PDLoadCharacterizations(
            Filename=self.char_file, OutputWorkspace="characterizations", SpectrumIDs="1", L2="3.18", Polar="90", Azimuthal="0"
        )

        self.wksp_mem = os.path.basename(self.data_file).split(".")[0]
        self.wksp_mem, self.wksp_file = self.wksp_mem + "_mem", self.wksp_mem + "_file"

        # load then process
        LoadEventNexus(Filename=self.data_file, OutputWorkspace=self.wksp_mem)
        FilterBadPulses(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, LowerCutoff=95)
        LoadDiffCal(Filename=self.cal_file, InputWorkspace=self.wksp_mem, WorkspaceName="PG3")
        PDDetermineCharacterizations(
            InputWorkspace=self.wksp_mem, Characterizations="characterizations", ReductionProperties="__snspowderreduction_inner"
        )
        AlignAndFocusPowder(
            InputWorkspace=self.wksp_mem,
            OutputWorkspace=self.wksp_mem,
            GroupingWorkspace="PG3_group",
            CalibrationWorkspace="PG3_cal",
            MaskWorkspace="PG3_mask",
            Params=-0.0002,
            CompressTolerance=-0.01,
            PrimaryFlightPath=60,
            SpectrumIDs="1",
            L2="3.18",
            Polar="90",
            Azimuthal="0",
            ReductionProperties="__snspowderreduction_inner",
        )
        NormaliseByCurrent(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem)
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target="dSpacing")
        SortEvents(self.wksp_mem)

        # everything inside the algorithm
        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.wksp_file,
            GroupingWorkspace="PG3_group",
            CalibrationWorkspace="PG3_cal",
            MaskWorkspace="PG3_mask",
            Params=-0.0002,
            CompressTolerance=0.01,
            CompressBinningMode="Logarithmic",
            FilterBadPulses=95,
            PrimaryFlightPath=60,
            SpectrumIDs="1",
            L2="3.18",
            Polar="90",
            Azimuthal="0",
            ReductionProperties="__snspowderreduction_inner",
        )
        NormaliseByCurrent(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file)
        ConvertUnits(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file, Target="dSpacing")
        SortEvents(self.wksp_file)

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.wksp_mem, self.wksp_file)


class ChunkingCompare(systemtesting.MantidSystemTest):
    # this test is very similar to SNAPRedux.Simple

    def requiredMemoryMB(self):
        return 24 * 1024  # GiB

    def runTest(self):
        GRP_WKSP = "SNAP_chnk_grouping"

        # 11MB file
        kwargs = {"Filename": "SNAP_45874", "Params": (0.5, -0.004, 7), "GroupingWorkspace": GRP_WKSP}

        # create grouping for two output spectra
        CreateGroupingWorkspace(InstrumentFilename="SNAP_Definition.xml", GroupDetectorsBy="Group", OutputWorkspace=GRP_WKSP)
        # process in 4 chunks
        AlignAndFocusPowderFromFiles(OutputWorkspace="with_chunks", MaxChunkSize=0.01, **kwargs)
        # process without chunks
        AlignAndFocusPowderFromFiles(OutputWorkspace="no_chunks", MaxChunkSize=0, **kwargs)

    def validateMethod(self):
        # self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return ("with_chunks", "no_chunks")


class CompressedCompare(systemtesting.MantidSystemTest):
    # this test is very similar to SNAPRedux.Simple

    def requiredMemoryMB(self):
        return 24 * 1024  # GiB

    def runTest(self):
        GRP_WKSP = "SNAP_compress_params"

        # 11MB file
        kwargs = {"Filename": "SNAP_45874", "Params": (0.5, -0.004, 7), "GroupingWorkspace": GRP_WKSP}

        # create grouping for two output spectra
        CreateGroupingWorkspace(InstrumentFilename="SNAP_Definition.xml", GroupDetectorsBy="Group", OutputWorkspace=GRP_WKSP)

        AlignAndFocusPowderFromFiles(
            OutputWorkspace="compress1", MaxChunkSize=0.0, CompressTolerance=1e-2, MinSizeCompressOnLoad=1e-14, **kwargs
        )

    def validateMethod(self):
        return None

    def validate(self):
        return None


class UseCache(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = "PG3_9829_event.nxs"

    def cleanup(self):
        return do_cleanup(self.cacheDir)

    def requiredMemoryMB(self):
        return 3 * 1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        self.cacheDir = getCacheDir()

        PDLoadCharacterizations(
            Filename=self.char_file, OutputWorkspace="characterizations", SpectrumIDs="1", L2="3.18", Polar="90", Azimuthal="0"
        )

        # first pass will create the cache file, second will use it
        self.wksp_make = "makecache"
        self.wksp_use = "usecache"

        # load then process
        LoadEventNexus(Filename=self.data_file, OutputWorkspace="meta", MetaDataOnly=True)
        LoadDiffCal(Filename=self.cal_file, InputWorkspace="meta", WorkspaceName="PG3")
        PDDetermineCharacterizations(
            InputWorkspace="meta", Characterizations="characterizations", ReductionProperties="__snspowderreduction_inner"
        )
        DeleteWorkspace(Workspace="meta")

        duration = {}
        for name in (self.wksp_make, self.wksp_use):
            time_start = time.time()
            AlignAndFocusPowderFromFiles(
                Filename=self.data_file,
                OutputWorkspace=name,
                CacheDir=self.cacheDir,
                GroupingWorkspace="PG3_group",
                CalibrationWorkspace="PG3_cal",
                MaskWorkspace="PG3_mask",
                Params=-0.0002,
                CompressTolerance=0.01,
                PrimaryFlightPath=60,
                SpectrumIDs="1",
                L2="3.18",
                Polar="90",
                Azimuthal="0",
                ReductionProperties="__snspowderreduction_inner",
            )
            duration[name] = time.time() - time_start
            NormaliseByCurrent(InputWorkspace=name, OutputWorkspace=name)
            ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")

        # verify that the second time is faster
        self.assertLessThan(
            duration[self.wksp_use],
            duration[self.wksp_make],
            "Should have been faster with cache {} > {}".format(duration[self.wksp_use], duration[self.wksp_make]),
        )

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.wksp_make, self.wksp_use)


class DifferentGrouping(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = "PG3_9829_event.nxs"

    def cleanup(self):
        return True  # nothing to cleanup

    def requiredMemoryMB(self):
        return 3 * 1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        self.cacheDir = getCacheDir()

        PDLoadCharacterizations(
            Filename=self.char_file, OutputWorkspace="characterizations", SpectrumIDs="1", L2="3.18", Polar="90", Azimuthal="0"
        )

        # first pass will create the cache file, second will use it
        wksp_single = "PG3_9829_single"
        wksp_banks = "PG3_9829_banks"
        grp_banks = "PG3_banks"

        # load then process
        LoadEventNexus(Filename=self.data_file, OutputWorkspace="meta", MetaDataOnly=True)
        LoadDiffCal(Filename=self.cal_file, InputWorkspace="meta", WorkspaceName="PG3")
        PDDetermineCharacterizations(
            InputWorkspace="meta", Characterizations="characterizations", ReductionProperties="__snspowderreduction_inner"
        )
        CreateGroupingWorkspace(InputWorkspace="meta", OutputWorkspace=grp_banks, GroupDetectorsBy="bank")
        DeleteWorkspace(Workspace="meta")

        for name, grouping in zip((wksp_single, wksp_banks), ("PG3_group", grp_banks)):
            print("processing {} with {}".format(name, grouping))
            AlignAndFocusPowderFromFiles(
                Filename=self.data_file,
                OutputWorkspace=name,
                GroupingWorkspace=grouping,
                CalibrationWorkspace="PG3_cal",
                MaskWorkspace="PG3_mask",
                Params=-0.0002,
                CompressTolerance=0.01,
                PrimaryFlightPath=60,
                SpectrumIDs="1",
                L2="3.18",
                Polar="90",
                Azimuthal="0",
                ReductionProperties="__snspowderreduction_inner",
            )
            NormaliseByCurrent(InputWorkspace=name, OutputWorkspace=name)
            ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="dSpacing")

        self.assertEqual(mtd[wksp_single].getNumberHistograms(), 1, "focusing into single spectrum")
        self.assertEqual(
            mtd[wksp_banks].getNumberHistograms(),
            23,
            "focusing into detector banks {}!={}".format(mtd[wksp_banks].getNumberHistograms(), 23),
        )

    def validate(self):
        pass


class AbsorptionCompare(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = "PG3_4866_event.nxs"

    def cleanup(self):
        return True

    def requiredMemoryMB(self):
        return 3 * 1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        PDLoadCharacterizations(
            Filename=self.char_file, OutputWorkspace="characterizations", SpectrumIDs="1", L2="3.18", Polar="90", Azimuthal="0"
        )

        self.wksp_mem = os.path.basename(self.data_file).split(".")[0]
        self.wksp_mem, self.wksp_file = self.wksp_mem + "_mem", self.wksp_mem + "_file"

        # load then process
        LoadEventAndCompress(
            Filename=self.data_file, OutputWorkspace=self.wksp_mem, MaxChunkSize=16, FilterBadPulses=0, CompressTOFTolerance=0
        )
        LoadDiffCal(Filename=self.cal_file, InputWorkspace=self.wksp_mem, WorkspaceName="PG3")
        PDDetermineCharacterizations(
            InputWorkspace=self.wksp_mem, Characterizations="characterizations", ReductionProperties="__snspowderreduction_inner"
        )

        # set-up the absorption calculation
        num_wl_bins = 200
        prop_manager = PropertyManagerDataService.retrieve("__snspowderreduction_inner")
        wl_min, wl_max = prop_manager["wavelength_min"].value, prop_manager["wavelength_max"].value  # 0.05, 2.20
        absorptionWS = WorkspaceFactory.create(
            mtd[self.wksp_mem], NVectors=mtd[self.wksp_mem].getNumberHistograms(), XLength=num_wl_bins + 1, YLength=num_wl_bins
        )
        xaxis = np.arange(0.0, float(num_wl_bins + 1)) * (wl_max - wl_min) / (num_wl_bins) + wl_min
        for i in range(absorptionWS.getNumberHistograms()):
            absorptionWS.setX(i, xaxis)
        absorptionWS.getAxis(0).setUnit("Wavelength")
        AnalysisDataService.addOrReplace("V_abs", absorptionWS)
        SetSample(
            InputWorkspace="V_abs",
            Material={"ChemicalFormula": "V", "SampleNumberDensity": 0.0721},
            Geometry={"Shape": "Cylinder", "Height": 6.97, "Radius": (0.63 / 2), "Center": [0.0, 0.0, 0.0]},
        )
        self.assertEqual(absorptionWS.blocksize(), num_wl_bins)
        # calculate the absorption
        CylinderAbsorption(InputWorkspace="V_abs", OutputWorkspace="V_abs", NumberOfSlices=20, NumberOfAnnuli=3)

        # do the work in memory
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target="Wavelength")
        Divide(LHSWorkspace=self.wksp_mem, RHSWorkspace="V_abs", OutputWorkspace=self.wksp_mem)
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target="TOF")
        AlignAndFocusPowder(
            InputWorkspace=self.wksp_mem,
            OutputWorkspace=self.wksp_mem,
            GroupingWorkspace="PG3_group",
            CalibrationWorkspace="PG3_cal",
            MaskWorkspace="PG3_mask",
            Params=-0.0002,
            CompressTolerance=0.01,
            PrimaryFlightPath=60,
            SpectrumIDs="1",
            L2="3.18",
            Polar="90",
            Azimuthal="0",
            ReductionProperties="__snspowderreduction_inner",
        )
        NormaliseByCurrent(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem)
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target="dSpacing")

        # everything inside the algorithm
        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.wksp_file,
            GroupingWorkspace="PG3_group",
            CalibrationWorkspace="PG3_cal",
            MaskWorkspace="PG3_mask",
            AbsorptionWorkspace="V_abs",
            Params=-0.0002,
            CompressTolerance=0.01,
            PrimaryFlightPath=60,
            SpectrumIDs="1",
            L2="3.18",
            Polar="90",
            Azimuthal="0",
            ReductionProperties="__snspowderreduction_inner",
        )
        NormaliseByCurrent(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file)
        ConvertUnits(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file, Target="dSpacing")

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        # verify the material name
        assert mtd[self.wksp_file].sample().getMaterial().name() == "V"
        # use standard method
        return (self.wksp_mem, self.wksp_file)


def checkRaggedWorkspaces(obs_name, exp_name):
    # CompareWorkspace does not support ragged workspaces
    wksp_obs = mtd[obs_name]
    wksp_exp = mtd[exp_name]

    # first check that the data arrays are the same length
    assert wksp_obs.getNumberHistograms() == wksp_exp.getNumberHistograms(), "number of histograms doesn't match"
    for i in range(wksp_exp.getNumberHistograms()):
        np.testing.assert_allclose(wksp_obs.readX(i), wksp_exp.readX(i), err_msg="x-values index={i}")
        np.testing.assert_allclose(wksp_obs.readY(i), wksp_exp.readY(i), err_msg="y-values index={i}")


class VulcanRaggedInD(systemtesting.MantidSystemTest):
    """This is identically to VulcanInD except the binning is done in d-space"""

    cal_file = "VULCAN_calibrate_2019_06_27.h5"
    data_file = "VULCAN_189186.nxs.h5"

    def requiredMemoryMB(self):
        return 3 * 1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.data_file]

    def runTest(self):
        # put together then names
        basename = os.path.basename(self.data_file).split(".")[0]
        self.wksp_obs, self.wksp_exp = basename + "_allinone", basename + "_separate"

        dmin = np.array((0.306, 0.306, 0.22))
        dmax = np.array((4.280, 4.280, 3.133))
        delta = np.array((-0.001, -0.001, -0.0003))
        tofmin, tofmax = 5000, 70000

        # wksp_exp is doing the reduction in event mode then calling RebinRagged
        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.wksp_exp,
            CalFileName=self.cal_file,
            PreserveEvents=True,
            Dspacing=True,
            Params=delta.max(),
            CompressTolerance=0.01,
            TMin=tofmin,
            TMax=tofmax,
            DMin=dmin.min(),
            DMax=dmax.max(),
        )
        ConvertUnits(InputWorkspace=self.wksp_exp, OutputWorkspace=self.wksp_exp, Target="dSpacing")
        RebinRagged(InputWorkspace=self.wksp_exp, OutputWorkspace=self.wksp_exp, XMin=dmin, XMax=dmax, Delta=delta)

        # wksp_obs is doing the reduction with RebinRagged internal
        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.wksp_obs,
            CalFileName=self.cal_file,
            PreserveEvents=True,
            Dspacing=True,
            Params=delta.max(),
            CompressTolerance=0.01,
            TMin=tofmin,
            TMax=tofmax,
            DMin=dmin,
            DMax=dmax,
            DeltaRagged=delta,
        )  # bonus bit for RebinRagged
        ConvertUnits(InputWorkspace=self.wksp_obs, OutputWorkspace=self.wksp_obs, Target="dSpacing")

    def validate(self):
        # CompareWorkspace does not support ragged workspaces
        checkRaggedWorkspaces(self.wksp_obs, self.wksp_exp)


class VulcanRaggedInTOF(systemtesting.MantidSystemTest):
    """This is identically to VulcanInD except the binning is done in time-of-flight"""

    cal_file = "VULCAN_calibrate_2019_06_27.h5"
    data_file = "VULCAN_189186.nxs.h5"

    def requiredMemoryMB(self):
        return 3 * 1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.data_file]

    def runTest(self):
        # put together then names
        basename = os.path.basename(self.data_file).split(".")[0]
        self.wksp_obs, self.wksp_exp = basename + "_allinone", basename + "_separate"

        dmin = np.array((0.306, 0.306, 0.22))
        dmax = np.array((4.280, 4.280, 3.133))
        delta = np.array((-0.001, -0.001, -0.0003))
        tofmin, tofmax = 5000, 70000

        # wksp_exp is doing the reduction in event mode then calling RebinRagged
        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.wksp_exp,
            CalFileName=self.cal_file,
            PreserveEvents=True,
            Dspacing=False,
            Params=delta.max(),
            CompressTolerance=0.01,
            TMin=tofmin,
            TMax=tofmax,
            DMin=dmin.min(),
            DMax=dmax.max(),
        )
        RebinRagged(InputWorkspace=self.wksp_exp, OutputWorkspace=self.wksp_exp, XMin=tofmin, XMax=tofmax, Delta=delta)

        # wksp_obs is doing the reduction with RebinRagged internal
        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.wksp_obs,
            CalFileName=self.cal_file,
            PreserveEvents=True,
            Dspacing=False,
            Params=delta.max(),
            CompressTolerance=0.01,
            TMin=tofmin,
            TMax=tofmax,
            DMin=dmin,
            DMax=dmax,
            DeltaRagged=delta,
        )  # bonus bit for RebinRagged

    def validate(self):
        # CompareWorkspace does not support ragged workspaces
        checkRaggedWorkspaces(self.wksp_obs, self.wksp_exp)
