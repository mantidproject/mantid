# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import MatrixWorkspace
from mantid.simpleapi import ReflectometryILLPreprocess, ReflectometryILLSumForeground, mtd
import unittest


class ReflectometryILLSumForegroundTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        ReflectometryILLPreprocess(Run = 'ILL/D17/317369.nxs',
                                   Measurement='DirectBeam',
                                   ForegroundHalfWidth=5,
                                   OutputWorkspace='db')
        ReflectometryILLPreprocess(Run='ILL/D17/317370.nxs',
                                   Measurement='ReflectedBeam',
                                   ForegroundHalfWidth=5,
                                   OutputWorkspace='rb')

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def testSumInLambdaD17(self):
        # first the direct beam
        ReflectometryILLSumForeground(InputWorkspace='db',
                                      OutputWorkspace='db_frg')

        # then the reflected beam
        ReflectometryILLSumForeground(InputWorkspace='rb',
                                      OutputWorkspace='rb_frg',
                                      SummationType='SumInLambda',
                                      DirectLineWorkspace='db',
                                      DirectForegroundWorkspace='db_frg')
        self.checkOutput(mtd['rb_frg'], 991)


    def testSumInQD17(self):
        # first the direct beam
        ReflectometryILLSumForeground(InputWorkspace='db',
                                      OutputWorkspace='db_frg')

        # then the reflected beam
        ReflectometryILLSumForeground(InputWorkspace='rb',
                                      OutputWorkspace='rb_inq_frg',
                                      SummationType='SumInQ',
                                      DirectLineWorkspace='db',
                                      DirectForegroundWorkspace='db_frg')

        self.checkOutput(mtd['rb_inq_frg'], 1045)


    def checkOutput(self, ws, blocksize):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertTrue(ws.isHistogramData())
        self.assertEquals(ws.blocksize(), blocksize)
        self.assertEquals(ws.getNumberHistograms(), 1)
        self.assertEquals(ws.getAxis(0).getUnit().unitID(), 'Wavelength')


if __name__ == "__main__":
    unittest.main()
