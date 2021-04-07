# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from MsdTestHelper import (is_registered, check_output, do_a_fit)


class StandardSCTest(unittest.TestCase):

	def test_function_has_been_registered(self):
		status, msg = is_registered("StandardSC")
		if not status:
			self.fail(msg)

	def test_function_output(self):
		input = [0.0, 4.0, 8.0, 12.0]
		expected = [0.6, -0.43265208, 0.15521027, 0.0046213]
		tolerance = 1.0e-05
		status, output = check_output("StandardSC", input, expected, tolerance, A0 = 0.5, FieldSC = 10, FieldBG = 11, Phi = 0.0, Sigma = 0.2, Abg = 0.1)
		if not status:
			msg = 'Computed output {} from input {} unequal to expected: {}'
			self.fail(msg.format(*[str(i) for i in (output, input, expected)]))

	def test_do_fit(self):
		do_a_fit(np.arange(0.1, 16, 0.2), 'StandardSC', guess = dict(A0 = 0.5, FieldSC = 10, FieldBG = 11, Phi = 0.05, Sigma = 0.2, Abg = 0.1), target = dict(A0 = 0.5, FieldSC = 10, FieldBG = 11, Phi = 0.0, Sigma = 0.2, Abg = 0.1), atol = 0.01)

if __name__ == '__main__':
	unittest.main()
