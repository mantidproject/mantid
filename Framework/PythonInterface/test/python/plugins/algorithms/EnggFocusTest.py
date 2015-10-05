import unittest
from mantid.simpleapi import *
from mantid.api import *

class EnggFocusTest(unittest.TestCase):

    _data_ws = None
    _van_curves_ws = None
    _van_integ_tbl = None

    _expected_yvals_bank1  = [0.016676032919961604, 0.015995344072536975, 0.047449159145519233,
                              0.15629648148139513, 0.11018845452876322, 0.017291707350351286]

    _expected_yvals_bank2 = [0.0, 0.018310873111703541, 0.071387646720910913,
                             0.061783574337739511, 0.13102948781549345, 0.001766956921862095]

    # Note not using @classmethod setUpClass / tearDownClass because that's not supported in the old
    # unittest of rhel6
    def setUp(self):
        """
        Set up dependencies for one or more of the tests below.
        """
        if not self.__class__._data_ws:
            self.__class__._data_ws = LoadNexus(Filename='ENGINX00228061.nxs',
                                                OutputWorkspace='ENGIN-X_test_ws')

        if not self.__class__._van_curves_ws:
            # Note the pre-calculated file instead of the too big vanadium run
            # self.__class__._van_ws = LoadNexus("ENGINX00236516.nxs", OutputWorkspace='ENGIN-X_test_vanadium_ws')
            self.__class__._van_curves_ws = LoadNexus(Filename=
                                               'ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs',
                                               OutputWorkspace='ENGIN-X_vanadium_curves_test_ws')
        if not self.__class__._van_integ_tbl:
            self.__class__._van_integ_tbl = LoadNexus(Filename=
                                                      'ENGINX_precalculated_vanadium_run000236516_integration.nxs',
                                                      OutputWorkspace='ENGIN-X_vanadium_integ_test_ws')

    def test_wrong_properties(self):
        """
        Tests proper error handling when passing wrong properties or not passing
        required ones.
        """

        # No InputWorkspace property
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          Bank='1', OutputWorkspace='nop')

        # Mispelled InputWorkspace prop
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWrkspace='anything_goes', Bank='1', OutputWorkspace='nop')

        # Wrong InputWorkspace name
        self.assertRaises(ValueError,
                          EnggFocus,
                          InputWorkspace='foo_is_not_there', Bank='1', OutputWorkspace='nop')

        # mispelled bank
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, bnk='2', OutputWorkspace='nop')

        # mispelled DetectorsPosition
        tbl = CreateEmptyTableWorkspace()
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, Detectors=tbl, OutputWorkspace='nop')

        # bank and indices list clsh. This starts as a ValueError but the managers is raising a RuntimeError
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, Bank='2', DetectorPositions=tbl,
                          SpectrumNumbers='1-10', OutputWorkspace='nop')

        # workspace index too big. This starts as a ValueError but the managers is raising a RuntimeError
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, DetectorPositions=tbl,
                          SpectrumNumbers='999999999999999', OutputWorkspace='nop')

    def _check_output_ok(self, ws, ws_name='', y_dim_max=1, yvalues=None):
        """
        Checks expected types, values, etc. of an output workspace from EnggFocus.
        """

        self.assertTrue(isinstance(ws, MatrixWorkspace),
                        'Output workspace should be a matrix workspace.')
        self.assertEqual(ws.name(), ws_name)
        self.assertEqual(ws.getTitle(), 'yscan;y=250.210')
        self.assertEqual(ws.isHistogramData(), True)
        self.assertEqual(ws.isDistribution(), True)
        self.assertEqual(ws.getNumberHistograms(), 1)
        self.assertEqual(ws.blocksize(), 98)
        self.assertEqual(ws.getNEvents(), 98)
        self.assertEqual(ws.getNumDims(), 2)
        self.assertEqual(ws.YUnit(), 'Counts')
        dimX = ws.getXDimension()
        self.assertAlmostEqual( dimX.getMaximum(), 36938.078125)
        self.assertEqual(dimX.getName(), 'Time-of-flight')
        self.assertEqual(dimX.getUnits(), 'microsecond')
        dimY = ws.getYDimension()
        self.assertEqual(dimY.getMaximum(), y_dim_max)
        self.assertEqual(dimY.getName(), 'Spectrum')
        self.assertEqual(dimY.getUnits(), '')

        if None == yvalues:
            raise ValueError("No y-vals provided for test")
        xvals = [10861.958645540433, 12192.372902418168, 13522.787159295902,
                 14853.201416173637, 24166.101214317776, 34809.415269339654]
        for i, bin in enumerate([0, 5, 10, 15, 50, 90]):
            self.assertAlmostEqual( ws.readX(0)[bin], xvals[i])
            self.assertAlmostEqual( ws.readY(0)[bin], yvalues[i])


    def test_runs_ok(self):
        """
        Checks that output looks fine for normal operation conditions, (default) Bank=1
        """

        out_name = 'out'
        out = EnggFocus(InputWorkspace=self.__class__._data_ws,
                        VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                        VanCurvesWorkspace=self.__class__._van_curves_ws,
                        Bank='1', OutputWorkspace=out_name)

        self._check_output_ok(ws=out, ws_name=out_name, y_dim_max=1, yvalues=self._expected_yvals_bank1)

    def test_runs_ok_north(self):
        """
        Same as before but with Bank='North' - equivalent to Bank='1'
        """

        out_name = 'out'
        out = EnggFocus(InputWorkspace=self.__class__._data_ws,
                        VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                        VanCurvesWorkspace=self.__class__._van_curves_ws,
                        Bank='North', OutputWorkspace=out_name)

        self._check_output_ok(ws=out, ws_name=out_name, y_dim_max=1, yvalues=self._expected_yvals_bank1)

    def test_runs_ok_indices(self):
        """
        Same as above but with detector (workspace) indices equivalent to bank 1
        """
        out_idx_name = 'out_idx'
        out_idx = EnggFocus(InputWorkspace=self.__class__._data_ws,
                            VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                            VanCurvesWorkspace=self.__class__._van_curves_ws,
                            SpectrumNumbers='1-1200',
                            OutputWorkspace=out_idx_name)

        self._check_output_ok(ws=out_idx, ws_name=out_idx_name, y_dim_max=1,
                              yvalues=self._expected_yvals_bank1)

    def test_runs_ok_indices_split3(self):
        """
        Same as above but with detector (workspace) indices equivalent to bank 1
        """
        out_idx_name = 'out_idx'
        out_idx = EnggFocus(InputWorkspace=self.__class__._data_ws,
                            VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                            VanCurvesWorkspace=self.__class__._van_curves_ws,
                            SpectrumNumbers='1-100, 101-500, 400-1200',
                            OutputWorkspace=out_idx_name)
        self._check_output_ok(ws=out_idx, ws_name=out_idx_name, y_dim_max=1,
                              yvalues=self._expected_yvals_bank1)

    def test_runs_ok_bank2(self):
        """
        Checks that output looks fine for normal operation conditions, Bank=2
        """

        out_name = 'out_bank2'
        out_bank2 = EnggFocus(InputWorkspace=self.__class__._data_ws, Bank='2',
                              VanCurvesWorkspace=self.__class__._van_curves_ws,
                              VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                              OutputWorkspace=out_name)

        self._check_output_ok(ws=out_bank2, ws_name=out_name, y_dim_max=1201,
                              yvalues=self._expected_yvals_bank2)

    def test_runs_ok_bank_south(self):
        """
        As before but using the Bank='South' alias. Should produce the same results.
        """
        out_name = 'out_bank_south'
        out_bank_south = EnggFocus(InputWorkspace=self.__class__._data_ws,
                                   VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                                   VanCurvesWorkspace=self.__class__._van_curves_ws,
                                   Bank='South', OutputWorkspace=out_name)

        self._check_output_ok(ws=out_bank_south, ws_name=out_name, y_dim_max=1201,
                              yvalues=self._expected_yvals_bank2)



if __name__ == '__main__':
    unittest.main()
