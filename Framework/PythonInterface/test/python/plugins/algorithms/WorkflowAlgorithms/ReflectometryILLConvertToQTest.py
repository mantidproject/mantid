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


class ReflectometryILLConvertToQTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testSumInLambdaModeProducesDX(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        mtd.add('dirWS', dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.03, 0.02)
        illhelpers.add_chopper_configuration_D17(dirWS)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 2.23)
        dirWS = illhelpers.refl_add_line_position(dirWS, 128.0)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS)
        dirFgdWS = illhelpers.refl_sum_foreground('difFgdWS', 'SumInLambda', dirWS)
        reflWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(reflWS)
        illhelpers.add_slit_configuration_D17(reflWS, 0.03, 0.02)
        illhelpers.refl_rotate_detector(reflWS, 1.5)
        mtd.add('reflWS', reflWS)
        reflWS = illhelpers.refl_add_line_position(reflWS, 128.0)
        reflWS = illhelpers.refl_add_two_theta(reflWS, 6.7)
        reflWS = illhelpers.refl_preprocess('reflWS', reflWS)
        fgdWS = illhelpers.refl_sum_foreground('fgdWS', 'SumInLambda', reflWS, dirFgdWS, dirWS)
        args = {
            'InputWorkspace': fgdWS,
            'OutputWorkspace': 'inQ',
            'DirectForegroundWorkspace': dirFgdWS,
            'GroupingQFraction': 0.2,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLConvertToQ', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getNumberHistograms(), 1)
        self.assertTrue(outWS.hasDx(0))

    def testSumInQModeProducesDX(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        mtd.add('dirWS', dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.03, 0.02)
        illhelpers.add_chopper_configuration_D17(dirWS)
        dirWS = illhelpers.refl_add_line_position(dirWS, 128.0)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 2.23)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS)
        dirFgdWS = illhelpers.refl_sum_foreground('dirFgdWS', 'SumInLambda', dirWS)
        reflWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(reflWS)
        illhelpers.add_slit_configuration_D17(reflWS, 0.03, 0.02)
        illhelpers.refl_rotate_detector(reflWS, 1.5)
        mtd.add('reflWS', reflWS)
        reflWS = illhelpers.refl_add_line_position(reflWS, 128.0)
        reflWS = illhelpers.refl_add_two_theta(reflWS, 6.7)
        reflWS = illhelpers.refl_preprocess('reflWS', reflWS)
        fgdWS = illhelpers.refl_sum_foreground('fgdWS', 'SumInQ', reflWS, dirFgdWS, dirWS)
        args = {
            'InputWorkspace': fgdWS,
            'OutputWorkspace': 'inQ',
            'DirectForegroundWorkspace': dirFgdWS,
            'GroupingQFraction': 0.2,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLConvertToQ', **args)
        assertRaisesNothing(self, alg.execute)
        outWS = alg.getProperty('OutputWorkspace').value
        self.assertEqual(outWS.getNumberHistograms(), 1)
        self.assertTrue(outWS.hasDx(0))


if __name__ == "__main__":
    unittest.main()
