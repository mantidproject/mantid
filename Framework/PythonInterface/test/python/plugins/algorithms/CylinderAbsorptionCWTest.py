# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from mantid.simpleapi import CylinderAbsorptionCW, CreateSampleWorkspace, EditInstrumentGeometry, SetSample


class CylinderAbsorptionCWTest(unittest.TestCase):
    def createWorkspace(self):
        # Create a sample workspace with 4 spectra with 2theta at 45, 90, 135, and 90 degrees
        ws = CreateSampleWorkspace(NumBanks=4, BankPixelWidth=1)
        EditInstrumentGeometry(
            ws,
            PrimaryFlightPath=5.0,
            SpectrumIDs=[1, 2, 3, 4],
            L2=[2.0, 2.0, 2.0, 2.0],
            Polar=[45.0, 90.0, 135.0, 90.0],
            Azimuthal=[0.0, 0.0, 0.0, 45.0],
            DetectorIDs=[1, 2, 3, 4],
            InstrumentName="Instrument",
        )
        return ws

    def testSears(self):
        ws = self.createWorkspace()

        # Run the algorithm with Sears method using Vanadium properties
        result = CylinderAbsorptionCW(
            InputWorkspace=ws,
            Radius=0.5,  # cm
            Height=1.0,  # cm
            Wavelength=1.7982,  # Å
            AttenuationXSection=5.08,  # barn at 1.798 Å
            ScatteringXSection=5.1,  # barn
            SampleNumberDensity=0.0723,  # atoms/Å^3
            AbsorptionCorrectionMethod="Sears",
            AbsorptionWorkspace="Absorption",
            MultipleScatteringWorkspace="MultipleScattering",
        )

        # Check absorption
        np.testing.assert_allclose(result.AbsorptionWorkspace.extractY()[:, 0], [0.54506044, 0.55826703, 0.57179362, 0.55826703])

        # Check multiple scattering
        self.assertAlmostEqual(result.MultipleScatteringWorkspace.extractY()[0][0], 0.120424)

    def testSears_large_μR(self):
        ws = self.createWorkspace()

        # When μR > 0.9, the algorithm should raise a ValueError indicating that Sears method cannot be used

        with self.assertRaises(RuntimeError) as context:
            CylinderAbsorptionCW(
                InputWorkspace=ws,
                Radius=5,  # cm
                Height=10,  # cm
                Wavelength=1.7982,  # Å
                AttenuationXSection=5.08,  # barn at 1.798 Å
                ScatteringXSection=5.1,  # barn
                SampleNumberDensity=0.0723,  # atoms/Å^3
                AbsorptionCorrectionMethod="Sears",
                AbsorptionWorkspace="Absorption",
                MultipleScatteringWorkspace="MultipleScattering",
            )

        self.assertIn(
            "μR > 0.9. Absorption correction cannot be calculated to ~1% accuracy using Sears method. Please select Sabine.",
            str(context.exception),
        )

    def testSearSetSamples(self):
        ws = self.createWorkspace()

        SetSample(
            ws,
            Geometry={
                "Shape": "Cylinder",
                "Height": 1.0,  # cm
                "Radius": 0.5,  # cm
            },
            Material={
                "ChemicalFormula": "V",
                "SampleNumberDensity": 0.0723,
            },
        )

        # Run the algorithm with Sears method getting radius and height from the workspace sample shape
        # and cross-sections from the workspace sample material
        result = CylinderAbsorptionCW(
            InputWorkspace=ws,
            Wavelength=1.7982,  # Å
            AbsorptionCorrectionMethod="Sears",
            AbsorptionWorkspace="Absorption",
            MultipleScatteringWorkspace="MultipleScattering",
        )

        # Check absorption
        np.testing.assert_allclose(result.AbsorptionWorkspace.extractY()[:, 0], [0.54506044, 0.55826703, 0.57179362, 0.55826703])

        # Check multiple scattering
        self.assertAlmostEqual(result.MultipleScatteringWorkspace.extractY()[0][0], 0.120424)

    def testSabine(self):
        ws = self.createWorkspace()

        # Run the algorithm with Sabine method using Vanadium properties
        result = CylinderAbsorptionCW(
            InputWorkspace=ws,
            Radius=0.5,  # cm
            Height=1.0,  # cm
            Wavelength=1.7982,  # Å
            AttenuationXSection=5.08,  # barn at 1.798 Å
            ScatteringXSection=5.1,  # barn
            SampleNumberDensity=0.0723,  # atoms/Å^3
            AbsorptionCorrectionMethod="Sabine",
            AbsorptionWorkspace="Absorption",
            MultipleScatteringWorkspace="MultipleScattering",
            MultipleScattering=False,
        )

        # Check absorption
        np.testing.assert_allclose(result.AbsorptionWorkspace.extractY()[:, 0], [0.54520177, 0.55782039, 0.570439, 0.55782039])

        # Check multiple scattering, should be 0 since we set MultipleScattering=False
        self.assertEqual(result.MultipleScatteringWorkspace.extractY()[0][0], 0)

    def testMissingProperties(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1)

        # Missing radius and height properties and workspace has no valid sample shape
        with self.assertRaises(RuntimeError) as context:
            CylinderAbsorptionCW(
                InputWorkspace=ws,
                Wavelength=1.7982,  # Å
                AttenuationXSection=5.08,  # barn at 1.798 Å
                ScatteringXSection=5.1,  # barn
                SampleNumberDensity=0.0723,  # atoms/Å^3
                AbsorptionWorkspace="Absorption",
                MultipleScatteringWorkspace="MultipleScattering",
            )
        self.assertIn("Please provide radius and height properties or use a workspace with a cylinder sample shape", str(context.exception))

        # Missing cross-section properties
        with self.assertRaises(RuntimeError) as context:
            CylinderAbsorptionCW(
                InputWorkspace=ws,
                Wavelength=1.7982,  # Å
                Radius=0.5,  # cm
                Height=1.0,  # cm
                AbsorptionWorkspace="Absorption",
                MultipleScatteringWorkspace="MultipleScattering",
            )
        self.assertIn("Attenuation cross-section, scattering cross-section, and number density are not provided ", str(context.exception))


if __name__ == "__main__":
    unittest.main()
