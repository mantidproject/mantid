# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import *


class ILL_D11_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILL_D11_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"
        config.appendDataSearchSubDir("ILL/D11/")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["iq", "ILL_SANS_D11_IQ.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D11_mask.nxs", OutputWorkspace="mask")
        # Process the dark current Cd/B4C for water
        SANSILLReduction(Run="010455", ProcessAs="Absorber", OutputWorkspace="Cdw", Version=1)
        # Process the empty beam for water
        SANSILLReduction(Run="010414", ProcessAs="Beam", AbsorberInputWorkspace="Cdw", OutputWorkspace="Dbw", Version=1)
        # Water container transmission
        SANSILLReduction(
            Run="010446",
            ProcessAs="Transmission",
            AbsorberInputWorkspace="Cdw",
            BeamInputWorkspace="Dbw",
            OutputWorkspace="wc_tr",
            Version=1,
        )
        # Water container
        SANSILLReduction(
            Run="010454",
            ProcessAs="Container",
            AbsorberInputWorkspace="Cdw",
            BeamInputWorkspace="Dbw",
            TransmissionInputWorkspace="wc_tr",
            OutputWorkspace="wc",
            Version=1,
        )
        # Water transmission
        SANSILLReduction(
            Run="010445",
            ProcessAs="Transmission",
            AbsorberInputWorkspace="Cdw",
            BeamInputWorkspace="Dbw",
            OutputWorkspace="w_tr",
            Version=1,
        )
        # Water
        SANSILLReduction(
            Run="010453",
            ProcessAs="Sample",
            AbsorberInputWorkspace="Cdw",
            MaskedInputWorkspace="mask",
            ContainerInputWorkspace="wc",
            BeamInputWorkspace="Dbw",
            TransmissionInputWorkspace="wc_tr",
            SensitivityOutputWorkspace="sens",
            OutputWorkspace="water",
            Version=1,
        )
        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Run="010462", ProcessAs="Absorber", OutputWorkspace="Cd", Version=1)
        # Process the empty beam for sample
        SANSILLReduction(
            Run="010413", ProcessAs="Beam", AbsorberInputWorkspace="Cd", OutputWorkspace="Db", FluxOutputWorkspace="fl", Version=1
        )
        # Sample container transmission
        SANSILLReduction(
            Run="010444",
            ProcessAs="Transmission",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Dbw",
            OutputWorkspace="sc_tr",
            Version=1,
        )
        # Sample container
        SANSILLReduction(
            Run="010460",
            ProcessAs="Container",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Db",
            TransmissionInputWorkspace="sc_tr",
            OutputWorkspace="sc",
            Version=1,
        )
        # Sample transmission
        SANSILLReduction(
            Run="010585", ProcessAs="Transmission", AbsorberInputWorkspace="Cd", BeamInputWorkspace="Dbw", OutputWorkspace="s_tr", Version=1
        )
        # Sample
        SANSILLReduction(
            Run="010569",
            ProcessAs="Sample",
            AbsorberInputWorkspace="Cd",
            ContainerInputWorkspace="sc",
            BeamInputWorkspace="Db",
            SensitivityInputWorkspace="sens",
            MaskedInputWorkspace="mask",
            TransmissionInputWorkspace="s_tr",
            OutputWorkspace="sample_flux",
            FluxInputWorkspace="fl",
            Version=1,
        )
        # Convert to I(Q)
        SANSILLIntegration(InputWorkspace="sample_flux", OutputWorkspace="iq", CalculateResolution="MildnerCarpenter")


class ILL_D22_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILL_D22_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D22"
        config.appendDataSearchSubDir("ILL/D22/")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["iq", "ILL_SANS_D22_IQ_v2.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D22_mask.nxs", OutputWorkspace="mask")

        # Absorber
        SANSILLReduction(Run="241238", ProcessAs="Absorber", OutputWorkspace="Cd", Version=1)

        # Beam
        SANSILLReduction(
            Run="241226", ProcessAs="Beam", AbsorberInputWorkspace="Cd", OutputWorkspace="Db", FluxOutputWorkspace="fl", Version=1
        )

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace="ctr")
        AddSampleLog(Workspace="ctr", LogName="ProcessedAs", LogText="Transmission", Version=1)

        # Container
        SANSILLReduction(
            Run="241239",
            ProcessAs="Container",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Db",
            TransmissionInputWorkspace="ctr",
            OutputWorkspace="can",
            Version=1,
        )

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace="str")
        AddSampleLog(Workspace="str", LogName="ProcessedAs", LogText="Transmission", Version=1)

        # Reference
        # Actually this is not water, but a deuterated buffer, but is fine for the test
        SANSILLReduction(
            Run="241261",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Db",
            ContainerInputWorkspace="can",
            OutputWorkspace="ref",
            SensitivityOutputWorkspace="sens",
            Version=1,
        )

        # remove the errors on the sensitivity, since they are too large because it is not water
        CreateWorkspace(
            DataX=mtd["sens"].extractX(),
            DataY=mtd["sens"].extractY(),
            NSpec=mtd["sens"].getNumberHistograms(),
            OutputWorkspace="sens",
            ParentWorkspace="sens",
        )
        AddSampleLog(Workspace="sens", LogName="ProcessedAs", LogText="Reference")

        # Sample
        SANSILLReduction(
            Run="241240",
            ProcessAs="Sample",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Db",
            TransmissionInputWorkspace="str",
            ContainerInputWorkspace="can",
            MaskedInputWorkspace="mask",
            SensitivityInputWorkspace="sens",
            OutputWorkspace="sample",
            FluxInputWorkspace="fl",
            Version=1,
        )

        # Integration
        SANSILLIntegration(InputWorkspace="sample", OutputWorkspace="iq")


