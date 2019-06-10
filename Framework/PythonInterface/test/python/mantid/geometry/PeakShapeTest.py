# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import CreatePeaksWorkspace, CreateSampleWorkspace
from mantid.geometry import PeakShape

class PeakShapeTest(unittest.TestCase):

    def test_basic_access(self):
        sampleWs = CreateSampleWorkspace()
        ws = CreatePeaksWorkspace(InstrumentWorkspace=sampleWs,NumberOfPeaks=1)
        peak = ws.getPeak(0)
        peak_shape = peak.getPeakShape()
        self.assertTrue(isinstance(peak_shape, PeakShape))
        self.assertEqual(peak_shape.shapeName(), "none")
    

if __name__ == '__main__':
    unittest.main()