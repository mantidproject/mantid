from __future__ import (absolute_import, print_function)

import unittest
from testhelpers import run_algorithm
from mantid import mtd
import mantid

class MDEventWorkspaceTest(unittest.TestCase):
    """
    Test the interface to MDEventWorkspaces
    """

    def setUp(self):
        run_algorithm('CreateMDWorkspace', Dimensions='3',Extents='0,10,0,10,0,10',Names='x,y,z',Units='m,m,m',SplitInto='5',
                      MaxRecursionDepth='20',OutputWorkspace='mdw')
        run_algorithm('FakeMDEventData', InputWorkspace="mdw",  UniformParams="1e4")
        

    def tearDown(self):
        mtd.remove('mdw')

    def test_interface(self):
        mdw= mtd['mdw']
        self.assertEqual(mdw.displayNormalization(),mantid.api.MDNormalization.VolumeNormalization)
        mdw.setDisplayNormalization(mantid.api.MDNormalization.NoNormalization)
        self.assertEqual(mdw.displayNormalization(),mantid.api.MDNormalization.NoNormalization)
        mdw.setDisplayNormalizationHisto(mantid.api.MDNormalization.NumEventsNormalization)
        self.assertEqual(mdw.displayNormalizationHisto(),mantid.api.MDNormalization.NumEventsNormalization)

if __name__ == '__main__':
    unittest.main()

