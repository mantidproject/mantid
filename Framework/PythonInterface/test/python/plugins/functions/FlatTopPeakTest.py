# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import is_registered, check_output, do_a_fit


class FlatTopPeakTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("FlatTopPeak")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.0, 4.0, 8.0, 12.0]
        expected = [2500.0, 2500.0, 2500.0, 2500.0]
        tolerance = 1.0e-05
        status, output = check_output(
            "FlatTopPeak", input, expected, tolerance, Scale=-2400.0, Centre=50.0, EndGrad=3.0, Background=2500.0, Width=20.0
        )

        if not status:
            msg = "Computed output {} from input {} unequal to expected: {}"
            self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

    def test_do_fit(self):
        guess = dict(Scale=-2400.0, Centre=50.0, EndGrad=3.0, Background=2500.0, Width=20.0)
        target = dict(Scale=-2400.0, Centre=50.0, EndGrad=3.0, Background=2500.0, Width=20.0)
        do_a_fit(np.arange(0.1, 16, 0.2), "FlatTopPeak", guess, target, atol=0.01)


if __name__ == "__main__":
    unittest.main()
