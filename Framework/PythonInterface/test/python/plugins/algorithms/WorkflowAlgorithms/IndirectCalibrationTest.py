# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import IndirectCalibration


class IndirectCalibrationTest(unittest.TestCase):
    def test_logs(self):
        cal_ws = IndirectCalibration(
            InputFiles="IRS38633.raw", DetectorRange=[3, 53], PeakRange=[62000, 65000], BackgroundRange=[59000, 61000], LoadLogFiles=True
        )

        self.assertEqual(cal_ws.getNumberHistograms(), 51)
        self.assertEqual(cal_ws.blocksize(), 1)

        self.assertEqual(cal_ws.run().getProperty("current_period").value, 1)

    def test_scale_by_factor_does_not_normalise(self):
        cal_ws = IndirectCalibration(
            InputFiles="IRS38633.raw", DetectorRange=[3, 53], PeakRange=[62000, 65000], BackgroundRange=[59000, 61000], LoadLogFiles=True
        )
        cal_ws_scaled = IndirectCalibration(
            InputFiles="IRS38633.raw",
            DetectorRange=[3, 53],
            PeakRange=[62000, 65000],
            BackgroundRange=[59000, 61000],
            LoadLogFiles=True,
            ScaleByFactor=True,
        )

        self.assertNotEqual(cal_ws_scaled.dataY(0) / cal_ws.dataY(0), 1.0)

    def test_scale_by_factor_uses_factor(self):
        cal_ws_1_0 = IndirectCalibration(
            InputFiles="IRS38633.raw",
            DetectorRange=[3, 53],
            PeakRange=[62000, 65000],
            BackgroundRange=[59000, 61000],
            LoadLogFiles=True,
            ScaleByFactor=True,
            ScaleFactor=1.0,
        )
        cal_ws_0_9 = IndirectCalibration(
            InputFiles="IRS38633.raw",
            DetectorRange=[3, 53],
            PeakRange=[62000, 65000],
            BackgroundRange=[59000, 61000],
            LoadLogFiles=True,
            ScaleByFactor=True,
            ScaleFactor=0.9,
        )

        self.assertEqual(cal_ws_0_9.dataY(0) / cal_ws_1_0.dataY(0), 0.9)


if __name__ == "__main__":
    unittest.main()
