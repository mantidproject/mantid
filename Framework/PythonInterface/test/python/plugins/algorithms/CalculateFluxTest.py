# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import CalculateFlux, CreateSampleWorkspace, FindDetectorsInShape, mtd
import numpy as np
import testhelpers

class CalculateFluxTest(unittest.TestCase):

    pixels_in_shape = 0

    def setUp(self):
        ws = CreateSampleWorkspace(XUnit="Wavelength", NumBanks=1, Function="One Peak")
        shape = FindDetectorsInShape(Workspace=ws,
                                     ShapeXML='<infinite-cylinder id="asbsolute_scale">'
                                              '<centre x="0.0" y="0.0" z="0.0" /> <axis x="0.0" y="0.0" z="1.0" />'
                                              '<radius val="0.05" /></infinite-cylinder>')
        self.pixels_in_shape = len(shape)

    def tearDown(self):
        mtd.clear()

    def testTOF(self):

        CalculateFlux(InputWorkspace="ws", BeamRadius=0.05, OutputWorkspace="flux")
        self.assertTrue(mtd["flux"])
        self.assertEqual(mtd["flux"].getNumberHistograms(), 1)
        self.assertEqual(mtd["flux"].blocksize(), mtd["ws"].blocksize())
        self.assertEqual(mtd["flux"].getAxis(0).getUnit().unitID(), "Wavelength")
        expectation = np.empty(100)
        expectation.fill(self.pixels_in_shape * 0.3)
        expectation[50] = self.pixels_in_shape * 10.3
        reality = mtd["flux"].readY(0)
        testhelpers.assert_almost_equal(reality, expectation, decimal=6)

if __name__ == "__main__":
    unittest.main()
