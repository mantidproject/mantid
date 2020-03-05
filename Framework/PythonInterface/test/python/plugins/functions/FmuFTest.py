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


class FmuFTest(unittest.TestCase):

	def test_function_has_been_registered(self):
		status, msg = is_registered("FmuF")
		if not status:
			self.fail(msg)

	def test_function_output(self):
		input = [0.0, 4.0, 8.0, 12.0]
		expected = [0.5, 0.12404101755414361, 0.040649467437777045, 0.0034489777749940457]
		tolerance = 1.0e-05
		status, output = check_output("FmuF", input, expected, tolerance, A0 = 0.5, FreqD = 0.2, Lambda = 0.1, Sigma = 0.2)
		if not status:
			msg = 'Computed output {} from input {} unequal to expected: {}'
			self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

	def test_do_fit(self):
		do_a_fit(np.arange(0.1, 16, 0.2), 'FmuF', guess = dict(A0 = 0.55, FreqD = 0.25, Lambda = 0.15, Sigma = 0.25), target = dict(A0 = 0.5, FreqD = 0.2, Lambda = 0.1, Sigma = 0.2), atol = 0.01)

if __name__ == '__main__':
	unittest.main()