class ILL_D22_Multiple_Sensitivity_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILL_D22_Multiple_Sensitivity_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D22"
        config.appendDataSearchSubDir("ILL/D22/")

        # create necessary masks:
        MaskBTP(Instrument="D22", Pixel="0-12,245-255")
        RenameWorkspace(InputWorkspace="D22MaskBTP", OutputWorkspace="top_bottom")
        MaskBTP(Instrument="D22", Tube="10-31", Pixel="105-150")
        Plus(LHSWorkspace="top_bottom", RHSWorkspace="D22MaskBTP", OutputWorkspace="mask_offset")
        MaskBTP(Instrument="D22", Tube="54-75", Pixel="108-150")
        Plus(LHSWorkspace="top_bottom", RHSWorkspace="D22MaskBTP", OutputWorkspace="mask_central")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        self.disableChecking = ["Instrument"]
        return ["iq_multi_sens", "ILL_SANS_D22_IQ_multi_sens.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D22_mask.nxs", OutputWorkspace="mask")

        # Absorber
        SANSILLReduction(Run="241238", ProcessAs="Absorber", OutputWorkspace="Cd", Version=1)

        # Beam
        SANSILLReduction(
            Run="241226", ProcessAs="Beam", AbsorberInputWorkspace="Cd", OutputWorkspace="Db", FluxOutputWorkspace="fl", Version=1
        )

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace="ctr")
        AddSampleLog(Workspace="ctr", LogName="ProcessedAs", LogText="Transmission", Version=1)

        # Container
        SANSILLReduction(
            Run="241239",
            ProcessAs="Container",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Db",
            TransmissionInputWorkspace="ctr",
            OutputWorkspace="can",
            Version=1,
        )

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace="str")
        AddSampleLog(Workspace="str", LogName="ProcessedAs", LogText="Transmission", Version=1)

        # Reference
        SANSILLReduction(
            Run="344411",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask_central",
            OutputWorkspace="ref1",
            SensitivityOutputWorkspace="sens1",
            Version=1,
        )

        SANSILLReduction(
            Run="344407",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask_offset",
            OutputWorkspace="ref2",
            SensitivityOutputWorkspace="sens2",
            Version=1,
        )

        GroupWorkspaces(InputWorkspaces=["ref1", "ref2"], OutputWorkspace="sensitivity_input")
        CalculateEfficiency(InputWorkspace="sensitivity_input", MergeGroup=True, OutputWorkspace="sens")

        AddSampleLog(Workspace="sens", LogName="ProcessedAs", LogText="Reference")

        # Sample
        SANSILLReduction(
            Run="241240",
            ProcessAs="Sample",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Db",
            TransmissionInputWorkspace="str",
            ContainerInputWorkspace="can",
            MaskedInputWorkspace="mask",
            SensitivityInputWorkspace="sens",
            OutputWorkspace="sample",
            FluxInputWorkspace="fl",
            Version=1,
        )

        # Integration
        SANSILLIntegration(InputWorkspace="sample", OutputWorkspace="iq_multi_sens")


class ILL_D33_VTOF_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILL_D33_VTOF_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D33"
        config.appendDataSearchSubDir("ILL/D33/")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["iq", "ILL_SANS_D33_VTOF_IQ.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D33_mask.nxs", OutputWorkspace="mask")

        # Beam
        SANSILLReduction(Run="093406", ProcessAs="Beam", OutputWorkspace="beam", FluxOutputWorkspace="flux", Version=1)

        # Container Transmission
        SANSILLReduction(Run="093407", ProcessAs="Transmission", BeamInputWorkspace="beam", OutputWorkspace="ctr", Version=1)

        # Container
        SANSILLReduction(
            Run="093409",
            ProcessAs="Container",
            BeamInputWorkspace="beam",
            TransmissionInputWorkspace="ctr",
            OutputWorkspace="can",
            Version=1,
        )

        # Sample transmission
        SANSILLReduction(Run="093408", ProcessAs="Transmission", BeamInputWorkspace="beam", OutputWorkspace="str", Version=1)

        # Sample
        SANSILLReduction(
            Run="093410",
            ProcessAs="Sample",
            BeamInputWorkspace="beam",
            TransmissionInputWorkspace="str",
            ContainerInputWorkspace="can",
            MaskedInputWorkspace="mask",
            OutputWorkspace="sample",
            FluxInputWorkspace="flux",
            Version=1,
        )
        # I(Q)
        SANSILLIntegration(InputWorkspace="sample", OutputBinning="0.005,-0.1,1", OutputWorkspace="iq")


