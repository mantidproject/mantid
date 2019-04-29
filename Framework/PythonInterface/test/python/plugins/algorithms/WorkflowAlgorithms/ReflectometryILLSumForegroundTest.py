# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from testhelpers import (assertRaisesNothing, create_algorithm, illhelpers)
import unittest


class ReflectometryILLSumForegroundTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testDirectBeamSummationExecutes(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_line_position(ws, 128.0)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testReflectedBeamSumInLambdaExecutes(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.03, 0.02)
        dirWS = illhelpers.refl_add_line_position(dirWS, 128.0)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 6.7)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS)
        args = {
            'InputWorkspace': dirWS,
            'OutputWorkspace': 'dirForeground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        dirForeground = alg.getProperty('OutputWorkspace').value
        reflWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(reflWS, 1.2)
        illhelpers.add_chopper_configuration_D17(reflWS)
        illhelpers.add_slit_configuration_D17(reflWS, 0.03, 0.02)
        reflWS = illhelpers.refl_add_line_position(reflWS, 128.0)
        reflWS = illhelpers.refl_add_two_theta(reflWS, 6.7)
        reflWS = illhelpers.refl_preprocess('refWS', reflWS)
        args = {
            'InputWorkspace': reflWS,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': dirForeground,
            'SummationType': 'SumInLambda',
            'DirectLineWorkspace': dirWS,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testReflectedBeamSumInQExecutes(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.02, 0.03)
        dirWS = illhelpers.refl_add_line_position(dirWS, 128.0)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 5.5)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS)
        args = {
            'InputWorkspace': dirWS,
            'OutputWorkspace': 'dirForeground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        dirForeground = alg.getProperty('OutputWorkspace').value
        reflWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(reflWS, 1.2)
        illhelpers.add_chopper_configuration_D17(reflWS)
        illhelpers.add_slit_configuration_D17(reflWS, 0.02, 0.03)
        reflWS = illhelpers.refl_add_line_position(reflWS, 128.0)
        reflWS = illhelpers.refl_add_two_theta(reflWS, 6.7)
        reflWS = illhelpers.refl_preprocess('refWS', reflWS)
        args = {
            'InputWorkspace': reflWS,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': dirForeground,
            'SummationType': 'SumInQ',
            'DirectLineWorkspace': dirWS,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testFigaroSumInLambdaExecutes(self):
        args = {
            'Run': 'ILL/Figaro/000002.nxs',
            'OutputWorkspace': 'direct',
            'ForegroundHalfWidth': [6, 6],
            'FlatBackground': 'Background OFF',
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        args = {
            'InputWorkspace': 'direct',
            'OutputWorkspace': 'direct-fgd'
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        args = {
            'Run': 'ILL/Figaro/000002.nxs',
            'OutputWorkspace': 'reflected',
            'ForegroundHalfWidth': [6, 6],
            'FlatBackground': 'Background OFF',
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        args = {
            'InputWorkspace': 'reflected',
            'OutputWorkspace': 'reflected-fgd',
            'DirectForegroundWorkspace': 'direct-fgd',
            'DirectLineWorkspace': 'direct'
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testWavelengthRange(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_line_position(ws, 128.0)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        xMin = 2.3
        xMax = 4.2
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'WavelengthRange': [xMin, xMax],
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        out = alg.getProperty('OutputWorkspace').value
        self.assertEquals(out.getNumberHistograms(), 1)
        Xs = out.readX(0)
        self.assertGreater(len(Xs), 1)
        self.assertGreater(Xs[0], xMin)
        self.assertLess(Xs[-1], xMax)

    def testWavelengthRangeDefault(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_line_position(ws, 128.0)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        out = alg.getProperty('OutputWorkspace').value
        self.assertEquals(out.getNumberHistograms(), 1)
        Xs = out.readX(0)
        self.assertGreater(len(Xs), 1)
        self.assertGreater(Xs[0], 0.)
        self.assertLess(Xs[-1], 30.)

    def testNoDirectForegroundAndSumInQRaises(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_line_position(ws, 128.0)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'SummationType': 'SumInQ',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        self.assertRaisesRegexp(RuntimeError, 'Some invalid Properties found', alg.execute)
        self.assertTrue(alg.isExecuted)

    def testNotSummedDirectForegroundRaises(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_line_position(ws, 128.0)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': ws,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        self.assertRaisesRegexp(RuntimeError, 'Some invalid Properties found', alg.execute)
        self.assertTrue(alg.isExecuted)

if __name__ == "__main__":
    unittest.main()
