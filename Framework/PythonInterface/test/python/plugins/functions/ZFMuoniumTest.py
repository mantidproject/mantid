# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import is_registered, check_output, do_a_fit


class ZFMuoniumTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("ZFMuonium")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.0, 4.0, 8.0, 12.0]
        expected = [0.15163664420855247, -0.05429533841911648, -0.021522983685159713, -0.02152298368515976]
        tolerance = 1.0e-05
        status, output = check_output("ZFMuonium", input, expected, tolerance, A0=0.2, FreqA=0.3, FreqD=0.2, FCut=1.0, Phi=0.0)
        if not status:
            msg = "Computed output {} from input {} unequal to expected: {}"
            self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

    def test_do_fit(self):
        guess = dict(A0=0.25, FreqA=0.35, FreqD=0.25, FCut=1.05, Phi=0.0)
        target = dict(A0=0.2, FreqA=0.3, FreqD=0.2, FCut=1.0, Phi=0.0)
        do_a_fit(np.arange(0.1, 16, 0.2), "ZFMuonium", guess, target, atol=0.01)


if __name__ == "__main__":
    unittest.main()
