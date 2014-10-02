import unittest
import numpy as np
import mantid.simpleapi
from mantid.simpleapi import IndirectTransmission

class IndirectTransmissionTest(unittest.TestCase):

    def test_indirect_transmission(self):
        instrument = "IRIS"
        analyser = "graphite"
        reflection = "002"

        #using water sample
        formula = "H2-O"
        density = 0.1
        thickness = 0.1

        ws = IndirectTransmission(Instrument=instrument, Analyser=analyser, Reflection=reflection,
                                  ChemicalFormula=formula, NumberDensity=density, Thickness=thickness)

        #expected values from table
        ref_result = [6.658, 0.821223, 2.58187, 53.5069, 56.0888, 0.1, 0.1, 0.566035, 0.429298]
        values = ws.column(1)
        np.testing.assert_array_almost_equal(values, ref_result, decimal=4)

if __name__=="__main__":
    unittest.main()