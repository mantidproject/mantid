# pylint: disable=no-init,invalid-name
import unittest

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import numpy as np


class PoldiLoadRunsTest(unittest.TestCase):
    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_GetActualMergeRange(self):
        rawRanges = [(1, 2, 1), (1, 2, 2), (1, 2, 3), (10, 15, 4), (10, 13, 4)]
        expectedRanges = [(1,2), (1,2), (), (10,13), (10,13)]

        alg = AlgorithmManager.create('PoldiLoadRuns')

        for raw, expected in zip(rawRanges, expectedRanges):
            mergeRange = alg.getActualMergeRange(raw)
            print mergeRange
            #self.assertItemsEqual(, expected)


if __name__ == '__main__':
    unittest.main()
