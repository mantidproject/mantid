# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import is_registered, check_output, do_a_fit


class TeixeiraWaterIqtFTTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("TeixeiraWaterIqtFT")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [0.47663363, 0.4657655, 0.14724691, 0.00261668]
        tolerance = 1.0e-05
        status, output = check_output(
            "TeixeiraWaterIqtFT",
            input,
            expected,
            tolerance,
            Q=0.4,
            a=0.98,
            Amp=1.0,
            Tau1=1.0,
            Gamma=1,
        )
        if not status:
            msg = "Computed output {} from input {} unequal to expected: {}"
            self.fail(msg.format(*[str(a) for a in (output, input, expected)]))

    def test_do_fit(self):
        do_a_fit(
            np.arange(0.1, 2.2, 0.2),
            "TeixeiraWaterIqtFT",
            guess=dict(Q=0.4, a=0.98, Amp=10, Tau1=1.2, Gamma=1.2),
            target=dict(Q=0.4, a=0.98, Amp=1.0, Tau1=1.0, Gamma=1),
            atol=0.01,
        )


if __name__ == "__main__":
    unittest.main()
