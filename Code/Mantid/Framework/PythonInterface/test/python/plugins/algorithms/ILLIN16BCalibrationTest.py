import unittest
import mantid
from mantid.simpleapi import ILLIN16BCalibration


class ILLIN16BCalibrationTest(unittest.TestCase):

    def test_happy_case_normal(self):
        calib_ws = ILLIN16BCalibration(Run='ILLIN16B_034745.nxs',
                                       MirrorMode=False,
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 24)
        self.assertEqual(calib_ws.blocksize(), 1)


    def test_happy_case_mirror_mode(self):
        calib_ws = ILLIN16BCalibration(Run='ILLIN16B_034745.nxs',
                                       MirrorMode=True,
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 24)
        self.assertEqual(calib_ws.blocksize(), 1)


    def test_spectra_range(self):
        calib_ws = ILLIN16BCalibration(Run='ILLIN16B_034745.nxs',
                                       MirrorMode=True,
                                       SpectraRange=[4, 10],
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 7)
        self.assertEqual(calib_ws.blocksize(), 1)

        self.assertEqual(calib_ws.getSpectrum(0).getSpectrumNo(), 4)
        self.assertEqual(calib_ws.getSpectrum(6).getSpectrumNo(), 10)


if __name__=="__main__":
    unittest.main()
