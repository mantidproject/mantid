# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import \
    LoadAscii, DeleteWorkspace, Multiply, ConvertToPointData, CalculateEfficiencyCorrection
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService


class CalculateEfficiencyCorrectionTest(unittest.TestCase):

    _input_wksp = "input_wksp"
    _correction_wksp = "correction_wksp"
    _output_wksp = "output_wksp"
    _alpha = 0.693
    _chemical_formula = "(He3)"
    _number_density = 0.0002336682167635477
    _mass_density = 0.0011702649036052439
    _efficiency1 = 0.712390781371
    _efficiency2 = { "Efficiency": 0.74110947758, "Wavelength": 1.95}
    _thickness = 1.0

    def setUp(self):
        '''
        This file is the back-calculated spectrum of polyethylene moderator (ambient 300K)
        prior to the efficiency correction described in Eq (3) of:
          Mildner et al. "A cooled polyethylene moderator on a pulsed neutron source",
          Nucl. Instr. Meth. 152, 1978, doi:/10.1016/0029-554X(78)90043-5

        After the correction is applied, the workspace will replicated (b) in Fig. 2 of this article.

        Similar results are shown in Fig 1.a for "ambient (300K)" in:
          S. Howells, "On the choice of moderator for a liquids diffractometer on a pulsed neutron source",
          Nucl. Instr. Meth. Phys. Res. 223, 1984, doi:10.1016/0167-5087(84)90256-4
        '''
        input_wksp = LoadAscii(
                        Filename="CalculateEfficiencyCorrection_milder_moderator_polyethlyene_300K.txt",
                        Unit="Wavelength")
        ConvertToPointData(InputWorkspace=input_wksp, OutputWorkspace=input_wksp)
        self._input_wksp = input_wksp

    def checkResults(self):
        Multiply(LHSWorkspace=self._input_wksp,
                 RHSWorkspace=self._correction_wksp,
                 OutputWorkspace=self._output_wksp)
        output_wksp = AnalysisDataService.retrieve(self._output_wksp)

        self.assertEqual(output_wksp.getAxis(0).getUnit().unitID(), 'Wavelength')
        self.assertAlmostEqual(output_wksp.readX(0)[79], 0.995)
        self.assertAlmostEqual(output_wksp.readY(0)[79], 3250.28183501)

    # Result tests

    def testCalculateEfficiencyCorrectionAlpha(self):
        alg_test = run_algorithm("CalculateEfficiencyCorrection",
                                 InputWorkspace=self._input_wksp,
                                 Alpha=self._alpha,
                                 OutputWorkspace=self._correction_wksp)
        self.assertTrue(alg_test.isExecuted())

        self.checkResults()

    def testCalculateEfficiencyCorrectionMassDensityAndThickness(self):
        correction_wksp = "correction_ws"
        alg_test = run_algorithm("CalculateEfficiencyCorrection",
                                 InputWorkspace=self._input_wksp,
                                 ChemicalFormula=self._chemical_formula,
                                 Density=self._mass_density,
                                 Thickness=self._thickness,
                                 OutputWorkspace=correction_wksp)
        self.assertTrue(alg_test.isExecuted())
        self._correction_wksp = AnalysisDataService.retrieve(correction_wksp)

        self.checkResults()

    def testCalculateEfficiencyCorrectionNumberDensityAndThickness(self):
        correction_wksp = "correction_ws"
        alg_test = run_algorithm("CalculateEfficiencyCorrection",
                                 InputWorkspace=self._input_wksp,
                                 ChemicalFormula=self._chemical_formula,
                                 DensityType="Number Density",
                                 Density=self._number_density,
                                 Thickness=self._thickness,
                                 OutputWorkspace=correction_wksp)
        self.assertTrue(alg_test.isExecuted())
        self._correction_wksp = AnalysisDataService.retrieve(correction_wksp)

        self.checkResults()

    def testCalculateEfficiencyCorrectionEfficiency(self):
        correction_wksp = "correction_ws"
        alg_test = run_algorithm("CalculateEfficiencyCorrection",
                                 InputWorkspace=self._input_wksp,
                                 ChemicalFormula=self._chemical_formula,
                                 MeasuredEfficiency=self._efficiency1,
                                 OutputWorkspace=correction_wksp)
        self.assertTrue(alg_test.isExecuted())
        self._correction_wksp = AnalysisDataService.retrieve(correction_wksp)

        self.checkResults()

    def testCalculateEfficiencyCorrectionEfficiencyWithWavelength(self):
        correction_wksp = "correction_ws"
        efficiency=self._efficiency2['Efficiency']
        wavelength=self._efficiency2['Wavelength']
        alg_test = run_algorithm("CalculateEfficiencyCorrection",
                                 InputWorkspace=self._input_wksp,
                                 ChemicalFormula=self._chemical_formula,
                                 MeasuredEfficiency=efficiency,
                                 MeasuredEfficiencyWavelength=wavelength,
                                 OutputWorkspace=correction_wksp)
        self.assertTrue(alg_test.isExecuted())
        self._correction_wksp = AnalysisDataService.retrieve(correction_wksp)

        self.checkResults()

    # Invalid checks
    def testCalculateEfficiencyCorretionInvalidDensity(self):
        self.assertRaises(RuntimeError,
                          CalculateEfficiencyCorrection,
                          InputWorkspace=self._input_wksp,
                          ChemicalFormula=self._chemical_formula,
                          Density=-1.0,
                          OutputWorkspace=self._output_wksp)

    def testCalculateEfficiencyCorretionMassDensityWithoutChemicalFormula(self):
        self.assertRaises(RuntimeError,
                          CalculateEfficiencyCorrection,
                          InputWorkspace=self._input_wksp,
                          Density=1.0,
                          OutputWorkspace=self._output_wksp)

    def testCalculateEfficiencyCorretionNumberDensityWithoutChemicalFormula(self):
        self.assertRaises(RuntimeError,
                          CalculateEfficiencyCorrection,
                          InputWorkspace=self._input_wksp,
                          DensityType="Number Density",
                          Density=1.0,
                          OutputWorkspace=self._output_wksp)

    def testCalculateEfficiencyCorretionEfficiencyWithoutChemicalFormula(self):
        self.assertRaises(RuntimeError,
                          CalculateEfficiencyCorrection,
                          InputWorkspace=self._input_wksp,
                          MeasuredEfficiency=1.0,
                          OutputWorkspace=self._output_wksp)

    def testCalculateEfficiencyCorretionEfficiencyAndDensity(self):
        self.assertRaises(RuntimeError,
                          CalculateEfficiencyCorrection,
                          InputWorkspace=self._input_wksp,
                          Density=1.0,
                          MeasuredEfficiency=1.0,
                          OutputWorkspace=self._output_wksp)

    def testCalculateEfficiencyCorretionAlphaAndDensity(self):
        self.assertRaises(RuntimeError,
                          CalculateEfficiencyCorrection,
                          InputWorkspace=self._input_wksp,
                          Density=1.0,
                          Alpha=1.0,
                          OutputWorkspace=self._output_wksp)

    def testCalculateEfficiencyCorretionEfficiencyAndAlpha(self):
        self.assertRaises(RuntimeError,
                          CalculateEfficiencyCorrection,
                          InputWorkspace=self._input_wksp,
                          Alpha=1.0,
                          MeasuredEfficiency=1.0,
                          OutputWorkspace=self._output_wksp)

    def cleanUp(self):
        if AnalysisDataService.doesExist(self._input_wksp):
            DeleteWorkspace(self._input_wksp)
        if AnalysisDataService.doesExist(self._output_wksp):
            DeleteWorkspace(self._output_wksp)


if __name__ == "__main__":
    unittest.main()