class ILL_D33_LTOF_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILL_D33_LTOF_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D33"
        config.appendDataSearchSubDir("ILL/D33/")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        self.nanEqual = True
        return ["iq", "ILL_SANS_D33_LTOF_IQ.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D33_mask.nxs", OutputWorkspace="mask")

        # Beam
        SANSILLReduction(Run="093411", ProcessAs="Beam", OutputWorkspace="beam", FluxOutputWorkspace="flux", Version=1)

        # Container Transmission
        SANSILLReduction(Run="093412", ProcessAs="Transmission", BeamInputWorkspace="beam", OutputWorkspace="ctr", Version=1)

        # Container
        SANSILLReduction(
            Run="093414",
            ProcessAs="Container",
            BeamInputWorkspace="beam",
            TransmissionInputWorkspace="ctr",
            OutputWorkspace="can",
            Version=1,
        )

        # Sample Transmission
        SANSILLReduction(Run="093413", ProcessAs="Transmission", BeamInputWorkspace="beam", OutputWorkspace="str", Version=1)

        # Sample
        SANSILLReduction(
            Run="093415",
            ProcessAs="Sample",
            BeamInputWorkspace="beam",
            TransmissionInputWorkspace="str",
            ContainerInputWorkspace="can",
            MaskedInputWorkspace="mask",
            OutputWorkspace="sample",
            FluxInputWorkspace="flux",
            Version=1,
        )

        # I(Q)
        SANSILLIntegration(InputWorkspace="sample", OutputBinning="0.005,-0.1,1", OutputWorkspace="iq")


class ILL_D33_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(ILL_D33_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D33"
        config.appendDataSearchSubDir("ILL/D33/")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["iq", "ILL_SANS_D33_IQ.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D33_mask.nxs", OutputWorkspace="mask")

        # Process the dark current Cd/B4C for water
        SANSILLReduction(Run="027885", ProcessAs="Absorber", OutputWorkspace="Cdw", Version=1)

        # Process the empty beam for water
        SANSILLReduction(Run="027858", ProcessAs="Beam", AbsorberInputWorkspace="Cdw", OutputWorkspace="Dbw", Version=1)

        # Water container transmission
        SANSILLReduction(
            Run="027860",
            ProcessAs="Transmission",
            AbsorberInputWorkspace="Cdw",
            BeamInputWorkspace="Dbw",
            OutputWorkspace="wc_tr",
            Version=1,
        )

        # Water container
        SANSILLReduction(
            Run="027886",
            ProcessAs="Container",
            AbsorberInputWorkspace="Cdw",
            BeamInputWorkspace="Dbw",
            TransmissionInputWorkspace="wc_tr",
            OutputWorkspace="wc",
            Version=1,
        )

        # Water transmission
        SANSILLReduction(
            Run="027861",
            ProcessAs="Transmission",
            AbsorberInputWorkspace="Cdw",
            BeamInputWorkspace="Dbw",
            OutputWorkspace="w_tr",
            Version=1,
        )

        # Water
        SANSILLReduction(
            Run="027887",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            AbsorberInputWorkspace="Cdw",
            ContainerInputWorkspace="wc",
            BeamInputWorkspace="Dbw",
            TransmissionInputWorkspace="wc_tr",
            SensitivityOutputWorkspace="sens",
            OutputWorkspace="water",
            Version=1,
        )

        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Run="027885", ProcessAs="Absorber", OutputWorkspace="Cd", Version=1)

        # Process the empty beam for sample
        SANSILLReduction(
            Run="027916",
            ProcessAs="Beam",
            BeamRadius=1.0,
            AbsorberInputWorkspace="Cd",
            OutputWorkspace="Db",
            FluxOutputWorkspace="flux",
            Version=1,
        )

        # Process the empty beam for sample transmission
        SANSILLReduction(Run="027858", ProcessAs="Beam", AbsorberInputWorkspace="Cd", OutputWorkspace="Dbtr", Version=1)

        # Sample container transmission
        SANSILLReduction(
            Run="027860",
            ProcessAs="Transmission",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Dbtr",
            OutputWorkspace="sc_tr",
            Version=1,
        )

        # Sample container
        SANSILLReduction(
            Run="027930",
            ProcessAs="Container",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Db",
            TransmissionInputWorkspace="sc_tr",
            OutputWorkspace="sc",
            Version=1,
        )

        # Sample transmission
        SANSILLReduction(
            Run="027985",
            ProcessAs="Transmission",
            AbsorberInputWorkspace="Cd",
            BeamInputWorkspace="Dbtr",
            OutputWorkspace="s_tr",
            Version=1,
        )

        # Sample with flux
        SANSILLReduction(
            Run="027925",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            AbsorberInputWorkspace="Cd",
            ContainerInputWorkspace="sc",
            BeamInputWorkspace="Db",
            TransmissionInputWorkspace="s_tr",
            OutputWorkspace="sample_flux",
            FluxInputWorkspace="flux",
            Version=1,
        )

        # I(Q)
        SANSILLIntegration(InputWorkspace="sample_flux", OutputWorkspace="iq")
