# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid import AnalysisDataServiceImpl, simpleapi


class SortByQVectorsTest(unittest.TestCase):
    def test_output(self):
        ws = simpleapi.LoadSassena("outputSassena_1.4.1.h5", TimeUnit=1.0)
        simpleapi.SortByQVectors('ws')
        self.assertAlmostEqual(ws[0].getNumberHistograms(), 5)
        self.assertAlmostEqual(ws[0].dataY(0)[0], 0.0)
        self.assertAlmostEqual(ws[0].dataY(1)[0], 0.00600600591861)
        self.assertAlmostEqual(ws[0].dataY(2)[0], 0.0120120118372)
        self.assertAlmostEqual(ws[0].dataY(3)[0], 0.0180180184543)
        self.assertAlmostEqual(ws[0].dataY(4)[0], 0.0240240236744)
        AnalysisDataServiceImpl.Instance().remove("ws")


if __name__ == "__main__":
    unittest.main()
