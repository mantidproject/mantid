from __future__ import (absolute_import, division, print_function)

import unittest
import os
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

        # create file for test
        filename = 'testDetScale.txt'
        hklfile = open(filename, "w")
        hklfile.write("  17  0.5\n")
        hklfile.write("  49  1.5\n")
        hklfile.close()

        SetDetScale(Workspace=w, DetScaleFile=filename)
        x = w.getInstrument().getNumberParameter("detScale17")[0]
        y = w.getInstrument().getNumberParameter("detScale49")[0]
        self.assertEqual(x, 0.5)
        self.assertEqual(y, 1.5)
        
        # test both input
        SetDetScale(Workspace=w, DetScaleList='17:1.0,49:2.0', DetScaleFile=filename)
        x = w.getInstrument().getNumberParameter("detScale17")[0]
        y = w.getInstrument().getNumberParameter("detScale49")[0]
        self.assertEqual(x, 1.0)
        self.assertEqual(y, 2.0)
        os.remove(filename)

if __name__ == '__main__':
    unittest.main()
