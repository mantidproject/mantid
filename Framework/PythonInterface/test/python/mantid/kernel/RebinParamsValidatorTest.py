# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest
import testhelpers

from mantid.kernel import RebinParamsValidator, FloatArrayProperty
from mantid.api import PythonAlgorithm


class RebinParamsValidatorTest(unittest.TestCase):

    def test_default_constructor(self):
        alg = self._create_alg_with_RebinParamsValidator()
        input = []
        self.assertRaises(ValueError, alg.setProperty, "Input", input)
        input = [0.7,1.3]
        self.assertRaises(ValueError, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9,1.5]
        self.assertRaises(ValueError, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9,1.5,5.1]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)

    def test_allow_empty(self):
        alg = self._create_alg_with_RebinParamsValidator(True)
        input = []
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)
        input = [0.7,1.3]
        self.assertRaises(ValueError, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9,1.5]
        self.assertRaises(ValueError, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9,1.5,5.1]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)

    def test_allow_empty_and_allow_ranges(self):
        alg = self._create_alg_with_RebinParamsValidator(True, True)
        input = []
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)
        input = [0.7,1.3]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9,1.5]
        self.assertRaises(ValueError, alg.setProperty, "Input", input)
        input = [0.7,0.2,2.9,1.5,5.1]
        testhelpers.assertRaisesNothing(self, alg.setProperty, "Input", input)

    def _create_alg_with_RebinParamsValidator(self, allow_empty=None, allow_ranges=None):
        """
            Creates a test algorithm with the validator
        """
        class TestAlgorithm(PythonAlgorithm):

            def PyInit(self):
                if not allow_empty:
                    if not allow_ranges:
                        validator = RebinParamsValidator()
                else:
                    if not allow_ranges:
                        validator = RebinParamsValidator(allow_empty)
                    else:
                        validator = RebinParamsValidator(allow_empty, allow_ranges)
                self.declareProperty(FloatArrayProperty("Input", validator))

            def PyExec(self):
                pass

        alg = TestAlgorithm()
        alg.initialize()
        return alg

if __name__ == '__main__':
    unittest.main()
