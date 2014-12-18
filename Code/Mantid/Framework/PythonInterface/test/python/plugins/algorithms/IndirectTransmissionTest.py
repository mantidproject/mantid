import unittest
import numpy as np
import mantid.simpleapi
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
                                  ChemicalFormula=formula, NumberDensity=density, Thickness=thickness)

        # Expected values from table
        ref_result = [6.658, 0.821223, 2.58187, 53.5069, 56.0888, 0.1, 0.1, 0.566035, 0.429298]
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
                                  ChemicalFormula=formula, NumberDensity=density, Thickness=thickness)

        # Expected values from table
        ref_result = [6.26761, 0.77307, 2.58187, 53.5069, 56.0888, 0.1, 0.1, 0.566307, 0.429298]
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
        Verified reflecction validation based on analyser is working.
        """

        instrument = "IRIS"
        analyser = "graphite"
        reflection = "111"

        # Using water sample
        formula = "H2-O"

        self.assertRaises(RuntimeError, IndirectTransmission,
                          Instrument=instrument, Analyser=analyser, Reflection=reflection, ChemicalFormula=formula,
                          OutputWorkspace='__IndirectTransmissionTest_ReflectionValidation')


if __name__=="__main__":
    unittest.main()
