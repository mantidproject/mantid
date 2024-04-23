# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import is_registered, check_output


class SpinDiffusionTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("SpinDiffusion")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [3.19722414e-04, 2.49714580e-04, 1.35235417e-04, 3.03475905e-05]
        tolerance = 1.0e-05
        status, output = check_output("SpinDiffusion", input, expected, tolerance, A=1.0, DParallel=1e3, DPerpendicular=1e-2, NDimensions=2)
        if not status:
            msg = "Computed output {} from input {} unequal to expected: {}"
            self.fail(msg.format(*[str(a) for a in (output, input, expected)]))


if __name__ == "__main__":
    unittest.main()
