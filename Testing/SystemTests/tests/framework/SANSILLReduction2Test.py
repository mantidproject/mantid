# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import *


class ILL_SANS_D11_MONO_TEST(systemtesting.MantidSystemTest):
    """
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the old D11
    """

    def __init__(self):
        super(ILL_SANS_D11_MONO_TEST, self).__init__()
        self.setUp()
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        self.directories = config["datasearch.directories"]

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"
        config.appendDataSearchSubDir("ILL/D11/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["out", "ILL_SANS_D11_MONO.nxs"]

    def runTest(self):
        # Load a pre-drawn mask
        LoadNexusProcessed(Filename="D11_mask.nxs", OutputWorkspace="mask")
        # First reduce the water measured at 8m
        SANSILLReduction(Runs="010455", ProcessAs="DarkCurrent", NormaliseBy="Monitor", OutputWorkspace="cad")
        # Process the empty beam for water
        SANSILLReduction(
            Runs="010414",
            ProcessAs="EmptyBeam",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            OutputWorkspace="mt",
            OutputFluxWorkspace="flux",
        )
        # Water container transmission
        SANSILLReduction(
            Runs="010446",
            ProcessAs="Transmission",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            FluxWorkspace="flux",
            OutputWorkspace="wc_tr",
        )
        # Water container
        SANSILLReduction(
            Runs="010454",
            ProcessAs="EmptyContainer",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            TransmissionWorkspace="wc_tr",
            OutputWorkspace="wc",
        )
        # Water transmission
        SANSILLReduction(
            Runs="010445",
            ProcessAs="Transmission",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            FluxWorkspace="flux",
            OutputWorkspace="w_tr",
        )
        # Water
        SANSILLReduction(
            Runs="010453",
            ProcessAs="Water",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            MaskWorkspace="mask",
            EmptyBeamWorkspace="mt",
            EmptyContainerWorkspace="wc",
            TransmissionWorkspace="w_tr",
            OutputSensitivityWorkspace="sens",
            FluxWorkspace="flux",
            OutputWorkspace="water",
        )
        # The sample is measured at 20m, its transmission is measured at 8m
        # Measure the transmissions next
        # Sample container transmission
        SANSILLReduction(
            Runs="010444",
            ProcessAs="Transmission",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            FluxWorkspace="flux",
            OutputWorkspace="sc_tr",
        )
        # Sample transmission
        SANSILLReduction(
            Runs="010585",
            ProcessAs="Transmission",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            OutputWorkspace="s_tr",
            FluxWorkspace="flux",
        )
        # Reduce the sample
        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Runs="010462", ProcessAs="DarkCurrent", NormaliseBy="Monitor", OutputWorkspace="scad")
        # Process the empty beam for sample
        SANSILLReduction(
            Runs="010413",
            ProcessAs="EmptyBeam",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="scad",
            OutputWorkspace="smt",
            OutputFluxWorkspace="sflux",
        )
        # Sample container
        SANSILLReduction(
            Runs="010460",
            ProcessAs="EmptyContainer",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="scad",
            EmptyBeamWorkspace="smt",
            TransmissionWorkspace="sc_tr",
            OutputWorkspace="sc",
        )
        # Sample with flux and sensitivity
        SANSILLReduction(
            Runs="010569",
            ProcessAs="Sample",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="scad",
            EmptyContainerWorkspace="sc",
            EmptyBeamWorkspace="smt",
            SensitivityWorkspace="sens",
            MaskWorkspace="mask",
            TransmissionWorkspace="s_tr",
            OutputWorkspace="sample_sens",
            FluxWorkspace="sflux",
        )
        # Sample with flux and water normalisation
        SANSILLReduction(
            Runs="010569",
            ProcessAs="Sample",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="scad",
            EmptyContainerWorkspace="sc",
            EmptyBeamWorkspace="smt",
            FlatFieldWorkspace="water",
            MaskWorkspace="mask",
            TransmissionWorkspace="s_tr",
            OutputWorkspace="sample_water",
            FluxWorkspace="sflux",
        )
        GroupWorkspaces(InputWorkspaces=["water", "sens", "sample_sens", "sample_water"], OutputWorkspace="out")


