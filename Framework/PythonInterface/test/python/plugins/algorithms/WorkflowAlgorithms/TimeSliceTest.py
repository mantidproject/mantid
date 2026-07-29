# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd
from mantid.simpleapi import TimeSlice, Load


class TimeSliceTest(unittest.TestCase):
    def test_basic(self):
        """
        Test to ensure that algorithm completes succesfully.
        """

        TimeSlice(
            InputFiles=["IRS26173.raw"],
            SpectraRange=[3, 53],
            PeakRange=[62500, 65000],
            BackgroundRange=[59000, 61500],
            OutputWorkspace="SliceTestOut",
        )

        self.assertTrue(mtd.doesExist("SliceTestOut"))
        self.assertTrue(mtd.doesExist("iris26173_slice"))

    def test_suffix(self):
        """
        Tests to ensure that output names have a suffic appended correctly.
        """

        TimeSlice(
            InputFiles=["IRS26173.raw"],
            SpectraRange=[3, 53],
            PeakRange=[62500, 65000],
            BackgroundRange=[59000, 61500],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )

        self.assertTrue(mtd.doesExist("iris26173_graphite002_slice"))

    def test_sample_workspace_property(self):
        """
        Test to check optional SampleWorkspace input property
        """
        Load("IRS26173.raw", OutputWorkspace="TestWs")
        TimeSlice(
            SampleWorkspace="TestWs",
            SpectraRange=[3, 53],
            PeakRange=[62500, 65000],
            BackgroundRange=[59000, 61500],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )
        self.assertTrue(mtd.doesExist("TestWs_graphite002_slice"))

    def test_sample_workspace_property_does_not_alter_input_ws(self):
        """
        Test to check optional SampleWorkspace input property
        """
        ws = Load("IRS26173.raw", OutputWorkspace="TestWs")
        no_hist = ws.getNumberHistograms()
        TimeSlice(
            SampleWorkspace="TestWs",
            SpectraRange=[3, 53],
            PeakRange=[62500, 65000],
            BackgroundRange=[59000, 61500],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )
        self.assertEqual(no_hist, mtd["TestWs"].getNumberHistograms())
        self.assertTrue(mtd.doesExist("TestWs_graphite002_slice"))

    def test_validation_input_files_empty(self):
        """
        Tests to ensure that a valid input is selected
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Must supply either 'InputFiles' or 'SampleWorkspace'",
            TimeSlice,
            SpectraRange=[3, 53],
            PeakRange=[62500, 65000],
            BackgroundRange=[59000, 61500],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )

    def test_validation_peak_range_order(self):
        """
        Tests validation of the PeakRange property.
        """

        self.assertRaisesRegex(
            RuntimeError,
            'Range must be in format "low,high"',
            TimeSlice,
            InputFiles=["IRS26173.raw"],
            SpectraRange=[3, 53],
            PeakRange=[65000, 62500],
            BackgroundRange=[59000, 61500],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )

    def test_validation_peak_range_count(self):
        """
        Tests validation of the PeakRange property.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Range must have two values",
            TimeSlice,
            InputFiles=["IRS26173.raw"],
            SpectraRange=[3, 53],
            PeakRange=[65000],
            BackgroundRange=[59000, 61500],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )

    def test_validation_background_range_order(self):
        """
        Tests validation of the BackgroundRange property.
        """

        self.assertRaisesRegex(
            RuntimeError,
            'Range must be in format "low,high"',
            TimeSlice,
            InputFiles=["IRS26173.raw"],
            SpectraRange=[3, 53],
            PeakRange=[65000, 62500],
            BackgroundRange=[61500, 59000],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )

    def test_validation_background_range_count(self):
        """
        Tests validation of the BackgroundRange property.
        """

        self.assertRaisesRegex(
            RuntimeError,
            "Range must have two values",
            TimeSlice,
            InputFiles=["IRS26173.raw"],
            SpectraRange=[3, 53],
            PeakRange=[65000, 62500],
            BackgroundRange=[59000],
            OutputNameSuffix="_graphite002_slice",
            OutputWorkspace="SliceTestOut",
        )


if __name__ == "__main__":
    unittest.main()
