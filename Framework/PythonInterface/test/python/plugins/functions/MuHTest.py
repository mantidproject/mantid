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


class MuHTest(unittest.TestCase):

	def test_function_has_been_registered(self):
		status, msg = is_registered("MuH")
		if not status:
			self.fail(msg)

	def test_function_output(self):
		input = [0.0, 4.0, 8.0, 12.0]
		expected = [0.5, 0.1476150834620071, 0.04187161279609799, 0.011411345712546492]
		tolerance = 1.0e-05
		status, output = check_output("MuH", input, expected, tolerance, A0 = 0.5, NuD = 0.5, Lambda = 0.3, Sigma = 0.05, Phi = 0.0)
		if not status:
			msg = 'Computed output {} from input {} unequal to expected: {}'
			self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

	def test_do_fit(self):
		do_a_fit(np.arange(0.1, 16, 0.2), 'MuH', guess = dict(A0 = 0.55, NuD = 0.5, Lambda = 0.3, Sigma = 0.05, Phi = 0.0), target = dict(A0 = 0.5, NuD = 0.5, Lambda = 0.3, Sigma = 0.05, Phi = 0.0), atol = 0.01)

if __name__ == '__main__':
	unittest.main()