class ILL_SANS_D22_MONO_TEST(systemtesting.MantidSystemTest):
    """
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the old D22
    """

    def __init__(self):
        super(ILL_SANS_D22_MONO_TEST, self).__init__()
        self.setUp()
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        self.directories = config["datasearch.directories"]

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D22"
        config.appendDataSearchSubDir("ILL/D22/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["out", "ILL_SANS_D22_MONO.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D22_mask.nxs", OutputWorkspace="mask")

        # Absorber
        SANSILLReduction(Runs="241238", ProcessAs="DarkCurrent", NormaliseBy="Monitor", OutputWorkspace="cad")

        # Beam
        SANSILLReduction(
            Runs="241226",
            ProcessAs="EmptyBeam",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            OutputWorkspace="mt",
            OutputFluxWorkspace="fl",
        )

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace="ctr")
        AddSampleLog(Workspace="ctr", LogName="ProcessedAs", LogText="Transmission")
        AddSampleLog(Workspace="ctr", LogName="wavelength", LogText="6.0", LogType="Number", LogUnit="Angstrom")

        # Container
        SANSILLReduction(
            Runs="241239",
            ProcessAs="EmptyContainer",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            TransmissionWorkspace="ctr",
            OutputWorkspace="can",
        )

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace="str")
        AddSampleLog(Workspace="str", LogName="ProcessedAs", LogText="Transmission")
        AddSampleLog(Workspace="str", LogName="wavelength", LogText="6.0", LogType="Number", LogUnit="Angstrom")

        # Sample
        SANSILLReduction(
            Runs="241240",
            ProcessAs="Sample",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            TransmissionWorkspace="str",
            EmptyContainerWorkspace="can",
            MaskWorkspace="mask",
            FluxWorkspace="fl",
            OutputWorkspace="out",
        )


class ILL_SANS_D22_MULTISENS(systemtesting.MantidSystemTest):
    """
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the old D22
    Tests creation of a sensitivity map without a shadow using 2 water measurements: with and w/o offset
    """

    def __init__(self):
        super(ILL_SANS_D22_MULTISENS, self).__init__()
        self.setUp()
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        self.directories = config["datasearch.directories"]

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D22"
        config.appendDataSearchSubDir("ILL/D22/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["outputs", "ILL_SANS_D22_MULTISENS.nxs"]

    def runTest(self):
        # create necessary masks:
        MaskBTP(Instrument="D22", Pixel="0-12,245-255")
        RenameWorkspace(InputWorkspace="D22MaskBTP", OutputWorkspace="top_bottom")
        MaskBTP(Instrument="D22", Tube="10-31", Pixel="105-150")
        Plus(LHSWorkspace="top_bottom", RHSWorkspace="D22MaskBTP", OutputWorkspace="mask_offset")
        MaskBTP(Instrument="D22", Tube="54-75", Pixel="108-150")
        Plus(LHSWorkspace="top_bottom", RHSWorkspace="D22MaskBTP", OutputWorkspace="mask_central")

        # Load the mask
        LoadNexusProcessed(Filename="D22_mask.nxs", OutputWorkspace="mask")

        # Absorber
        SANSILLReduction(Runs="241238", ProcessAs="DarkCurrent", NormaliseBy="Monitor", OutputWorkspace="cad")

        # Beam
        SANSILLReduction(
            Runs="241226",
            ProcessAs="EmptyBeam",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            OutputWorkspace="mt",
            OutputFluxWorkspace="fl",
        )

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace="ctr")
        AddSampleLog(Workspace="ctr", LogName="ProcessedAs", LogText="Transmission")
        AddSampleLog(Workspace="ctr", LogName="wavelength", LogText="6.0", LogType="Number", LogUnit="Ansgrom")

        # Container
        SANSILLReduction(
            Runs="241239",
            ProcessAs="EmptyContainer",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            TransmissionWorkspace="ctr",
            OutputWorkspace="can",
        )

        # Water Reference
        SANSILLReduction(
            Runs="344411",
            ProcessAs="Water",
            NormaliseBy="Monitor",
            MaskWorkspace="mask_central",
            OutputWorkspace="ref1",
            OutputSensitivityWorkspace="sens1",
        )

        SANSILLReduction(
            Runs="344407",
            ProcessAs="Water",
            NormaliseBy="Monitor",
            MaskWorkspace="mask_offset",
            OutputWorkspace="ref2",
            OutputSensitivityWorkspace="sens2",
        )

        GroupWorkspaces(InputWorkspaces=["ref1", "ref2"], OutputWorkspace="sensitivity_input")
        CalculateEfficiency(InputWorkspace="sensitivity_input", MergeGroup=True, OutputWorkspace="sens")

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace="str")
        AddSampleLog(Workspace="str", LogName="ProcessedAs", LogText="Transmission")
        AddSampleLog(Workspace="str", LogName="wavelength", LogText="6.0", LogType="Number", LogUnit="Ansgrom")

        # Sample
        SANSILLReduction(
            Runs="241240",
            ProcessAs="Sample",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            TransmissionWorkspace="str",
            EmptyContainerWorkspace="can",
            MaskWorkspace="mask",
            SensitivityWorkspace="sens",
            OutputWorkspace="out",
            FluxWorkspace="fl",
        )

        GroupWorkspaces(InputWorkspaces=["out", "sens", "ref1", "ref2"], OutputWorkspace="outputs")


