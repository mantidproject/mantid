# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (ReflectometryILLPreprocess, ReflectometryILLReduction)
from testhelpers import (assertRaisesNothing, create_algorithm)
import unittest

class ReflectometryILLReductionTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def testDefaultRunExecutesSuccessfully(self):
        preprocessOutWSName = 'preprocessedWS'
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'OutputWorkspace': preprocessOutWSName,
            'ForegroundHalfWidth': 0,
            'rethrow': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        outWSName = 'outWS'
        args = {
            'InputWorkspace': preprocessOutWSName,
            'OutputWorkspace': outWSName,
            'rethrow': True
        }
        alg = create_algorithm('ReflectometryILLReduction', **args)
        assertRaisesNothing(self, alg.execute)

if __name__ == "__main__":
    unittest.main()
