# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from MsdTestHelper import is_registered, check_output, do_a_fit


class EISFDiffCylinderTest(unittest.TestCase):
    def test_function_has_been_registered(self):
        status, msg = is_registered("EISFDiffCylinder")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.01, 0.1, 1.0, 10.0]
        expected = [9.93604824e-01, 9.69451382e-01, 5.88013757e-02, 3.89828036e-06]
        tolerance = 1.0e-05
        status, output = check_output("EISFDiffCylinder", input, expected, tolerance, A=1.0, R=3.5)
        if not status:
            msg = "Computed output {} from input {} unequal to expected: {}"
            self.fail(msg.format(*[str(a) for a in (output, input, expected)]))

    def test_do_fit(self):
        target = dict(A=1.0, R=3.5, L=1.7)
        # we need only one parameter when fitting only one histogram,
        # else we incur in overfitting
        guesses = (dict(A=1.0, R=3.5, L=3.0), dict(A=1.0, R=0.5, L=1.7))
        fixes = (["A", "R"], ["A", "L"])
        for guess, fix in zip(guesses, fixes):
            status, fit = do_a_fit(np.arange(0.1, 2.2, 0.2), "EISFDiffCylinder", guess=guess, fixes=fix, target=target, atol=0.01)
            if not status:
                msg_p = "param {} target value was {}, obtained = {}"
                msg = "\n".join([msg_p.format(*[p, target[p], fit.Function[p]]) for p in target])
                self.fail("\n" + msg)


if __name__ == "__main__":
    unittest.main()