class ILL_SANS_D11B_MONO_TEST(systemtesting.MantidSystemTest):
    """
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the new D11B
    It is a water measurement performed at 3 distances with 1 wavelength.
    """

    def __init__(self):
        super(ILL_SANS_D11B_MONO_TEST, self).__init__()
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
        return ["out", "ILL_SANS_D11B_MONO.nxs"]

    def runTest(self):
        cadmiums = ["8551", "8566", "8581"]
        empty_beams = ["8552", "8567", "8582"]
        tr_beam = "8538"
        can_tr = "8535"
        cans = ["8550", "8565", "8580"]
        str = "8524"
        water = ["8539", "8554", "8569"]
        edge_mask = "edge_mask_2p5m"
        masks = ["bs_mask_1p7m", "bs_mask_2p0m", "bs_mask_2p5m"]

        Load(Filename=edge_mask, OutputWorkspace=edge_mask)
        for i, mask in enumerate(masks):
            Load(Filename=mask, OutputWorkspace=mask)

        for i, cad in enumerate(cadmiums):
            SANSILLReduction(Runs=cad, ProcessAs="DarkCurrent", NormaliseBy="Monitor", OutputWorkspace=f"cad_{i}")

        SANSILLReduction(Runs=tr_beam, ProcessAs="EmptyBeam", NormaliseBy="Monitor", OutputWorkspace="trb", OutputFluxWorkspace="trfl")

        SANSILLReduction(Runs=can_tr, ProcessAs="Transmission", NormaliseBy="Monitor", FluxWorkspace="trfl", OutputWorkspace="ctr")

        SANSILLReduction(
            Runs=str, ProcessAs="Transmission", NormaliseBy="Monitor", EmptyBeamWorkspace="trb", FluxWorkspace="trfl", OutputWorkspace="str"
        )

        for i, mt in enumerate(empty_beams):
            SANSILLReduction(
                Runs=mt,
                ProcessAs="EmptyBeam",
                NormaliseBy="Monitor",
                DarkCurrentWorkspace=f"cad_{i}",
                OutputWorkspace=f"mt_{i}",
                OutputFluxWorkspace=f"fl_{i}",
            )

        for i, can in enumerate(cans):
            SANSILLReduction(
                Runs=can,
                ProcessAs="EmptyContainer",
                NormaliseBy="Monitor",
                TransmissionWorkspace="ctr",
                DarkCurrentWorkspace=f"cad_{i}",
                EmptyBeamWorkspace=f"mt_{i}",
                OutputWorkspace=f"can_{i}",
            )

        for i, wat in enumerate(water):
            SANSILLReduction(
                Runs=wat,
                ProcessAs="Water",
                NormaliseBy="Monitor",
                DefaultMaskWorkspace=edge_mask,
                MaskWorkspace=masks[i],
                DarkCurrentWorkspace=f"cad_{i}",
                EmptyBeamWorkspace=f"mt_{i}",
                FluxWorkspace=f"fl_{i}",
                TransmissionWorkspace="str",
                EmptyContainerWorkspace=f"can_{i}",
                OutputWorkspace=f"h2o_{i}",
            )

        for i in range(3):
            mtd[f"h2o_{i}"].getAxis(0).setUnit("Empty")
            CalculateDynamicRange(f"h2o_{i}")

        Q1DWeighted("h2o_0", OutputBinning="0.04,-0.02,0.54", NumberOfWedges=0, OutputWorkspace="iq0")
        Q1DWeighted("h2o_1", OutputBinning="0.03,-0.02,0.47", NumberOfWedges=0, OutputWorkspace="iq1")
        Q1DWeighted("h2o_2", OutputBinning="0.03,-0.02,0.38", NumberOfWedges=0, OutputWorkspace="iq2")
        GroupWorkspaces(InputWorkspaces=["iq0", "iq1", "iq2"], OutputWorkspace="out")


