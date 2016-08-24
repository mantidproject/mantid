from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *


class SetDetScaleTest(unittest.TestCase):

    def testScaleDetectors(self):
        w = LoadIsawPeaks('TOPAZ_3007.peaks')
        x = w.getInstrument().getNumberParameter("detScale17")[0]
        y = w.getInstrument().getNumberParameter("detScale49")[0]
        self.assertEqual(x, 1.18898)
        self.assertEqual(y, 0.79420)
        SetDetScale(Workspace=w, DetScaleList='17:1.0,49:2.0')
        x = w.getInstrument().getNumberParameter("detScale17")[0]
        y = w.getInstrument().getNumberParameter("detScale49")[0]
        self.assertEqual(x, 1.0)
        self.assertEqual(y, 2.0)

if __name__ == '__main__':
    unittest.main()
