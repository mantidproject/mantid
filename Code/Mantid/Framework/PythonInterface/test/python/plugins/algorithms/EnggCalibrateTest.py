import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnggCalibrateTest(unittest.TestCase):

    _data_ws = None

    # Note not using @classmethod setUpClass / tearDownClass because that's not supported in the old
    # unittest of rhel6
    def setUp(self):
        """
        Set up dependencies for one or more of the tests below.
        """
        if not self.__class__._data_ws:
            self.__class__._data_ws = LoadNexus("ENGINX00228061.nxs", OutputWorkspace='ENGIN-X_test_ws')

    def test_issues_with_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing required
        ones.
        """

        # No InputWorkspace property (required)
        self.assertRaises(RuntimeError,
                          EnggCalibrate,
                          File='foo', Bank='1')

        # Wrong (mispelled) InputWorkspace property
        self.assertRaises(RuntimeError,
                          EnggCalibrate,
                          InputWorkpace='anything_goes', Bank='2')

        # mispelled ExpectedPeaks
        tbl = CreateEmptyTableWorkspace(OutputWorkspace='test_table')
        self.assertRaises(RuntimeError,
                          EnggCalibrate,
                          Inputworkspace=self.__class__._data_ws, DetectorPositions=tbl, Bank='2', Peaks='2')

        # mispelled DetectorPositions
        self.assertRaises(RuntimeError,
                          EnggCalibrate,
                          InputWorkspace=self.__class__._data_ws, Detectors=tbl, Bank='2', Peaks='2')

        # There's no output workspace
        self.assertRaises(RuntimeError,
                          EnggCalibrate,
                          InputWorkspace=self.__class__._data_ws, Bank='1')


    def test_fails_gracefully(self):
        """
        Checks a bad run.
        """

        # This should produce 'given peak center ... is outside of data range' warnings
        # and finally raise after a 'some peaks not found' error
        self.assertRaises(RuntimeError,
                          EnggCalibrate,
                          InputWorkspace=self.__class__._data_ws, ExpectedPeaks=[0.2, 0.4], Bank='2')

    def test_runs_ok(self):
        """
        Checks normal operation.
        """

        difc, zero = EnggCalibrate(InputWorkspace=self.__class__._data_ws,
                                     ExpectedPeaks=[1.6, 1.1, 1.8], Bank='2')

        self.check_3peaks_values(difc, zero)


    def test_runs_ok_with_peaks_file(self):
        """
        Normal operation with a csv input file with expected peaks
        """
        # This file has: 1.6, 1.1, 1.8 (as the test above)
        filename = 'EnginX_3_expected_peaks_unittest.csv'
        difc, zero = EnggCalibrate(InputWorkspace=self.__class__._data_ws,
                                     ExpectedPeaks=[-4, 40, 323], # nonsense, but FromFile should prevail
                                     ExpectedPeaksFromFile=filename,
                                     Bank='2')

        self.check_3peaks_values(difc, zero)

    def check_3peaks_values(self, difc, zero):
        # There are platform specific differences in final parameter values
        # For example, debian: 369367.57492582797; win7: 369242.28850305633
        expected_difc = 369367.57492582797
        # assertLess would be nices, but only available in unittest >= 2.7
        self.assertTrue(abs((expected_difc-difc)/expected_difc) < 5e-3)
        expected_zero = -223297.87349744083
        self.assertTrue(abs((expected_zero-zero)/expected_zero) < 5e-3)


if __name__ == '__main__':
    unittest.main()
