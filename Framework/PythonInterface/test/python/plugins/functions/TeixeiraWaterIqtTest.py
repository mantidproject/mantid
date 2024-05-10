# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import is_registered, check_output, do_a_fit


class TeixeiraWaterTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("TeixeiraWaterIqt")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [9.89878890e-01, 9.03317815e-01, 3.62579771e-01, 4.32020532e-05]
        tolerance = 1.0e-05
        status, output = check_output("TeixeiraWaterIqt", input, expected, tolerance, Q=0.4, a=0.98, Amp=1.0, Tau1=1.0, Gamma=1)
        if not status:
            msg = "Computed output {} from input {} unequal to expected: {}"
            self.fail(msg.format(*[str(a) for a in (output, input, expected)]))

    def test_do_fit(self):
        do_a_fit(
            np.arange(0.1, 2.2, 0.2),
            "TeixeiraWaterIqt",
            guess=dict(Q=0.4, a=0.98, Amp=10, Tau1=1.2, Gamma=1.2),
            target=dict(Q=0.4, a=0.98, Amp=1.0, Tau1=1.0, Gamma=1),
            atol=0.01,
        )


if __name__ == "__main__":
    unittest.main()
