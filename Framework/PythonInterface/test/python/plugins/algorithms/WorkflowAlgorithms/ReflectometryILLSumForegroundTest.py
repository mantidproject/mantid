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

if __name__ == "__main__":
    unittest.main()
