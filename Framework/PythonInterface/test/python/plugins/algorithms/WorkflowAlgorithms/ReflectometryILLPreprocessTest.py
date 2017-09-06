# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from mantid.simpleapi import (ReflectometryILLPreprocess)
from testhelpers import (assertRaisesNothing, create_algorithm)
import unittest

class ReflectometryILLPreprocessTest(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def testDefaultRunExecutesSuccessfully(self):
        outWSName = 'outWS'
        args = {
            'Run': 'ILL/D17/317370.nxs',
            'OutputWorkspace': outWSName,
            'rethrow': True
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)


if __name__ == "__main__":
    unittest.main()
