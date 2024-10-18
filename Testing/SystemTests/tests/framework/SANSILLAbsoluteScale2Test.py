# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import CompareWorkspaces, ConvertToHistogram, GroupWorkspaces, LoadNexusProcessed, SANSILLReduction, Scale


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
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["abs_scale_outputs", "D11_AbsScaleReference.nxs"]

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename="D11_mask.nxs", OutputWorkspace="mask")

        # Process the dark current Cd/B4C for water
        SANSILLReduction(Runs="010455", ProcessAs="DarkCurrent", OutputWorkspace="Cdw", Version=2)

        # Process the empty beam for water
        SANSILLReduction(
            Runs="010414", ProcessAs="EmptyBeam", DarkCurrentWorkspace="Cdw", OutputWorkspace="Dbw", OutputFluxWorkspace="Flw", Version=2
        )
        # Water container transmission
        SANSILLReduction(
            Runs="010446",
            ProcessAs="Transmission",
            DarkCurrentWorkspace="Cdw",
            EmptyBeamWorkspace="Dbw",
            FluxWorkspace="Flw",
            OutputWorkspace="wc_tr",
            Version=2,
        )
        # Water container
        SANSILLReduction(
            Runs="010454",
            ProcessAs="EmptyContainer",
            DarkCurrentWorkspace="Cdw",
            EmptyBeamWorkspace="Dbw",
            FluxWorkspace="Flw",
            TransmissionWorkspace="wc_tr",
            OutputWorkspace="wc",
            Version=2,
        )
        # Water transmission
        SANSILLReduction(
            Runs="010445",
            ProcessAs="Transmission",
            DarkCurrentWorkspace="Cdw",
            EmptyBeamWorkspace="Dbw",
            FluxWorkspace="Flw",
            OutputWorkspace="w_tr",
            Version=2,
        )
        # Water as reference
        SANSILLReduction(
            Runs="010453",
            ProcessAs="Water",
            DarkCurrentWorkspace="Cdw",
            MaskWorkspace="mask",
            EmptyContainerWorkspace="wc",
            EmptyBeamWorkspace="Dbw",
            FluxWorkspace="Flw",
            TransmissionWorkspace="wc_tr",
            OutputSensitivityWorkspace="sens",
            OutputWorkspace="reference",
            Version=2,
        )

        # Water as sample with sensitivity and flux
        SANSILLReduction(
            Runs="010453",
            ProcessAs="Sample",
            DarkCurrentWorkspace="Cdw",
            MaskWorkspace="mask",
            EmptyContainerWorkspace="wc",
            EmptyBeamWorkspace="Dbw",
            FluxWorkspace="Flw",
            TransmissionWorkspace="wc_tr",
            SensitivityWorkspace="sens",
            OutputWorkspace="water_with_sens_flux",
            Version=2,
        )
        # Water with itself as reference and flux
        SANSILLReduction(
            Runs="010453",
            ProcessAs="Sample",
            DarkCurrentWorkspace="Cdw",
            MaskWorkspace="mask",
            EmptyContainerWorkspace="wc",
            EmptyBeamWorkspace="Dbw",
            FluxWorkspace="Flw",
            TransmissionWorkspace="wc_tr",
            FlatFieldWorkspace="reference",
            OutputWorkspace="water_with_reference",
            Version=2,
        )

        # Setting up proper X axis since SANSILLReductionv2 provides PointData workspaces
        sens = ConvertToHistogram(mtd["sens"])
        reference = ConvertToHistogram(mtd["reference"])
        water_with_reference = ConvertToHistogram(mtd["water_with_reference"])
        water_with_sens_flux = ConvertToHistogram(mtd["water_with_sens_flux"])
        for s in range(sens.getNumberHistograms()):
            sens.setX(s, np.array([5.73, 6.27]))
            reference.setX(s, np.array([5.73, 6.27]))
            water_with_reference.setX(s, np.array([5.73, 6.27]))
            water_with_sens_flux.setX(s, np.array([5.73, 6.27]))
        sens.getAxis(0).setUnit("Wavelength")
        reference.getAxis(0).setUnit("Wavelength")
        water_with_reference.getAxis(0).setUnit("Wavelength")
        water_with_sens_flux.getAxis(0).setUnit("Wavelength")

        # Group the workspaces
        GroupWorkspaces(InputWorkspaces=[sens, reference, water_with_reference, water_with_sens_flux], OutputWorkspace="abs_scale_outputs")


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
        SANSILLReduction(Runs="010414", ProcessAs="EmptyBeam", OutputWorkspace="Dbw", OutputFluxWorkspace="flw", Version=2)

        # Reduce water with flux normalisation
        SANSILLReduction(
            Runs="010453",
            ProcessAs="Sample",
            MaskWorkspace="mask",
            OutputWorkspace="water_with_flux",
            FluxWorkspace="flw",
            Version=2,
        )

        # Reduce water without flux normalisation
        SANSILLReduction(Runs="010453", ProcessAs="Sample", MaskWorkspace="mask", OutputWorkspace="water_wo_flux", Version=2)

        # Calculate flux for sample
        SANSILLReduction(Runs="010413", ProcessAs="EmptyBeam", OutputWorkspace="Db", OutputFluxWorkspace="fl", Version=2)

        # Reduce sample with flux normalisation and flux normalised water reference
        SANSILLReduction(
            Runs="010569",
            ProcessAs="Sample",
            MaskWorkspace="mask",
            OutputWorkspace="sample_with_flux",
            FluxWorkspace="fl",
            FlatFieldWorkspace="water_with_flux",
            Version=2,
        )

        # Reduce sample without flux normalisation and not flux normalised water reference
        SANSILLReduction(
            Runs="010569",
            ProcessAs="Sample",
            MaskWorkspace="mask",
            OutputWorkspace="sample_wo_flux",
            FlatFieldWorkspace="water_wo_flux",
            Version=2,
        )

        # Now the sample_with_flux and sample_wo_flux should be approximately at the same scale
        result1, _ = CompareWorkspaces(Workspace1="sample_with_flux", Workspace2="sample_wo_flux", Tolerance=0.1)
        self.assertTrue(result1)

        # Then we want to simulate the situation where water has no flux measurement
        # Reduce water, but normalise it to the sample flux
        # Commit f25a6cd5 removed the rescaling by the distance ratio squared in the algorithm
        SANSILLReduction(
            Runs="010453",
            ProcessAs="Sample",
            MaskWorkspace="mask",
            OutputWorkspace="water_with_sample_flux",
            FluxWorkspace="fl",
            Version=2,
        )

        # Reduce sample with flux normalisation and sample flux normalised water reference
        # Here there is no additional scaling, since both are already normalised
        SANSILLReduction(
            Runs="010569",
            ProcessAs="Sample",
            MaskWorkspace="mask",
            OutputWorkspace="sample_with_flux_water_with_sample_flux",
            FluxWorkspace="fl",
            FlatFieldWorkspace="water_with_sample_flux",
            Version=2,
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
            "Runs": "010569",
            "ProcessAs": "Sample",
            "MaskWorkspace": "mask",
            "OutputWorkspace": "sample_with_flux_water_wo_flux",
            "FluxWorkspace": "fl",
            "FlatFieldWorkspace": "water_wo_flux",
            "Version": 2,
        }
        self.assertRaises(RuntimeError, SANSILLReduction, **kwargs)
