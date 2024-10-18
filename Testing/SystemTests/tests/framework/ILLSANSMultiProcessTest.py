# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import SANSILLMultiProcess


class ILL_SANS_D11B_MONO_MULTI_TEST(systemtesting.MantidSystemTest):
    """
    Tests a standard monochromatic reduction with the multiprocess algorithm for the new D11B data.
    Tests 2 samples (H2O and D2O) measured at 3 distances with the same wavelength.
    Tests also getting the sample names from the nexus files.
    """

    def __init__(self):
        super(ILL_SANS_D11B_MONO_MULTI_TEST, self).__init__()
        self.setUp()
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        self.directories = config["datasearch.directories"]

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"
        config.appendDataSearchSubDir("ILL/D11B/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["water", "ILL_SANS_D11B_MONO_MULTI.nxs"]

    def runTest(self):
        cadmiums = ["8551", "8566", "8581"]
        empty_beams = ["8552", "8567", "8582"]
        tr_beam = "8538"
        can_tr = "8535"
        cans = ["8550", "8565", "8580"]
        edge_mask = "edge_mask_2p5m"
        masks = ["bs_mask_1p7m", "bs_mask_2p0m", "bs_mask_2p5m"]

        samplesD1 = "8539,8549"
        samplesD2 = "8554,8564"
        samplesD3 = "8569,8579"
        sampleTrs = "8524,8534"

        SANSILLMultiProcess(
            SampleRunsD1=samplesD1,
            SampleRunsD2=samplesD2,
            SampleRunsD3=samplesD3,
            SampleTrRunsW1=sampleTrs,
            NormaliseBy="Monitor",
            EmptyBeamRuns=",".join(empty_beams),
            DarkCurrentRuns=",".join(cadmiums),
            TrEmptyBeamRuns=tr_beam,
            ContainerTrRuns=can_tr,
            EmptyContainerRuns=",".join(cans),
            DefaultMask=edge_mask,
            BeamStopMasks=",".join(masks),
            SampleNamesFrom="Nexus",
            OutputWorkspace="water",
        )


class ILL_SANS_D11B_KINE_MULTI_TEST(systemtesting.MantidSystemTest):
    """
    Tests a kinetic data reduction with the multiprocess algorithm for the new D11B data.
    Tests 3 samples measured with 85 frames at 3 distances with the same wavelength.
    Tests also repeated transmissions (non-kinetic) and summed numors input.
    """

    def __init__(self):
        super(ILL_SANS_D11B_KINE_MULTI_TEST, self).__init__()
        self.setUp()
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        self.directories = config["datasearch.directories"]

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"
        config.appendDataSearchSubDir("ILL/D11B/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["out", "ILL_SANS_D11B_KINE_MULTI.nxs"]

    def runTest(self):
        cads = ["17285", "17284", "17082"]
        trbeam = "17097"
        emptybeams = ["17112", "17097", "17098"]
        cans = ["17109", "17095", "17099"]
        ctr = "17096"
        masks = ["mask3m", "mask_8m", "mask_34m"]
        flat_fields = ["flat_field_3m", "flat_field_8m", "flat_field_8m"]
        edge = "edge_mask"

        sampleTr = ["17162", "17166", "17166"]
        samplesD1 = ["17251", "17177", "17257"]
        samplesD2 = ["17192", "17194", "17200"]
        samplesD3 = ["17180+17184", "17182+17186", "17208+17212"]

        SANSILLMultiProcess(
            SampleRunsD1=",".join(samplesD1),
            SampleRunsD2=",".join(samplesD2),
            SampleRunsD3=",".join(samplesD3),
            DarkCurrentRuns=",".join(cads),
            EmptyBeamRuns=",".join(emptybeams),
            EmptyContainerRuns=",".join(cans),
            SampleTrRunsW1=",".join(sampleTr),
            ContainerTrRuns=ctr,
            TrEmptyBeamRuns=trbeam,
            TrDarkCurrentRuns=cads[1],
            DefaultMask=edge,
            BeamStopMasks=",".join(masks),
            FlatFields=",".join(flat_fields),
            NormaliseBy="Time",
            CalculateResolution="DirectBeam",
            BinningFactor=2,
            OutputWorkspace="out",
        )


class ILL_SANS_D33_MONO_MULTI_TEST(systemtesting.MantidSystemTest):
    """
    Tests a mono data reduction with the multiprocess algorithm for D33 data.
    """

    def __init__(self):
        super(ILL_SANS_D33_MONO_MULTI_TEST, self).__init__()
        self.setUp()
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        self.directories = config["datasearch.directories"]

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D33"
        config.appendDataSearchSubDir("ILL/D33/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["out", "ILL_SANS_D33_MONO_MULTI.nxs"]

    def runTest(self):
        SANSILLMultiProcess(
            SampleRunsD1="027925",
            NormaliseBy="Time",
            DarkCurrentRuns="027885",
            EmptyBeamRuns="027916",
            TrEmptyBeamRuns="027858",
            ContainerTrRuns="027860",
            EmptyContainerRuns="027930",
            SampleTrRunsW1="027985",
            DefaultMask="D33_mask.nxs",
            OutputWorkspace="out",
        )
