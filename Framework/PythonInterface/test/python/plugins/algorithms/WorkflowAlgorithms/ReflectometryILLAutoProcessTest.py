# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import ReflectometryILLAutoProcess
import numpy.testing
from testhelpers import (assertRaisesNothing, create_algorithm, illhelpers)
import unittest
import ReflectometryILL_common as common


class ReflectometryILLAutoProcessTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def testSingleAngle(self):
        args = {
            'DirectRun': 'ILL/D17/317369.nxs',
            'ReflectedRun': 'ILL/D17/317370.nxs',
            'OutputWorkspace': 'outWS',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        mtd.clear()

    def testMultipleAngles(self):
        args = {
            'DirectRun': 'ILL/D17/317369.nxs, ILL/D17/317369.nxs',
            'ReflectedRun': 'ILL/D17/317370.nxs, ILL/D17/317370.nxs',
            'OutputWorkspace': 'outWS',
            'GroupingQFraction': '0.5, 0.5',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        mtd.clear()

    '''
    ReflectometryILLConvertToQ - [Warning] Invalid value for DirectForegroundWorkspace: Binning
    does not match with InputWorkspace.
    def testSampleAngle(self):
        args = {
            'DirectRun': 'ILL/D17/317369.nxs',
            'ReflectedRun': 'ILL/D17/317370.nxs',
            'OutputWorkspace': 'outWS',
            'AngleOption': 'Sample angle',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLAutoProcess', **args)
        assertRaisesNothing(self, alg.execute)
        mtd.clear()
    '''


if __name__ == "__main__":
    unittest.main()
