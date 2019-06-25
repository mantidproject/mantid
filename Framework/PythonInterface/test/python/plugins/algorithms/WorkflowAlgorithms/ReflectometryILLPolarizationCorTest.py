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


class ReflectometryILLPolarizationCorTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testExecutes(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_flipper_configuration_D17(ws, 1, 1)
        mtd.add('ws', ws)
        ws = illhelpers.refl_add_line_position(ws, 128.0)
        ws = illhelpers.refl_add_two_theta(ws, 6.6)
        ws = illhelpers.refl_preprocess('ws', ws)
        illhelpers.refl_sum_foreground('ws', 'SumInLambda', ws)
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
