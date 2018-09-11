# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from testhelpers import (assertRaisesNothing, create_algorithm, illhelpers)
import unittest


class ReflectometryILLPolarizationCorTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testExecutes(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_flipper_configuration_D17(ws, 1, 1)
        mtd.add('ws', ws)
        illhelpers.refl_create_beam_position_ws('beamPosWS', ws, 0., 128)
        ws = illhelpers.refl_preprocess('ws', ws, 'beamPosWS')
        ws = illhelpers.refl_sum_in_lambda('ws', ws)
        args = {
            'InputWorkspaces': 'ws',
            'OutputWorkspace': 'corrected',
            'EfficiencyFile': 'ILL/D17/PolarizationFactors.txt',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLPolarizationCor', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(mtd.doesExist('corrected_++'))


if __name__ == "__main__":
    unittest.main()