class ILL_SANS_D22B_MONO_TEST(systemtesting.MantidSystemTest):
    """
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the new D22B
    It is a water measurement normalised to a flux measured with chopper as attenuator at a different distance (but same collimation).
    This also tests the panel separation.
    """

    def __init__(self):
        super(ILL_SANS_D22B_MONO_TEST, self).__init__()
        self.setUp()
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        self.directories = config["datasearch.directories"]

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D22"
        config.appendDataSearchSubDir("ILL/D22B/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        config["datasearch.directories"] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["out", "ILL_SANS_D22B_MONO.nxs"]

    def runTest(self):
        Load(Filename="D22B_bs_mask.nxs", OutputWorkspace="bs_mask")
        Load(Filename="D22B_edge_mask.nxs", OutputWorkspace="edge_mask")

        SANSILLReduction(Runs="51690+51704", ProcessAs="DarkCurrent", NormaliseBy="Monitor", OutputWorkspace="cad")

        SANSILLReduction(
            Runs="51701",
            ProcessAs="EmptyBeam",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            OutputFluxWorkspace="fl",
            OutputWorkspace="mt",
        )

        SANSILLReduction(
            Runs="51703",
            ProcessAs="Transmission",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            FluxWorkspace="fl",
            OutputWorkspace="ctr",
        )

        SANSILLReduction(
            Runs="51727+51724+51693",
            ProcessAs="EmptyContainer",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            TransmissionWorkspace="ctr",
            OutputWorkspace="can",
        )

        SANSILLReduction(
            Runs="51702",
            ProcessAs="Transmission",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            FluxWorkspace="fl",
            OutputWorkspace="wtr",
        )

        # Empty beam for flux calculation
        SANSILLReduction(
            Runs="51644",
            ProcessAs="EmptyBeam",
            NormaliseBy="Monitor",
            DarkCurrentWorkspace="cad",
            OutputFluxWorkspace="chfl",
            OutputWorkspace="mtfl",
        )

        # note that in the original script there was also 51787 summed,
        # but it is NOT WATER, it is an empty beam!
        # adding that obviously brings the scale down to a wrong level
        SANSILLReduction(
            Runs="51726+51692+51723",
            ProcessAs="Water",
            NormaliseBy="Monitor",
            MaskWorkspace="bs_mask",
            DefaultMaskWorkspace="edge_mask",
            DarkCurrentWorkspace="cad",
            EmptyBeamWorkspace="mt",
            TransmissionWorkspace="wtr",
            EmptyContainerWorkspace="can",
            FluxWorkspace="chfl",
            OutputSensitivityWorkspace="sens",
            OutputWorkspace="water",
        )

        front = CropToComponent("water", ["detector_front"])
        back = CropToComponent("water", ["detector_back"])
        front.getAxis(0).setUnit("Empty")
        back.getAxis(0).setUnit("Empty")
        mtd["water"].getAxis(0).setUnit("Empty")
        Q1DWeighted("water", "0.005,-0.01,0.655", NumberOfWedges=0, OutputWorkspace="iq")
        Q1DWeighted("front", "0.06,-0.01,0.655", NumberOfWedges=0, OutputWorkspace="iqf")
        Q1DWeighted("back", "0.005,-0.01,0.066", NumberOfWedges=0, OutputWorkspace="iqb")
        GroupWorkspaces(InputWorkspaces=["iq", "iqf", "iqb"], OutputWorkspace="out")


class ILL_SANS_D33_MONO_TEST(systemtesting.MantidSystemTest):
    """
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the D33.
    No water normalisation is performed.
    """

    def __init__(self):
        super(ILL_SANS_D33_MONO_TEST, self).__init__()
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
        return ["iq", "ILL_SANS_D33_MONO.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D33_mask.nxs", OutputWorkspace="mask")

        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Runs="027885", NormaliseBy="Time", ProcessAs="DarkCurrent", OutputWorkspace="dcws")

        # Process the empty beam for sample
        SANSILLReduction(
            Runs="027916",
            NormaliseBy="Time",
            ProcessAs="EmptyBeam",
            DarkCurrentWorkspace="dcws",
            OutputWorkspace="ebws",
            OutputFluxWorkspace="flws",
        )

        # Process the empty beam for transmissions
        SANSILLReduction(
            Runs="027858",
            NormaliseBy="Time",
            ProcessAs="EmptyBeam",
            DarkCurrentWorkspace="dcws",
            OutputWorkspace="ebwstr",
            OutputFluxWorkspace="flwstr",
        )

        # Sample container transmission
        SANSILLReduction(
            Runs="027860",
            NormaliseBy="Time",
            ProcessAs="Transmission",
            DarkCurrentWorkspace="dcws",
            EmptyBeamWorkspace="ebwstr",
            FluxWorkspace="flwstr",
            OutputWorkspace="sc_tr",
        )

        # Sample container
        SANSILLReduction(
            Runs="027930",
            NormaliseBy="Time",
            ProcessAs="EmptyContainer",
            DarkCurrentWorkspace="dcws",
            EmptyBeamWorkspace="ebws",
            TransmissionWorkspace="sc_tr",
            OutputWorkspace="sc",
        )

        # Sample transmission
        SANSILLReduction(
            Runs="027985",
            NormaliseBy="Time",
            ProcessAs="Transmission",
            DarkCurrentWorkspace="dcws",
            EmptyBeamWorkspace="ebwstr",
            FluxWorkspace="flwstr",
            OutputWorkspace="s_tr",
        )

        # Sample
        SANSILLReduction(
            Runs="027925",
            NormaliseBy="Time",
            ProcessAs="Sample",
            MaskWorkspace="mask",
            DarkCurrentWorkspace="dcws",
            EmptyContainerWorkspace="sc",
            EmptyBeamWorkspace="ebws",
            FluxWorkspace="flws",
            TransmissionWorkspace="s_tr",
            OutputWorkspace="sample",
        )

        Q1DWeighted("sample", "0.01,-0.01,0.655", NumberOfWedges=0, OutputWorkspace="iq")


