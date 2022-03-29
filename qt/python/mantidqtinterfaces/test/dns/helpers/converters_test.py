# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantidqtinterfaces.dns.helpers.converters import (
    lambda_to_energy, twotheta_to_q)


class DNSconvertersTest(unittest.TestCase):

    def setUp(self):
        pass

    def test_lambda_to_E(self):
        energy = lambda_to_energy(4.74)
        self.assertAlmostEqual(energy, 3.64098566)
        with self.assertRaises(ZeroDivisionError):
            lambda_to_energy(0)

    def test_twotheta_to_q(self):
        qabs = twotheta_to_q(120, 4.74, 0)
        self.assertAlmostEqual(qabs, 2.29594856)
        qabs = twotheta_to_q(120, 4.74, 3)
        self.assertAlmostEqual(qabs, 1.67443094)
        with self.assertRaises(ZeroDivisionError):
            twotheta_to_q(120, 0, 0)


if __name__ == '__main__':
    unittest.main()
