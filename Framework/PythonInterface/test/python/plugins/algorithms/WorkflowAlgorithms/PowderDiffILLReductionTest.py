from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import PowderDiffILLReduction
from mantid import config, mtd


class PowderDiffILLReductionTest(unittest.TestCase):

    _runs = '967087:967088'

    def setUp(self):
        config.appendDataSearchSubDir('ILL/D20')
        config.setFacility('ILL')

    def tearDown(self):
        mtd.remove('red')

    def test_default_options(self):
        red = PowderDiffILLReduction(Run=self._runs)
        self.assertTrue(red)
        self.assertTrue(not red.isDistribution())
        self.assertTrue(not red.isHistogramData())
        self.assertEquals(red.getNumberHistograms(),2)
        self.assertEquals(red.blocksize(),3072)
        xaxis = red.getAxis(0).extractValues()
        xunit = red.getAxis(0).getUnit().unitID()
        self.assertEquals(xunit,'Degrees')
        self.assertAlmostEqual(xaxis[0],-2.7966,4)
        self.assertAlmostEqual(xaxis[-1],150.7534,4)
        spectrumaxis = red.getAxis(1).extractValues()
        self.assertAlmostEqual(spectrumaxis[0],253.924,5)
        self.assertAlmostEqual(spectrumaxis[1],242.82001,5)
        self.assertEquals(red.readY(0)[1400],32538)
        self.assertEquals(red.readY(1)[2100],9038)

    def test_sort_temperature_axis(self):
        red = PowderDiffILLReduction(Run=self._runs,SortObservableAxis=True)
        self.assertTrue(red)
        spectrumaxis = red.getAxis(1).extractValues()
        self.assertAlmostEqual(spectrumaxis[0],242.82001,5)
        self.assertAlmostEqual(spectrumaxis[1],253.924,5)

    def test_momentum_transfer(self):
        red = PowderDiffILLReduction(Run=self._runs,Unit='MomentumTransfer')
        self.assertTrue(red)
        xunit = red.getAxis(0).getUnit().unitID()
        self.assertEquals(xunit,'MomentumTransfer')

    def test_dspacing(self):
        red = PowderDiffILLReduction(Run=self._runs,Unit='dSpacing')
        self.assertTrue(red)
        xunit = red.getAxis(0).getUnit().unitID()
        self.assertEquals(xunit,'dSpacing')

    def test_normalise_monitor(self):
        red = PowderDiffILLReduction(Run=self._runs,NormaliseTo='Monitor')
        self.assertTrue(red)
        self.assertAlmostEquals(red.readY(0)[1400],0.01186,5)
        self.assertAlmostEquals(red.readY(1)[2100],0.0031,5)

    def test_normalise_time(self):
        red = PowderDiffILLReduction(Run=self._runs,NormaliseTo='Time')
        self.assertTrue(red)
        self.assertAlmostEquals(red.readY(0)[1400],108.46,2)
        self.assertAlmostEquals(red.readY(1)[2100],30.13,2)

    def test_normalise_roi(self):
        red = PowderDiffILLReduction(Run=self._runs,NormaliseTo='ROI',ROI='0,100')
        self.assertTrue(red)
        self.assertAlmostEquals(red.readY(0)[1400],0.00189,5)
        self.assertAlmostEquals(red.readY(1)[2100],0.00049,5)

    def test_fullprof(self):
        red = PowderDiffILLReduction(Run=self._runs,PrepareToSaveAs='FullProf')
        self.assertTrue(red)
        self.assertEquals(red.blocksize(),3002)
        xaxis = red.getAxis(0).extractValues()
        self.assertAlmostEqual(xaxis[0],0.4034,4)
        self.assertAlmostEqual(xaxis[-1],150.7534,4)

if __name__ == '__main__':
    unittest.main()
