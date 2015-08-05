import unittest
from mantid.api import *
import mantid.simpleapi as sapi

class EnggCalibrateTest(unittest.TestCase):

    _data_ws = None
    _van_curves_ws = None
    _van_integ_tbl = None

    # Note not using @classmethod setUpClass / tearDownClass because that's not supported in the old
    # unittest of rhel6
    def setUp(self):
        """
        Set up dependencies for one or more of the tests below.
        """
        if not self.__class__._data_ws:
            self.__class__._data_ws = sapi.LoadNexus("ENGINX00228061.nxs", OutputWorkspace='ENGIN-X_test_ws')

        if not self.__class__._van_curves_ws:
            # Note the pre-calculated file instead of the too big vanadium run
            # self.__class__._van_ws = LoadNexus("ENGINX00236516.nxs", OutputWorkspace='ENGIN-X_test_vanadium_ws')
            self.__class__._van_curves_ws = sapi.LoadNexus(Filename=
                                                           'ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs',
                                                           OutputWorkspace='ENGIN-X_vanadium_curves_test_ws')
            self.__class__._van_integ_tbl = sapi.LoadNexus(Filename=
                                                           'ENGINX_precalculated_vanadium_run000236516_integration.nxs',
                                                           OutputWorkspace='ENGIN-X_vanadium_integ_test_ws')


    def test_issues_with_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing required
        ones.
        """

        # No InputWorkspace property (required)
        self.assertRaises(RuntimeError,
                          sapi.EnggCalibrate,
                          File='foo', Bank='1')

        # Wrong (mispelled) InputWorkspace property
        self.assertRaises(RuntimeError,
                          sapi.EnggCalibrate,
                          InputWorkpace='anything_goes', Bank='2')

        # mispelled ExpectedPeaks
        tbl = sapi.CreateEmptyTableWorkspace(OutputWorkspace='test_table')
        self.assertRaises(RuntimeError,
                          sapi.EnggCalibrate,
                          Inputworkspace=self.__class__._data_ws, DetectorPositions=tbl, Bank='2', Peaks='2')

        # mispelled DetectorPositions
        self.assertRaises(RuntimeError,
                          sapi.EnggCalibrate,
                          InputWorkspace=self.__class__._data_ws, Detectors=tbl, Bank='2', Peaks='2')

        # There's no output workspace
        self.assertRaises(RuntimeError,
                          sapi.EnggCalibrate,
                          InputWorkspace=self.__class__._data_ws, Bank='1')


    def test_fails_gracefully(self):
        """
        Checks a bad run.
        """

        # This should produce 'given peak center ... is outside of data range' warnings
        # and finally raise after a 'some peaks not found' error
        self.assertRaises(RuntimeError,
                          sapi.EnggCalibrate,
                          InputWorkspace=self.__class__._data_ws, ExpectedPeaks=[0.2, 0.4], Bank='2')

    def test_runs_ok(self):
        """
        Checks normal operation.
        """

        difc, zero = sapi.EnggCalibrate(InputWorkspace=self.__class__._data_ws,
                                        VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                                        VanCurvesWorkspace=self.__class__._van_curves_ws,
                                        ExpectedPeaks=[1.6, 1.1, 1.8], Bank='2')

        self.check_3peaks_values(difc, zero)


    def test_runs_ok_with_peaks_file(self):
        """
        Normal operation with a csv input file with expected peaks
        """
        # This file has: 1.6, 1.1, 1.8 (as the test above)
        filename = 'EnginX_3_expected_peaks_unittest.csv'
        difc, zero = sapi.EnggCalibrate(InputWorkspace=self.__class__._data_ws,
                                        VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                                        VanCurvesWorkspace=self.__class__._van_curves_ws,
                                        ExpectedPeaks=[-4, 40, 323], # nonsense, but FromFile should prevail
                                        ExpectedPeaksFromFile=filename,
                                        Bank='2')

        self.check_3peaks_values(difc, zero)

    def check_3peaks_values(self, difc, zero):
        # There are platform specific differences in final parameter values
        # For example in earlier versions, debian: 369367.57492582797; win7: 369242.28850305633
        expected_difc = 19110.7598121
        # win7 results were ~0.831% different (19269.451153) from linux expected values,
        # osx were ~0.995% different (18920.539474)
        difc_err_epsilon = 1e-2

        # assertLess would be nice, but only available in unittest >= 2.7
        self.assertTrue(abs((expected_difc-difc)/expected_difc) < difc_err_epsilon,
                        "Difc (%f) is too far from its expected value (%f)" %(difc, expected_difc))

        expected_zero = -724.337353801
        # especially this zero parameter is extremely platform dependent/sensitive
        # ubuntu: -724.337354; osx: -396.628396; win7: -995.879786

        # this is obviously a ridiculous threshold to do just a very rough test that results/funcionality
        # do not change too much
        zero_err_epsilon = 0.5

        self.assertTrue(abs((expected_zero-zero)/expected_zero) < zero_err_epsilon,
                        "Zero (%f) is too far from its expected value (%f)" %(zero, expected_zero))


if __name__ == '__main__':
    unittest.main()
