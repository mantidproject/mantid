# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class MuMinusExpTFTest(unittest.TestCase):

    def test_function_has_been_registered(self):
        status, msg = is_registered("MuMinusExpTF")
        if not status:
            self.fail(msg)

    def test_function_output(self):
        input = [0.0, 4.0, 8.0, 12.0]
        expected = [1.955336489125606, 0.6228986431794532, 0.1475400407753117, 0.06485370358808339]
        tolerance = 1.0e-05
        status, output = check_output("MuMinusExpTF", input, expected, tolerance, A = 1.0, Lambda = 0.1, N0 = 1.0, Tau = 5.0, Phi = 0.3, Nu = 0.2)

        if not status:
            msg = 'Computed output {} from input {} unequal to expected: {}'
            self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

    def test_do_fit(self):
        guess = dict(A = 1.1, Lambda = 0.15, N0 = 1.1, Tau = 5.0, Phi = 0.3, Nu = 0.2)
        target = dict(A = 1.0, Lambda = 0.1, N0 = 1.0, Tau = 5.0, Phi = 0.3, Nu = 0.2)
        do_a_fit(np.arange(0.1, 16, 0.2), 'MuMinusExpTF', guess, target, atol = 0.01)

if __name__ == '__main__':
	unittest.main()
