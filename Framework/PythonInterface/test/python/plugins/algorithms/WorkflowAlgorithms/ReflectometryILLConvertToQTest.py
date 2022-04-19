# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import MatrixWorkspace
from mantid.simpleapi import (ReflectometryILLPreprocess, ReflectometryILLSumForeground, ReflectometryILLConvertToQ, mtd)
import unittest


class ReflectometryILLConvertToQTest(unittest.TestCase):

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

        # then the reflected beam in lambda
        ReflectometryILLSumForeground(InputWorkspace='rb',
                                      OutputWorkspace='rb_frg',
                                      SummationType='SumInLambda',
                                      DirectLineWorkspace='db',
                                      DirectForegroundWorkspace='db_frg')

        # then the reflected beam in q
        ReflectometryILLSumForeground(InputWorkspace='rb',
                                      OutputWorkspace='rb_inq_frg',
                                      SummationType='SumInQ',
                                      DirectLineWorkspace='db',
                                      DirectForegroundWorkspace='db_frg')

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def testD17InLambda(self):
        ReflectometryILLConvertToQ(
            InputWorkspace='rb_frg',
            OutputWorkspace='in_lambda',
            DirectForegroundWorkspace='db_frg'
        )
        self.checkOutput(mtd['in_lambda'], 991)

    def testD17InQ(self):
        ReflectometryILLConvertToQ(
            InputWorkspace='rb_inq_frg',
            OutputWorkspace='in_q',
            DirectForegroundWorkspace='db_frg'
        )
        self.checkOutput(mtd['in_q'], 1045)

    def checkOutput(self, ws, blocksize):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertFalse(ws.isHistogramData())
        self.assertEqual(ws.blocksize(), blocksize)
        self.assertEqual(ws.getNumberHistograms(), 1)
        self.assertTrue(ws.hasDx(0))
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'MomentumTransfer')

if __name__ == "__main__":
    unittest.main()
