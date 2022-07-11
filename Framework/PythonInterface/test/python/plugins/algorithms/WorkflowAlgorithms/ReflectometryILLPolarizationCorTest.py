# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import (ReflectometryILLPreprocess, ReflectometryILLSumForeground, ReflectometryILLPolarizationCor, mtd)
import unittest


class ReflectometryILLPolarizationCorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        ReflectometryILLPreprocess(Run='ILL/D17/317369.nxs',
                                   Measurement='DirectBeam',
                                   ForegroundHalfWidth=5,
                                   OutputWorkspace='db')
        ReflectometryILLPreprocess(Run='ILL/D17/317370.nxs',
                                   Measurement='ReflectedBeam',
                                   ForegroundHalfWidth=5,
                                   OutputWorkspace='rb')
        # first the direct beam
        ReflectometryILLSumForeground(InputWorkspace='db',
                                      OutputWorkspace='db_frg')

        # then the reflected beam
        ReflectometryILLSumForeground(InputWorkspace='rb',
                                      OutputWorkspace='rb_frg',
                                      SummationType='SumInLambda',
                                      DirectLineWorkspace='db',
                                      DirectForegroundWorkspace='db_frg')

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def testExecutes(self):
        ReflectometryILLPolarizationCor(
            InputWorkspaces='rb_frg',
            OutputWorkspace='pol_corrected',
            EfficiencyFile='ILL/D17/PolarizationFactors.txt'
        )
        self.checkOutput(mtd['pol_corrected'], 1, 991)

    def checkOutput(self, ws, items, blocksize):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), items)
        item = ws[0]
        self.assertTrue(isinstance(item, MatrixWorkspace))
        self.assertTrue(item.isHistogramData())
        self.assertEqual(item.blocksize(), blocksize)
        self.assertEqual(item.getNumberHistograms(), 1)
        self.assertEqual(item.getAxis(0).getUnit().unitID(), 'Wavelength')

if __name__ == "__main__":
    unittest.main()
