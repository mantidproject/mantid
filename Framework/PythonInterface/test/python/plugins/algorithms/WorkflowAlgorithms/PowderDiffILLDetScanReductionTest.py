from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import PowderDiffILLDetScanReduction, config, mtd


# This is just a light test for the default options.
# More options are covered by system tests, since it takes too long for a unit test.
class PowderDiffILLDetScanReductionTest(unittest.TestCase):

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D2B'
        config.appendDataSearchSubDir('ILL/D2B/')

    def tearDown(self):
        mtd.clear()

    def test_default_options_d2b(self):
        red = PowderDiffILLDetScanReduction(Run='508093')
        self.assertTrue(red)
        self.assertTrue(isinstance(red, WorkspaceGroup))
        self.assertEquals(red.getNumberOfEntries(), 1)
        item = red.getItem(0)
        self.assertTrue(item)
        self.assertTrue(isinstance(item, MatrixWorkspace))
        self.assertTrue(item.isHistogramData())
        self.assertTrue(not item.isDistribution())
        self.assertEquals(item.getNumberHistograms(),1)
        self.assertEquals(item.blocksize(),2975)
        xaxis = item.getAxis(0).extractValues()
        xunit = item.getAxis(0).getUnit().label()
        self.assertEquals(xunit,'degrees')
        self.assertAlmostEqual(xaxis[0],-0.029,3)
        self.assertAlmostEqual(xaxis[1],0.021,3)
        self.assertAlmostEqual(xaxis[-1],148.721,3)
        spectrumaxis = item.getAxis(1).extractValues()
        self.assertEquals(spectrumaxis[0],0)

if __name__ == '__main__':
    unittest.main()
