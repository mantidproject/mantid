import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnginXCalibrateFullTest(unittest.TestCase):

    def test_issues_with_properties(self):
        """
        Handle in/out property errors appropriately.
        """

        # No Filename property (required)
        self.assertRaises(RuntimeError,
                          EnginXCalibrateFull,
                          File='foo', Bank=1)

        # Wrong filename
        self.assertRaises(RuntimeError,
                          EnginXCalibrateFull,
                          File='bar_file_is_not_there.not', Bank=2)

        # mispelled ExpectedPeaks
        self.assertRaises(RuntimeError,
                          EnginXCalibrateFull,
                          Filename='ENGINX00228061.nxs', Bank=2, Peaks='2')

        # all fine, except missing DetectorPositions (output)
        self.assertRaises(RuntimeError,
                          EnginXCalibrateFull,
                          Filename='ENGINX00228061.nxs', Bank=2)

    def test_wrong_fit_fails_gracefully(self):
        """
        Checks a bad run fails reasonably.
        """

        # This should produce fitting 'given peak center ... is outside of data range'
        # warnings and finally raise after a 'some peaks not found' error
        self.assertRaises(RuntimeError,
                          EnginXCalibrateFull,
                          Filename="ENGINX00228061.nxs", ExpectedPeaks=[0.01], Bank=1,
                          DetectorPositions='out_det_positions_table')


    def test_run_ok_but_bad_data(self):
        """
        Tests a run that doesn't go well becaus of inappropriate data is used here.
        """

        # This is not a realistic CalibrateFull run, but it just runs fine
        # for testing purposes. A test with real (much larger) data that produces
        # a correct fit is included in system tests
        tbl_name = 'det_peaks_tbl'
        det_peaks_tbl = CreateEmptyTableWorkspace()
        self.assertRaises(RuntimeError,
                          EnginXCalibrateFull,
                          Filename="ENGINX00228061.nxs", Bank=2,
                          ExpectedPeaks='0.915, 1.257, 1.688',
                          DetectorPositions=tbl_name)



if __name__ == '__main__':
    unittest.main()
