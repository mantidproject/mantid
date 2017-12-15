from __future__ import (absolute_import, division, print_function)

import unittest
import testhelpers

from mantid.kernel import IntArrayOrderedPairsValidator, FloatArrayOrderedPairsValidator, \
    IntArrayProperty, FloatArrayProperty
from mantid.api import PythonAlgorithm


class ArrayOrderedPairsValidatorTest(unittest.TestCase):

    def test_fail_odd_entries(self):
        alg = self._create_alg()
        int_vals = [5,7,13]
        float_vals = [2.1]
        self.assertRaises(ValueError, alg.setProperty, "IntInput", int_vals)
        self.assertRaises(ValueError, alg.setProperty, "FloatInput", float_vals)

    def test_fail_unordered_pairs(self):
        alg = self._create_alg()
        int_vals = [5, 18, 4, 2]
        float_vals = [2.1, 5.7, 4.3, 1.5]
        self.assertRaises(ValueError, alg.setProperty, "IntInput", int_vals)
        self.assertRaises(ValueError, alg.setProperty, "FloatInput", float_vals)

    def test_pass_ordered_pairs(self):
        alg = self._create_alg()
        int_vals = [5, 18, 4, 9]
        float_vals = [2.1, 5.7, 4.3, 6.7]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "IntInput", int_vals)
        testhelpers.assertRaisesNothing(self, alg.setProperty, "FloatInput", float_vals)

    def _create_alg(self):
        """
            Creates a test algorithm with a ordered pairs validator
        """
        class TestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                int_validator = IntArrayOrderedPairsValidator()
                self.declareProperty(IntArrayProperty("IntInput", int_validator))
                float_validator = FloatArrayOrderedPairsValidator()
                self.declareProperty(FloatArrayProperty("FloatInput", float_validator))

            def PyExec(self):
                pass

        alg = TestAlgorithm()
        alg.initialize()
        return alg

if __name__ == '__main__':
    unittest.main()
