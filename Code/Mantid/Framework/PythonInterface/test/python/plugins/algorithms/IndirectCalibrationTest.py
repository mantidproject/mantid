import unittest
import mantid
from mantid.simpleapi import IndirectCalibration


class IndirectCalibrationTest(unittest.TestCase):

    def test_simple(self):
        cal_ws = IndirectCalibration(InputFiles='IRS38633.raw',
                                            DetectorRange=[3,53],
                                            PeakRange=[62000,65000],
                                            BackgroundRange=[59000,61000])

        self.assertEqual(cal_ws.getNumberHistograms(), 51)
        self.assertEqual(cal_ws.blocksize(), 1)


if __name__ == "__main__":
	unittest.main()
