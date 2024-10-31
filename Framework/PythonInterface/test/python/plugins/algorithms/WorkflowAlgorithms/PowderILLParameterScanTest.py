# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import PowderILLParameterScan
from mantid import config, mtd


class PowderILLParameterScanTest(unittest.TestCase):
    _runs = "967087:967088"

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir("ILL/D20/")

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D20"

    def tearDown(self):
        mtd.remove("red")

    def test_default_options(self):
        red = PowderILLParameterScan(Run=self._runs)
        self.assertTrue(red)
        self.assertTrue(not red.isDistribution())
        self.assertTrue(not red.isHistogramData())
        self.assertEqual(red.getNumberHistograms(), 2)
        self.assertEqual(red.blocksize(), 3008)
        xaxis = red.getAxis(0).extractValues()
        xunit = red.getAxis(0).getUnit().unitID()
        self.assertEqual(xunit, "Degrees")
        self.assertAlmostEqual(xaxis[0], 0.4034, 4)
        self.assertAlmostEqual(xaxis[-1], 150.7534, 4)
        spectrumaxis = red.getAxis(1).extractValues()
        self.assertAlmostEqual(spectrumaxis[0], 253.924, 5)
        self.assertAlmostEqual(spectrumaxis[1], 242.82001, 5)
        self.assertEqual(red.readY(0)[0], 644)
        self.assertAlmostEqual(red.readE(0)[0], 25.3772, 4)
        self.assertEqual(red.readY(0)[3007], 8468)
        self.assertAlmostEqual(red.readE(0)[3007], 92.0217, 4)
        self.assertEqual(red.readY(1)[1], 1105)
        self.assertAlmostEqual(red.readE(1)[1], 33.2415, 4)
        self.assertEqual(red.readY(0)[1400], 9532)
        self.assertEqual(red.readY(1)[2100], 9789)

    def test_sort_temperature_axis(self):
        red = PowderILLParameterScan(Run=self._runs, SortObservableAxis=True)
        self.assertTrue(red)
        spectrumaxis = red.getAxis(1).extractValues()
        self.assertAlmostEqual(spectrumaxis[0], 242.82001, 5)
        self.assertAlmostEqual(spectrumaxis[1], 253.924, 5)

    def test_momentum_transfer(self):
        red = PowderILLParameterScan(Run=self._runs, Unit="MomentumTransfer")
        self.assertTrue(red)
        xunit = red.getAxis(0).getUnit().unitID()
        self.assertEqual(xunit, "MomentumTransfer")

    def test_dspacing(self):
        red = PowderILLParameterScan(Run=self._runs, Unit="dSpacing")
        self.assertTrue(red)
        xunit = red.getAxis(0).getUnit().unitID()
        self.assertEqual(xunit, "dSpacing")

    def test_normalise_monitor(self):
        red = PowderILLParameterScan(Run=self._runs, NormaliseTo="Monitor")
        self.assertTrue(red)
        self.assertAlmostEqual(red.readY(0)[1400], 0.00348, 5)
        self.assertAlmostEqual(red.readY(1)[2100], 0.00335, 5)

    def test_normalise_time(self):
        red = PowderILLParameterScan(Run=self._runs, NormaliseTo="Time")
        self.assertTrue(red)
        self.assertAlmostEqual(red.readY(0)[1400], 9532 / 300.0, 4)
        self.assertAlmostEqual(red.readY(1)[2100], 9789 / 300.0, 2)

    def test_normalise_roi(self):
        red = PowderILLParameterScan(Run=self._runs, NormaliseTo="ROI", ROI="0,100")
        self.assertTrue(red)
        self.assertAlmostEqual(red.readY(0)[1400], 0.00055, 5)
        self.assertAlmostEqual(red.readY(1)[2100], 0.00053, 5)

    def test_crop_zero_counting_cells(self):
        red = PowderILLParameterScan(Run=self._runs, ZeroCountingCells="Crop")
        self.assertTrue(red)
        self.assertEqual(red.blocksize(), 3002)

    def test_rebin(self):
        red = PowderILLParameterScan(Run=self._runs, ScanAxisBinWidth=12, SortObservableAxis=True)
        self.assertEqual(red.getNumberHistograms(), 1)
        self.assertAlmostEqual(red.getAxis(1).extractValues()[0], 248.372, 5)

    def test_normalise_time_single(self):
        red = PowderILLParameterScan(Run="967087", NormaliseTo="Time")
        self.assertTrue(red)
        self.assertAlmostEqual(red.readY(0)[1400], 9532 / 300.0, 4)


if __name__ == "__main__":
    unittest.main()
