# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.simpleapi import IndirectTransmission


class IndirectTransmissionTest(unittest.TestCase):
    def test_indirect_transmission_iris_graphite_002(self):
        """
        Test a transmission calculation using IRIS, graphite, 002.
        """

        instrument = "IRIS"
        analyser = "graphite"
        reflection = "002"

        # Using water sample
        formula = "H2-O"
        density = 0.1
        thickness = 0.1

        ws = IndirectTransmission(Instrument=instrument, Analyser=analyser, Reflection=reflection,
                                  ChemicalFormula=formula, DensityType='Number Density', Density=density, Thickness=thickness)

        # Expected values from table
        ref_result = [6.65800, 0.82122, 0.039174, 56.05149, 56.09067, 0.1, 0.1, 0.56602, 0.42931]
        values = ws.column(1)
        np.testing.assert_array_almost_equal(values, ref_result, decimal=4)

    def test_indirect_transmission_tosca_graphite_002(self):
        """
        Test a transmission calculation using TOSCA, graphite, 002.
        """

        instrument = "TOSCA"
        analyser = "graphite"
        reflection = "002"

        # Using water sample
        formula = "H2-O"
        density = 0.1
        thickness = 0.1

        ws = IndirectTransmission(Instrument=instrument, Analyser=analyser, Reflection=reflection,
                                  ChemicalFormula=formula, DensityType='Number Density', Density=density, Thickness=thickness)

        # Expected values from table
        ref_result = [5.5137, 0.680081, 0.039174, 56.05149, 56.09067,   0.1, 0.1, 0.56682, 0.42931]
        values = ws.column(1)
        np.testing.assert_array_almost_equal(values, ref_result, decimal=4)

    def test_indirect_transmission_basis_silicon_111(self):
        """
        Test a transmission calculation using BASIS, silicon 111.
        """

        instrument = "BASIS"
        analyser = "silicon"
        reflection = "111"

        # Using water sample
        formula = "H2-O"
        density = 0.1
        thickness = 0.1

        ws = IndirectTransmission(Instrument=instrument, Analyser=analyser, Reflection=reflection,
                                  ChemicalFormula=formula, DensityType='Number Density', Density=density, Thickness=thickness)

        # Expected values from table
        ref_result = [6.2676, 0.7731, 0.03917, 56.0515, 56.0907, 0.1, 0.1, 0.56630, 0.42931]

        values = ws.column(1)
        np.testing.assert_array_almost_equal(values, ref_result, decimal=4)

    def test_indirect_transmission_analyser_validation(self):
        """
        Verifies analyser validation based on instrument is working.
        """

        instrument = "IRIS"
        analyser = "silicon"
        reflection = "002"

        # Using water sample
        formula = "H2-O"

        self.assertRaises(RuntimeError, IndirectTransmission,
                          Instrument=instrument, Analyser=analyser, Reflection=reflection, ChemicalFormula=formula,
                          OutputWorkspace='__IndirectTransmissionTest_AnalyserValidation')

    def test_indirect_transmission_reflection_validation(self):
        """
        Verified reflection validation based on analyser is working.
        """

        instrument = "IRIS"
        analyser = "graphite"
        reflection = "111"

        # Using water sample
        formula = "H2-O"

        self.assertRaises(RuntimeError, IndirectTransmission,
                          Instrument=instrument, Analyser=analyser, Reflection=reflection, ChemicalFormula=formula,
                          OutputWorkspace='__IndirectTransmissionTest_ReflectionValidation')

    def test_mass_density(self):
        """
        Tests a transmission calculation using mass density
        """
        instrument = "IRIS"
        analyser = "graphite"
        reflection = "002"

        formula = "H2-O"
        density = 1.0
        thickness = 0.1

        ws = IndirectTransmission(Instrument=instrument, Analyser=analyser, Reflection=reflection,
                                  ChemicalFormula=formula, DensityType='Mass Density', Density=density, Thickness=thickness)

        ref_result = [6.65800, 0.82122, 0.0391739, 56.051493, 56.09067, 0.100283900, 0.1, 0.56511, 0.43022]
        mass_values = ws.column(1)
        np.testing.assert_array_almost_equal(mass_values, ref_result, decimal=4)

        density = 0.100272
        ws = IndirectTransmission(Instrument=instrument, Analyser=analyser, Reflection=reflection,
                                  ChemicalFormula=formula, DensityType='Number Density', Density=density, Thickness=thickness)
        num_values = ws.column(1)
        np.testing.assert_array_almost_equal(mass_values, num_values, decimal=4)

if __name__ == "__main__":
    unittest.main()
