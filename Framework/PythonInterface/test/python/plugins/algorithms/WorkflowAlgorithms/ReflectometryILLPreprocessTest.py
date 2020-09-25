# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import mtd
from mantid.simpleapi import ReflectometryILLPreprocess
import numpy.testing
from testhelpers import (assertRaisesNothing, assert_almost_equal, create_algorithm, illhelpers)
import unittest
import ReflectometryILL_common as common
import math


class ReflectometryILLPreprocessTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def testDirectBeamD17(self):
        args = {
            'Run': 'ILL/D17/317369.nxs',
            'Measurement': 'DirectBeam',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 202.177, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight))/2.
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 0., delta=0.1)

    def testReflectedBeamUserAngleD17(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'Measurement': 'ReflectedBeam',
            'AngleOption': 'UserAngle',
            'BraggAngle': 1.5,
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 201.674, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight))/2.
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 3.0, delta=0.01)

    def testReflectedBeamSanAngleD17(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'Measurement': 'ReflectedBeam',
            'AngleOption': 'SampleAngle',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 201.674, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight))/2.
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 1.6, delta=0.01)

    def testReflectedBeamDanAngleD17(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'Measurement': 'ReflectedBeam',
            'AngleOption': 'DetectorAngle',
            'DirectBeamForegroundCentre': 202.177,
            'DirectBeamDetectorAngle': 0.1,
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 201.674, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight))/2.
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 3.097, delta=0.01)

    def testDefaultRunFIGARO(self):
        args = {
            'Run': 'ILL/Figaro/000002.nxs',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        fgCentre = outWS.run().getProperty(common.SampleLogs.LINE_POSITION).value
        self.assertAlmostEqual(fgCentre, 127.801, delta=0.001)
        peakLeft = math.floor(fgCentre)
        peakRight = peakLeft + 1
        two_theta_fg = (outWS.spectrumInfo().twoTheta(peakLeft) + outWS.spectrumInfo().twoTheta(peakRight)) / 2.
        self.assertAlmostEqual(numpy.rad2deg(two_theta_fg), 0., delta=0.1)

    def testTwoInputFiles(self):
        outWSName = 'outWS'
        args = {
            'Run': 'ILL/D17/317369, ILL/D17/317370.nxs',
            'OutputWorkspace': outWSName,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getAxis(0).getUnit().caption(), 'Wavelength')
        self.assertEqual(mtd.getObjectNames(), [])

if __name__ == "__main__":
    unittest.main()
