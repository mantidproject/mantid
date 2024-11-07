# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import IndirectResolution


class IndirectResolutionTest(unittest.TestCase):
    def test_simple(self):
        res_ws = IndirectResolution(
            InputFiles="IRS26173.raw",
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            DetectorRange=[3, 53],
            BackgroundRange=[-0.16, -0.14],
            RebinParam="-0.175, 0.002, 0.175",
        )

        self.assertEqual(res_ws.getNumberHistograms(), 1)
        self.assertEqual(res_ws.blocksize(), 175)
        run = res_ws.run()
        self.assertEqual(run.getProperty("res_back_end").value, -0.14)
        self.assertEqual(run.getProperty("rebin_high").value, 0.175)
        self.assertEqual(run.getProperty("rebin_width").value, 0.002)

    def test_scale(self):
        res_ws = IndirectResolution(
            InputFiles="IRS26173.raw",
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            DetectorRange=[3, 53],
            BackgroundRange=[-0.16, -0.14],
            RebinParam="-0.175,0.002,0.175",
            ScaleFactor=10,
        )

        self.assertEqual(res_ws.getNumberHistograms(), 1)
        self.assertEqual(res_ws.blocksize(), 175)
        self.assertEqual(res_ws.run().getProperty("res_scale_factor").value, 10)

    def test_logs(self):
        res_ws = IndirectResolution(
            InputFiles="IRS26173.raw",
            Instrument="IRIS",
            Analyser="graphite",
            Reflection="002",
            DetectorRange=[3, 53],
            BackgroundRange=[-0.16, -0.14],
            RebinParam="-0.175,0.002,0.175",
            LoadLogFiles=True,
        )

        self.assertEqual(res_ws.getNumberHistograms(), 1)
        self.assertEqual(res_ws.blocksize(), 175)
        # testing current period because it's in the log files
        self.assertTrue(res_ws.run().getProperty("current_period").value, 1)


if __name__ == "__main__":
    unittest.main()
