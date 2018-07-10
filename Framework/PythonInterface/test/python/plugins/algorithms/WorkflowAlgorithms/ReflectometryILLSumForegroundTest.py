# -*- coding: utf-8 -*-

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
        beamPosWS = illhelpers.refl_create_beam_position_ws('beamPosWS', ws, 1.2, 128)
        ws = illhelpers.refl_preprocess('ws', ws, beamPosWS)
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
        dirBeamPosWS = illhelpers.refl_create_beam_position_ws('dirBeamPosWS', dirWS, 0., 128)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS, dirBeamPosWS)
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
        reflBeamPosWS = illhelpers.refl_create_beam_position_ws('reflBeamPosWS', reflWS, 1.2, 128)
        reflWS = illhelpers.refl_preprocess('refWS', reflWS, reflBeamPosWS)
        args = {
            'InputWorkspace': reflWS,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': dirForeground,
            'SummationType': 'SumInLambda',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testReflectedBeamSumInQExecutes(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        dirBeamPosWS = illhelpers.refl_create_beam_position_ws('dirBeamPosWS', dirWS, 0., 128)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS, dirBeamPosWS)
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
        reflBeamPosWS = illhelpers.refl_create_beam_position_ws('reflBeamPosWS', reflWS, 1.2, 128)
        reflWS = illhelpers.refl_preprocess('refWS', reflWS, reflBeamPosWS)
        args = {
            'InputWorkspace': reflWS,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': dirForeground,
            'SummationType': 'SumInQ',
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
            'DirectForegroundWorkspace': 'direct-fgd'
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testWavelengthRange(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        beamPosWS = illhelpers.refl_create_beam_position_ws('beamPosWS', ws, 1.2, 128)
        ws = illhelpers.refl_preprocess('ws', ws, beamPosWS)
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
        ws = alg.getProperty('OutputWorkspace').value
        self.assertEquals(ws.getNumberHistograms(), 1)
        Xs = ws.readX(0)
        self.assertGreater(len(Xs), 1)
        self.assertGreater(Xs[0], xMin)
        self.assertLess(Xs[-1], xMax)


if __name__ == "__main__":
    unittest.main()
