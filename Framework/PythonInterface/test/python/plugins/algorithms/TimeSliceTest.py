import unittest
from mantid.simpleapi import *
from mantid.api import *

class TimeSliceTest(unittest.TestCase):

    def test_basic(self):
        """
        Test to ensure that algorithm completes succesfully.
        """

        TimeSlice(InputFiles=['IRS26173.raw'],
                  SpectraRange=[3, 53],
                  PeakRange=[62500, 65000],
                  BackgroundRange=[59000, 61500],
                  OutputWorkspace='SliceTestOut')

        self.assertTrue(mtd.doesExist('SliceTestOut'))
        self.assertTrue(mtd.doesExist('irs26173_slice'))


    def test_suffix(self):
        """
        Tests to ensure that output names have a suffic appended correctly.
        """

        TimeSlice(InputFiles=['IRS26173.raw'],
                  SpectraRange=[3, 53],
                  PeakRange=[62500, 65000],
                  BackgroundRange=[59000, 61500],
                  OutputNameSuffix='_graphite002_slice',
                  OutputWorkspace='SliceTestOut')

        self.assertTrue(mtd.doesExist('irs26173_graphite002_slice'))


    def test_validation_peak_range_order(self):
        """
        Tests validation of the PeakRange property.
        """

        self.assertRaises(RuntimeError,
                          TimeSlice,
                          InputFiles=['IRS26173.raw'],
                          SpectraRange=[3, 53],
                          PeakRange=[65000, 62500],
                          BackgroundRange=[59000, 61500],
                          OutputNameSuffix='_graphite002_slice',
                          OutputWorkspace='SliceTestOut')


    def test_validation_peak_range_count(self):
        """
        Tests validation of the PeakRange property.
        """

        self.assertRaises(RuntimeError,
                          TimeSlice,
                          InputFiles=['IRS26173.raw'],
                          SpectraRange=[3, 53],
                          PeakRange=[65000],
                          BackgroundRange=[59000, 61500],
                          OutputNameSuffix='_graphite002_slice',
                          OutputWorkspace='SliceTestOut')


    def test_validation_background_range_order(self):
        """
        Tests validation of the BackgroundRange property.
        """

        self.assertRaises(RuntimeError,
                          TimeSlice,
                          InputFiles=['IRS26173.raw'],
                          SpectraRange=[3, 53],
                          PeakRange=[65000, 62500],
                          BackgroundRange=[61500, 59000],
                          OutputNameSuffix='_graphite002_slice',
                          OutputWorkspace='SliceTestOut')


    def test_validation_peak_range_count(self):
        """
        Tests validation of the BackgroundRange property.
        """

        self.assertRaises(RuntimeError,
                          TimeSlice,
                          InputFiles=['IRS26173.raw'],
                          SpectraRange=[3, 53],
                          PeakRange=[65000, 62500],
                          BackgroundRange=[59000],
                          OutputNameSuffix='_graphite002_slice',
                          OutputWorkspace='SliceTestOut')


if __name__ == '__main__':
    unittest.main()