class ILL_SANS_D33_VTOF_TEST(systemtesting.MantidSystemTest):
    """
    Tests a variable binning TOF reduction with v2 of the algorithm and data from the D33.
    """

    def __init__(self):
        super(ILL_SANS_D33_VTOF_TEST, self).__init__()
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
        return ["iq", "ILL_SANS_D33_VTOF.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D33_mask.nxs", OutputWorkspace="mask")

        # Beam
        SANSILLReduction(Runs="093406", NormaliseBy="Time", ProcessAs="EmptyBeam", OutputWorkspace="beam", OutputFluxWorkspace="flux")

        # Container Transmission
        SANSILLReduction(Runs="093407", NormaliseBy="Time", ProcessAs="Transmission", FluxWorkspace="flux", OutputWorkspace="ctr")

        # Container
        SANSILLReduction(
            Runs="093409",
            NormaliseBy="Time",
            ProcessAs="EmptyContainer",
            EmptyBeamWorkspace="beam",
            TransmissionWorkspace="ctr",
            OutputWorkspace="can",
        )

        # Sample transmission
        SANSILLReduction(Runs="093408", NormaliseBy="Time", ProcessAs="Transmission", FluxWorkspace="flux", OutputWorkspace="str")

        # Sample
        SANSILLReduction(
            Runs="093410",
            ProcessAs="Sample",
            NormaliseBy="Time",
            EmptyBeamWorkspace="beam",
            TransmissionWorkspace="str",
            EmptyContainerWorkspace="can",
            MaskWorkspace="mask",
            OutputWorkspace="sample",
            FluxWorkspace="flux",
        )

        # I(Q)
        SANSILLIntegration(InputWorkspace="sample", OutputBinning="0.02,-0.1,1", OutputWorkspace="iq")


class ILL_SANS_D33_LTOF_TEST(systemtesting.MantidSystemTest):
    """
    Tests a equidistant binning TOF reduction with v2 of the algorithm and data from the D33.
    """

    def __init__(self):
        super(ILL_SANS_D33_LTOF_TEST, self).__init__()
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
        return ["iq", "ILL_SANS_D33_LTOF.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D33_mask.nxs", OutputWorkspace="mask")

        # Beam
        SANSILLReduction(Runs="093411", ProcessAs="EmptyBeam", NormaliseBy="Time", OutputWorkspace="beam", OutputFluxWorkspace="flux")

        # Container Transmission
        SANSILLReduction(Runs="093412", NormaliseBy="Time", ProcessAs="Transmission", FluxWorkspace="flux", OutputWorkspace="ctr")

        # Container
        SANSILLReduction(
            Runs="093414",
            NormaliseBy="Time",
            ProcessAs="EmptyContainer",
            EmptyBeamWorkspace="beam",
            TransmissionWorkspace="ctr",
            OutputWorkspace="can",
        )

        # Sample Transmission
        SANSILLReduction(Runs="093413", NormaliseBy="Time", ProcessAs="Transmission", FluxWorkspace="flux", OutputWorkspace="str")

        # Sample
        SANSILLReduction(
            Runs="093415",
            NormaliseBy="Time",
            ProcessAs="Sample",
            EmptyBeamWorkspace="beam",
            TransmissionWorkspace="str",
            EmptyContainerWorkspace="can",
            MaskWorkspace="mask",
            OutputWorkspace="sample",
            FluxWorkspace="flux",
        )

        # I(Q)
        SANSILLIntegration(InputWorkspace="sample", OutputBinning="0.02,-0.1,1", OutputWorkspace="iq")
