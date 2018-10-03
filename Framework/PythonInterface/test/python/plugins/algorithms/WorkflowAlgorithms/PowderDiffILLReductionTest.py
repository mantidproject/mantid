# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import PowderDiffILLReduction
from mantid import config, mtd


class PowderDiffILLReductionTest(unittest.TestCase):

    _runs = '967087:967088'

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')

    def tearDown(self):
        mtd.remove('red')

    def test_default_options(self):
        red = PowderDiffILLReduction(Run=self._runs)
        self.assertTrue(red)
        self.assertTrue(not red.isDistribution())
        self.assertTrue(not red.isHistogramData())
        self.assertEquals(red.getNumberHistograms(),2)
        self.assertEquals(red.blocksize(),3008)
        xaxis = red.getAxis(0).extractValues()
        xunit = red.getAxis(0).getUnit().unitID()
        self.assertEquals(xunit,'Degrees')
        self.assertAlmostEqual(xaxis[0],0.4034,4)
        self.assertAlmostEqual(xaxis[-1],150.7534,4)
        spectrumaxis = red.getAxis(1).extractValues()
        self.assertAlmostEqual(spectrumaxis[0],253.924,5)
        self.assertAlmostEqual(spectrumaxis[1],242.82001,5)
        self.assertEquals(red.readY(0)[0],644)
        self.assertAlmostEqual(red.readE(0)[0],25.3772,4)
        self.assertEquals(red.readY(0)[3007], 8468)
        self.assertAlmostEqual(red.readE(0)[3007],92.0217,4)
        self.assertEquals(red.readY(1)[1],1105)
        self.assertAlmostEqual(red.readE(1)[1],33.2415,4)
        self.assertEquals(red.readY(0)[1400],9532)
        self.assertEquals(red.readY(1)[2100],9789)

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
        self.assertAlmostEquals(red.readY(0)[1400],0.00348,5)
        self.assertAlmostEquals(red.readY(1)[2100],0.00335,5)

    def test_normalise_time(self):
        red = PowderDiffILLReduction(Run=self._runs,NormaliseTo='Time')
        self.assertTrue(red)
        self.assertAlmostEquals(red.readY(0)[1400],9532/300,4)
        self.assertAlmostEquals(red.readY(1)[2100],9789/300,2)

    def test_normalise_roi(self):
        red = PowderDiffILLReduction(Run=self._runs,NormaliseTo='ROI',ROI='0,100')
        self.assertTrue(red)
        self.assertAlmostEquals(red.readY(0)[1400],0.00055,5)
        self.assertAlmostEquals(red.readY(1)[2100],0.00053,5)

    def test_crop_zero_counting_cells(self):
        red = PowderDiffILLReduction(Run=self._runs,ZeroCountingCells='Crop')
        self.assertTrue(red)
        self.assertEquals(red.blocksize(), 3002)

    def test_rebin(self):
        red = PowderDiffILLReduction(Run=self._runs,ScanAxisBinWidth=12,SortObservableAxis=True)
        self.assertEquals(red.getNumberHistograms(), 1)
        self.assertAlmostEqual(red.getAxis(1).extractValues()[0], 248.372, 5)

if __name__ == '__main__':
    unittest.main()
