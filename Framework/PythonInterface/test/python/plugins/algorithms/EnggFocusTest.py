# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from mantid.api import *


class EnggFocusTest(unittest.TestCase):

    _data_ws = None
    _van_curves_ws = None
    _van_integ_tbl = None

    _expected_yvals_bank1  = [0.01671889741296237, 0.016036458905862805, 0.04757112365349549,
                              0.1566982298749516, 0.11047168569414788, 0.017336154388340106]

    _expected_yvals_bank2 = [0.0, 0.01835793983891016, 0.07157114331736783,
                             0.06194238438581619, 0.13136628929521785, 0.0017714987522226692]

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

        # bank and indices list clash. This starts as a ValueError but the managers is raising a RuntimeError
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, Bank='2', DetectorPositions=tbl,
                          SpectrumNumbers='1-10', OutputWorkspace='nop')

        # workspace index too big. This starts as a ValueError but the managers is raising a RuntimeError
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, DetectorPositions=tbl,
                          SpectrumNumbers='999999999999999', OutputWorkspace='nop')

        # wrong normalize option
        self.assertRaises(ValueError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, DetectorPositions=tbl,
                          SpectrumNumbers='3', NormaliseByCurrent='4',
                          OutputWorkspace='nop')

        # wrong bin values for masking
        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, DetectorPositions=tbl,
                          SpectrumNumbers='3',
                          MaskBinsXMins=[1, 2, 3], MaskBinsXMaxs=[10, 20],
                          OutputWorkspace='nop')

        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, DetectorPositions=tbl,
                          SpectrumNumbers='3', MaskBinsXMins=[1, 2, 3],
                          OutputWorkspace='nop')

        self.assertRaises(RuntimeError,
                          EnggFocus,
                          InputWorkspace=self.__class__._data_ws, DetectorPositions=tbl,
                          SpectrumNumbers='3', MaskBinsXMaxs=[10, 20, 30],
                          OutputWorkspace='nop')

    def _check_output_ok(self, wks, ws_name='', y_dim_max=1, yvalues=None):
        """
        Checks expected types, values, etc. of an output workspace from EnggFocus.
        """
        self.assertTrue(isinstance(wks, MatrixWorkspace),
                        'Output workspace should be a matrix workspace.')
        self.assertEqual(wks.name(), ws_name)
        self.assertEqual(wks.getTitle(), 'yscan;y=250.210')
        self.assertEqual(wks.isHistogramData(), True)
        self.assertEqual(wks.isDistribution(), True)
        self.assertEqual(wks.getNumberHistograms(), 1)
        self.assertEqual(wks.blocksize(), 98)
        self.assertEqual(wks.getNEvents(), 98)
        self.assertEqual(wks.getNumDims(), 2)
        self.assertEqual(wks.YUnit(), 'Counts')
        dimX = wks.getXDimension()
        self.assertAlmostEqual( dimX.getMaximum(), 36843.375)
        self.assertEqual(dimX.name, 'Time-of-flight')
        self.assertEqual(dimX.getUnits(), 'microsecond')
        dimY = wks.getYDimension()
        self.assertEqual(dimY.getMaximum(), y_dim_max)
        self.assertEqual(dimY.name, 'Spectrum')
        self.assertEqual(dimY.getUnits(), '')

        if yvalues is None:
            raise ValueError("No y-vals provided for test")
        xvals = [10834.11037667225, 12161.113671020974, 13488.116965369696,
                 14815.12025971842, 24104.14332015949, 34720.16967494928]
        p_charge = wks.getRun().getProtonCharge()
        for i, bin_idx in enumerate([0, 5, 10, 15, 50, 90]):
            self.assertAlmostEqual(wks.readX(0)[bin_idx], xvals[i])
            self.assertAlmostEqual(wks.readY(0)[bin_idx], yvalues[i] / p_charge)

    def test_runs_ok(self):
        """
        Checks that output looks fine for normal operation conditions, (default) Bank=1
        """

        out_name = 'out'
        out = EnggFocus(InputWorkspace=self.__class__._data_ws,
                        VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                        VanCurvesWorkspace=self.__class__._van_curves_ws,
                        Bank='1', OutputWorkspace=out_name)

        self._check_output_ok(wks=out, ws_name=out_name, y_dim_max=1, yvalues=self._expected_yvals_bank1)

    def test_runs_ok_north(self):
        """
        Same as before but with Bank='North' - equivalent to Bank='1'
        """

        out_name = 'out'
        out = EnggFocus(InputWorkspace=self.__class__._data_ws,
                        VanIntegrationWorkspace=self.__class__._van_integ_tbl,
                        VanCurvesWorkspace=self.__class__._van_curves_ws,
                        Bank='North', OutputWorkspace=out_name)

        self._check_output_ok(wks=out, ws_name=out_name, y_dim_max=1, yvalues=self._expected_yvals_bank1)

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

        self._check_output_ok(wks=out_idx, ws_name=out_idx_name, y_dim_max=1,
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
        self._check_output_ok(wks=out_idx, ws_name=out_idx_name, y_dim_max=1,
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

        self._check_output_ok(wks=out_bank2, ws_name=out_name, y_dim_max=1201,
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

        self._check_output_ok(wks=out_bank_south, ws_name=out_name, y_dim_max=1201,
                              yvalues=self._expected_yvals_bank2)


if __name__ == '__main__':
    unittest.main()
