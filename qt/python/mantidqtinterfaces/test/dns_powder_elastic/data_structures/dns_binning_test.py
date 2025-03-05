# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

import numpy as np

from mantidqtinterfaces.dns_powder_elastic.data_structures.dns_binning import DNSBinning


class DNSBinningTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.binning = DNSBinning(xmin=0, xmax=5, step=1)

    def test___init__(self):
        self.assertEqual(self.binning.min, 0)
        self.assertEqual(self.binning.max, 5)
        self.assertEqual(self.binning.step, 1)
        self.assertEqual(self.binning.nbins, 6)
        self.assertTrue(np.array_equal(self.binning.range, np.asarray([0, 1, 2, 3, 4, 5])))
        self.assertEqual(self.binning.bin_edge_min, -0.5)
        self.assertEqual(self.binning.bin_edge_max, 5.5)
        self.assertTrue(np.array_equal(self.binning.bin_edge_range, np.asarray([-0.5, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5])))

        self.binning = DNSBinning(xmin=0, xmax=5, step=0)
        self.assertEqual(self.binning.nbins, 0)


if __name__ == "__main__":
    unittest.main()
