# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    CalculateEfficiencyCorrection,
    CloneWorkspace,
    ConvertToPointData,
    CreateSampleWorkspace,
    DeleteWorkspace,
    LoadAscii,
    Multiply,
)
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService


class CalculateEfficiencyCorrectionTest(unittest.TestCase):
    _input_wksp = "input_wksp"
    _correction_wksp = "correction_wksp"
    _output_wksp = "output_wksp"
    _wavelengths = "0.2,0.01,4.0"
    _alpha = 0.693
    _chemical_formula = "(He3)"
    _number_density = 0.0002336682167635477
    _mass_density = 0.0011702649036052439
    _efficiency1_forAbsXS = 0.712390781371
    _efficiency2_forAbsXS = {"Efficiency": 0.74110947758, "Wavelength": 1.95}
    _efficiency1_forTotalXS = 0.712793729636
    _efficiency2_forTotalXS = {"Efficiency": 0.741472190178, "Wavelength": 1.95}
    _thickness = 1.0

    def setUp(self):
        """
        This file is the back-calculated spectrum of polyethylene moderator (ambient 300K)
        prior to the efficiency correction described in Eq (3) of:
          Mildner et al. "A cooled polyethylene moderator on a pulsed neutron source",
          Nucl. Instr. Meth. 152, 1978, doi:/10.1016/0029-554X(78)90043-5

        After the correction is applied, the workspace will replicated (b) in Fig. 2 of this article.

        Similar results are shown in Fig 1.a for "ambient (300K)" in:
          S. Howells, "On the choice of moderator for a liquids diffractometer on a pulsed neutron source",
          Nucl. Instr. Meth. Phys. Res. 223, 1984, doi:10.1016/0167-5087(84)90256-4
        """
        LoadAscii(
            OutputWorkspace=self._input_wksp,
            Filename="CalculateEfficiencyCorrection_milder_moderator_polyethlyene_300K.txt",
            Unit="Wavelength",
        )
        ConvertToPointData(InputWorkspace=self._input_wksp, OutputWorkspace=self._input_wksp)

    def checkResults(self, eventCheck=False, xsection="AttenuationXSection"):
        # Check results

        Multiply(LHSWorkspace=self._input_wksp, RHSWorkspace=self._correction_wksp, OutputWorkspace=self._output_wksp)
        output_wksp = AnalysisDataService.retrieve(self._output_wksp)

        self.assertEqual(output_wksp.getAxis(0).getUnit().unitID(), "Wavelength")
        self.assertAlmostEqual(output_wksp.readX(0)[79], 0.995)
        if eventCheck:
            self.assertAlmostEqual(output_wksp.readY(0)[79], 66.23970242900438)
        else:
            if xsection == "AttenuationXSection":
                self.assertAlmostEqual(output_wksp.readY(0)[79], 3250.28183501)
            if xsection == "TotalXSection":
                self.assertAlmostEqual(output_wksp.readY(0)[79], 3245.70148939)

    # Result tests
    def testCalculateEfficiencyCorrectionAlphaForEventWksp(self):
        self._input_wksp = "input_wksp"
        self._correction_wksp = "correction_wksp"
        self._output_wksp = "output_wksp"

        # Create an exponentially decaying function in wavelength to simulate
        # measured sample
        CreateSampleWorkspace(
            WorkspaceType="Event",
            Function="User Defined",
            UserDefinedFunction="name=ExpDecay, Height=100, Lifetime=4",
            Xmin=0.2,
            Xmax=4.0,
            BinWidth=0.01,
            XUnit="Wavelength",
            NumEvents=10000,
            NumBanks=1,
            BankPixelWidth=1,
            OutputWorkspace=self._input_wksp,
        )

        # Calculate the efficiency correction
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection", InputWorkspace=self._input_wksp, Alpha=self._alpha, OutputWorkspace=self._correction_wksp
        )
        self.assertTrue(alg_test.isExecuted())
        ConvertToPointData(InputWorkspace=self._input_wksp, OutputWorkspace=self._input_wksp)
        self.checkResults(eventCheck=True)

    def testCalculateEfficiencyCorrectionAlphaForEventWkspInputWkspNotInADS(self):
        self.cleanup()

        # Create an exponentially decaying function in wavelength to simulate
        # measured sample
        tmp_wksp = CreateSampleWorkspace(
            WorkspaceType="Event",
            Function="User Defined",
            UserDefinedFunction="name=ExpDecay, Height=100, Lifetime=4",
            Xmin=0.2,
            Xmax=4.0,
            BinWidth=0.01,
            XUnit="Wavelength",
            NumEvents=10000,
            NumBanks=1,
            BankPixelWidth=1,
            OutputWorkspace=self._input_wksp,
            StoreInADS=False,
        )

        # Calculate the efficiency correction
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection", InputWorkspace=tmp_wksp, Alpha=self._alpha, OutputWorkspace=self._correction_wksp
        )
        self.assertTrue(alg_test.isExecuted())
        CloneWorkspace(InputWorkspace=tmp_wksp, OutputWorkspace=self._input_wksp)
        ConvertToPointData(InputWorkspace=self._input_wksp, OutputWorkspace=self._input_wksp)
        self.checkResults(eventCheck=True)

    def testCalculateEfficiencyCorrectionAlpha(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection", InputWorkspace=self._input_wksp, Alpha=self._alpha, OutputWorkspace=self._correction_wksp
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionAlphaWithWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection", WavelengthRange=self._wavelengths, Alpha=self._alpha, OutputWorkspace=self._correction_wksp
        )
        self.assertTrue(alg_test.isExecuted())
        self.checkResults()

    def testCalculateEfficiencyCorrectionMassDensityAndThickness(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            Density=self._mass_density,
            Thickness=self._thickness,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionMassDensityAndThicknessWithWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            Density=self._mass_density,
            Thickness=self._thickness,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionNumberDensityAndThickness(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            DensityType="Number Density",
            Density=self._number_density,
            Thickness=self._thickness,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionNumberDensityAndThicknessWithWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            DensityType="Number Density",
            Density=self._number_density,
            Thickness=self._thickness,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionEfficiency(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=self._efficiency1_forAbsXS,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionEfficiencyWithWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=self._efficiency1_forAbsXS,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionEfficiencyWithWavelength(self):
        efficiency = self._efficiency2_forAbsXS["Efficiency"]
        wavelength = self._efficiency2_forAbsXS["Wavelength"]
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=efficiency,
            MeasuredEfficiencyWavelength=wavelength,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionEfficiencyWithWavelengthWithWaveRange(self):
        efficiency = self._efficiency2_forAbsXS["Efficiency"]
        wavelength = self._efficiency2_forAbsXS["Wavelength"]
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=efficiency,
            MeasuredEfficiencyWavelength=wavelength,
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionAlphaWithTotalXS(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            Alpha=self._alpha,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionAlphaWithTotalXSWithWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            Alpha=self._alpha,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionMassDensityAndThicknessWithTotalXS(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            Density=self._mass_density,
            Thickness=self._thickness,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorrectionMassDensityAndThicknessWithTotalXSWithWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            Density=self._mass_density,
            Thickness=self._thickness,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorrectionNumberDensityAndThicknessWithTotalXS(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            DensityType="Number Density",
            Density=self._number_density,
            Thickness=self._thickness,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorrectionNumberDensityAndThicknessWithTotalXSWithWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            DensityType="Number Density",
            Density=self._number_density,
            Thickness=self._thickness,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorrectionEfficiencyWithTotalXS(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=self._efficiency1_forTotalXS,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorrectionEfficiencyWithTotalXSWitWaveRange(self):
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=self._efficiency1_forTotalXS,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorrectionEfficiencyWithWavelengthWithTotalXS(self):
        efficiency = self._efficiency2_forTotalXS["Efficiency"]
        wavelength = self._efficiency2_forTotalXS["Wavelength"]
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=efficiency,
            MeasuredEfficiencyWavelength=wavelength,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorrectionEfficiencyWithWavelengthWithTotalXSWithWaveRange(self):
        efficiency = self._efficiency2_forTotalXS["Efficiency"]
        wavelength = self._efficiency2_forTotalXS["Wavelength"]
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection",
            WavelengthRange=self._wavelengths,
            ChemicalFormula=self._chemical_formula,
            MeasuredEfficiency=efficiency,
            MeasuredEfficiencyWavelength=wavelength,
            XSectionType="TotalXSection",
            OutputWorkspace=self._correction_wksp,
        )
        self.assertTrue(alg_test.isExecuted())

        self.checkResults(xsection="TotalXSection")

    def testCalculateEfficiencyCorretionStoreADSCheck(self):
        self.cleanup()
        alg_test = run_algorithm(
            "CalculateEfficiencyCorrection", WavelengthRange=self._wavelengths, Alpha=self._alpha, OutputWorkspace=self._output_wksp
        )
        self.assertTrue(alg_test.isExecuted())
        self.assertTrue(AnalysisDataService.doesExist(self._output_wksp))

    # Invalid checks
    def testCalculateEfficiencyCorretionInvalidStoreADSCheck(self):
        self.cleanup()
        corr_wksp = CalculateEfficiencyCorrection(WavelengthRange=self._wavelengths, Alpha=self._alpha, StoreInADS=False)
        self.assertFalse(AnalysisDataService.doesExist(corr_wksp.name()))

    def testCalculateEfficiencyCorretionInvalidInput(self):
        self.assertRaises(RuntimeError, CalculateEfficiencyCorrection, OutputWorkspace=self._output_wksp)

    def testCalculateEfficiencyCorretionInvalidTooManyInputs(self):
        self.assertRaises(
            RuntimeError,
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            WavelengthRange=self._wavelengths,
            OutputWorkspace=self._output_wksp,
        )

    def testCalculateEfficiencyCorretionInvalidDensity(self):
        self.assertRaises(
            ValueError,
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            ChemicalFormula=self._chemical_formula,
            Density=-1.0,
            OutputWorkspace=self._output_wksp,
        )

    def testCalculateEfficiencyCorretionMassDensityWithoutChemicalFormula(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Must specify the ChemicalFormula with Density",
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            Density=1.0,
            OutputWorkspace=self._output_wksp,
        )

    def testCalculateEfficiencyCorretionNumberDensityWithoutChemicalFormula(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Must specify the ChemicalFormula with Density",
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            DensityType="Number Density",
            Density=1.0,
            OutputWorkspace=self._output_wksp,
        )

    def testCalculateEfficiencyCorretionEfficiencyWithoutChemicalFormula(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Must specify the ChemicalFormula with MeasuredEfficiency",
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            MeasuredEfficiency=1.0,
            OutputWorkspace=self._output_wksp,
        )

    def testCalculateEfficiencyCorretionEfficiencyAndDensity(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot select both MeasuredEfficiency and Density as input",
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            Density=1.0,
            MeasuredEfficiency=1.0,
            OutputWorkspace=self._output_wksp,
        )

    def testCalculateEfficiencyCorretionAlphaAndDensity(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot select both Alpha and Density as input",
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            Density=1.0,
            Alpha=1.0,
            OutputWorkspace=self._output_wksp,
        )

    def testCalculateEfficiencyCorretionEfficiencyAndAlpha(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Cannot select both MeasuredEfficiency and Alpha as input",
            CalculateEfficiencyCorrection,
            InputWorkspace=self._input_wksp,
            Alpha=1.0,
            MeasuredEfficiency=1.0,
            OutputWorkspace=self._output_wksp,
        )

    def cleanup(self):
        if AnalysisDataService.doesExist(self._input_wksp):
            DeleteWorkspace(self._input_wksp)
        if AnalysisDataService.doesExist(self._output_wksp):
            DeleteWorkspace(self._output_wksp)
        if AnalysisDataService.doesExist(self._correction_wksp):
            DeleteWorkspace(self._correction_wksp)


if __name__ == "__main__":
    unittest.main()
