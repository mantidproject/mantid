# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import CompareWorkspaces, GroupWorkspaces, LoadNexusProcessed, SANSILLReduction, Scale


class D11_AbsoluteScale_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(D11_AbsoluteScale_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"
        config.appendDataSearchSubDir("ILL/D11/")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["abs_scale_outputs", "D11_AbsScaleReference.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D11_mask.nxs", OutputWorkspace="mask")

        # Process the dark current Cd/B4C for water
        SANSILLReduction(Run="010455", ProcessAs="Absorber", OutputWorkspace="Cdw", Version=1)

        # Process the empty beam for water
        SANSILLReduction(
            Run="010414", ProcessAs="Beam", AbsorberInputWorkspace="Cdw", OutputWorkspace="Dbw", FluxOutputWorkspace="Flw", Version=1
        )
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
        # Water as reference
        SANSILLReduction(
            Run="010453",
            ProcessAs="Sample",
            AbsorberInputWorkspace="Cdw",
            MaskedInputWorkspace="mask",
            ContainerInputWorkspace="wc",
            BeamInputWorkspace="Dbw",
            TransmissionInputWorkspace="wc_tr",
            SensitivityOutputWorkspace="sens",
            OutputWorkspace="reference",
            FluxInputWorkspace="Flw",
            Version=1,
        )
        # Water as sample with sensitivity and flux
        SANSILLReduction(
            Run="010453",
            ProcessAs="Sample",
            AbsorberInputWorkspace="Cdw",
            MaskedInputWorkspace="mask",
            ContainerInputWorkspace="wc",
            BeamInputWorkspace="Dbw",
            TransmissionInputWorkspace="wc_tr",
            SensitivityInputWorkspace="sens",
            OutputWorkspace="water_with_sens_flux",
            FluxInputWorkspace="Flw",
            Version=1,
        )
        # Water with itself as reference and flux
        SANSILLReduction(
            Run="010453",
            ProcessAs="Sample",
            AbsorberInputWorkspace="Cdw",
            MaskedInputWorkspace="mask",
            ContainerInputWorkspace="wc",
            BeamInputWorkspace="Dbw",
            TransmissionInputWorkspace="wc_tr",
            ReferenceInputWorkspace="reference",
            OutputWorkspace="water_with_reference",
            FluxInputWorkspace="Flw",
            Version=1,
        )

        # Group the worksaces
        GroupWorkspaces(
            InputWorkspaces=["sens", "reference", "water_with_reference", "water_with_sens_flux"], OutputWorkspace="abs_scale_outputs"
        )


class D11_AbsoluteScaleFlux_Test(systemtesting.MantidSystemTest):
    def __init__(self):
        super(D11_AbsoluteScaleFlux_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"
        config.appendDataSearchSubDir("ILL/D11/")

    def cleanup(self):
        mtd.clear()

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D11_mask.nxs", OutputWorkspace="mask")

        # Calculate flux for water
        SANSILLReduction(Run="010414", ProcessAs="Beam", OutputWorkspace="Dbw", FluxOutputWorkspace="flw", Version=1)

        # Reduce water with flux normalisation
        SANSILLReduction(
            Run="010453",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            OutputWorkspace="water_with_flux",
            FluxInputWorkspace="flw",
            Version=1,
        )

        # Reduce water without flux normalisation
        SANSILLReduction(Run="010453", ProcessAs="Sample", MaskedInputWorkspace="mask", OutputWorkspace="water_wo_flux", Version=1)

        # Calculate flux for sample
        SANSILLReduction(Run="010413", ProcessAs="Beam", OutputWorkspace="Db", FluxOutputWorkspace="fl", Version=1)

        # Reduce sample with flux normalisation and flux normalised water reference
        SANSILLReduction(
            Run="010569",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            OutputWorkspace="sample_with_flux",
            FluxInputWorkspace="fl",
            ReferenceInputWorkspace="water_with_flux",
            Version=1,
        )

        # Reduce sample without flux normalisation and not flux normalised water reference
        SANSILLReduction(
            Run="010569",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            OutputWorkspace="sample_wo_flux",
            ReferenceInputWorkspace="water_wo_flux",
            Version=1,
        )

        # Now the sample_with_flux and sample_wo_flux should be approximately at the same scale
        result1, _ = CompareWorkspaces(Workspace1="sample_with_flux", Workspace2="sample_wo_flux", Tolerance=0.1)
        self.assertTrue(result1)

        # Then we want to simulate the situation where water has no flux measurement
        # Reduce water, but normalise it to the sample flux
        # Commit f25a6cd5 removed the rescaling by the distance ratio squared in the algorithm
        SANSILLReduction(
            Run="010453",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            OutputWorkspace="water_with_sample_flux",
            FluxInputWorkspace="fl",
            Version=1,
        )

        # Reduce sample with flux normalisation and sample flux normalised water reference
        # Here there is no additional scaling, since both are already normalised
        SANSILLReduction(
            Run="010569",
            ProcessAs="Sample",
            MaskedInputWorkspace="mask",
            OutputWorkspace="sample_with_flux_water_with_sample_flux",
            FluxInputWorkspace="fl",
            ReferenceInputWorkspace="water_with_sample_flux",
            Version=1,
        )

        # Before f25a6cd5 this output should still be at the same scale as the two above
        # (basically it is the same scaling, just happening in different place)
        # After f25a6cd5, since the scaling is removed, we need to scale by that factor before comparing
        Scale(
            InputWorkspace="sample_with_flux_water_with_sample_flux",
            OutputWorkspace="sample_with_flux_water_with_sample_flux",
            Factor=(20.007 / 8) ** 2,
        )
        result2, _ = CompareWorkspaces(Workspace1="sample_with_flux_water_with_sample_flux", Workspace2="sample_wo_flux", Tolerance=0.1)
        self.assertTrue(result2)

        result3, _ = CompareWorkspaces(Workspace1="sample_with_flux_water_with_sample_flux", Workspace2="sample_with_flux", Tolerance=0.1)
        self.assertTrue(result3)

        # Finally we want to make sure that trying to divide flux normalised sample by
        # non flux normalised water raises an error
        kwargs = {
            "Run": "010569",
            "ProcessAs": "Sample",
            "MaskedInputWorkspace": "mask",
            "OutputWorkspace": "sample_with_flux_water_wo_flux",
            "FluxInputWorkspace": "fl",
            "ReferenceInputWorkspace": "water_wo_flux",
            "Version": 1,
        }
        self.assertRaises(RuntimeError, SANSILLReduction, **kwargs)
