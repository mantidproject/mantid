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
                  BackgroundRange=[59000, 61500])

        self.assertTrue(mtd.doesExist('irs26173_slice'))

    def test_group(self):
        """
        Tests to ensure that result workspaces are goruped correctly.
        """

        TimeSlice(InputFiles=['IRS26173.raw'],
                  SpectraRange=[3, 53],
                  PeakRange=[62500, 65000],
                  BackgroundRange=[59000, 61500],
                  OutputWorkspaceGroup='SliceTestOut')

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
                  OutputNameSuffix='_graphite002_slice')

        self.assertTrue(mtd.doesExist('irs26173_graphite002_slice'))

if __name__ == '__main__':
    unittest.main()
