# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import mtd
from testhelpers import (assertRaisesNothing, create_algorithm)
<<<<<<< 522c2795220a09d8d4b6c923905b8c0a7f15b685
=======
#import numpy
>>>>>>> Flake 8 warnings Refs 24555
import unittest


class ReflectometryILLAutoProcessTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testSingleAngle(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'DirectRun': 'ILL/D17/317369.nxs',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertEquals(mtd.getObjectNames(),
                          ['direct-317369',
                           'direct-317369-foreground',
                           'outWS',
                           'reflected-317370',
                           'reflected-317370-foreground'])
        mtd.clear()

    def testSingleAngleMultipleFiles(self):
        args = {
            'Run': 'ILL/D17/317370+ILL/D17/317369',
            'DirectRun': 'ILL/D17/317369.nxs+ILL/D17/317370.nxs',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertEquals(mtd.getObjectNames(),
                          ['direct-317370',
                           'direct-317370-foreground',
                           'outWS',
                           'reflected-317369',
                           'reflected-317369-foreground'])
        mtd.clear()

    def testMultipleAngles(self):
        args = {
            'Run': 'ILL/D17/317370.nxs, ILL/D17/317370.nxs',
            'DirectRun': 'ILL/D17/317369.nxs, ILL/D17/317369.nxs',
            'OutputWorkspace': 'outWS',
            'GroupingQFraction': '0.5, 0.5',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(self, mtd.doesExist('outWS'))
        mtd.clear()

    def testTwoThetaInput(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'DirectRun': 'ILL/D17/317369.nxs',
            'OutputWorkspace': 'outWS',
            'TwoTheta': 30.2,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
<<<<<<< 522c2795220a09d8d4b6c923905b8c0a7f15b685
=======
        #out = mtd['outWS']
        #self.assertEquals(out.spectrumInfo().signedTwoTheta(0), 2.*30.2*numpy.pi/180.)
>>>>>>> Flake 8 warnings Refs 24555
        mtd.clear()

    def testSampleAngle(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'DirectRun': 'ILL/D17/317369.nxs',
            'OutputWorkspace': 'outWS',
            'AngleOption': 'Sample angle',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        mtd.clear()

    def testDefaultValues(self):
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'DirectRun': 'ILL/D17/317369.nxs',
            'OutputWorkspace': 'outWS',
            'rethrow': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        out = mtd['outWS']
        self.assertEquals(out.getHistory().size(), 1)
        algH = out.getHistory().getAlgorithmHistory(0)

        from ReflectometryILLAutoProcess import PropAutoProcess
        from ReflectometryILLPreprocess import Prop
        self.assertEquals(algH.getPropertyValue('AngleOption'), 'Detector angle')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.LOW_BKG_OFFSET_DIRECT), '7')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.LOW_BKG_OFFSET_REFLECTED), '7')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.LOW_BKG_WIDTH_DIRECT), '5')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.LOW_BKG_WIDTH_REFLECTED), '5')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.HIGH_BKG_OFFSET_DIRECT), '7')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.HIGH_BKG_OFFSET_REFLECTED), '7')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.HIGH_BKG_WIDTH_DIRECT), '5')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.HIGH_BKG_WIDTH_REFLECTED), '5')
        self.assertEquals(algH.getPropertyValue(Prop.SUBALG_LOGGING), 'Logging OFF')
        self.assertEquals(algH.getPropertyValue(Prop.CLEANUP), 'Cleanup ON')
        self.assertEquals(algH.getPropertyValue(Prop.SLIT_NORM), 'Slit Normalisation OFF')
        self.assertEquals(algH.getPropertyValue(Prop.FLUX_NORM_METHOD), 'Normalise To Time')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.BKG_METHOD_DIRECT), 'Background Constant Fit')
        self.assertEquals(algH.getPropertyValue(Prop.BKG_METHOD), 'Background Constant Fit')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.START_WS_INDEX_DIRECT), '0')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.START_WS_INDEX_REFLECTED), '0')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.END_WS_INDEX_DIRECT), '255')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.END_WS_INDEX_REFLECTED), '255')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.SUM_TYPE), 'SumInLambda')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.WAVELENGTH_LOWER), '0')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.WAVELENGTH_UPPER), '35')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH), '0')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH), '0')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH_DIRECT), '0')
        self.assertEquals(algH.getPropertyValue(PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH_DIRECT), '0')
        mtd.clear()

if __name__ == "__main__":
    unittest.main()
