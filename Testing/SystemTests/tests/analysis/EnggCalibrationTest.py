#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

def rel_err_less_delta(val, ref, epsilon):
    """
    Checks that a value 'val' does not differ from a reference value 'ref' by 'epsilon'
    or more. This method compares the relative error. An epsilon of 0.1 means a relative
    difference of 10 % = 100*0.1 %

    @param val :: value obtained from a calculation or algorithm
    @param ref :: (expected) reference value
    @param epsilon :: acceptable relative error (error tolerance)

    @returns if val differs in relative terms from ref by less than epsilon
    """
    if 0 == ref:
        return False
    check = (abs((ref-val)/ref) < epsilon)
    if not check:
        print ("Value '{0}' differs from reference '{1}' by more than required epsilon '{2}' (relative)"
               .format(val, ref, epsilon))

    return check

class EnginXFocusWithVanadiumCorrection(stresstesting.MantidStressTest):

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)

        # This test makes sure that the pre-calculated values (which are extensively used in the
        # unit tests) are still the same results as we get from the actual calculations
        self._precalc_van_ws = LoadNexus(Filename='ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs',
                                         OutputWorkspace='ENGIN-X_vanadium_curves_test_ws')
        self._precalc_van_integ_tbl = LoadNexus(Filename=
                                                'ENGINX_precalculated_vanadium_run000236516_integration.nxs',
                                                OutputWorkspace='ENGIN-X_vanadium_integ_test_ws')

        self.van_bank_curves_name = 'enginx_van_bank_curves'
        self.van_integ_name = 'enginx_van_bank_curves_with_precalc_integ'
        self.out_ws_name = 'enginx_focussed'
        self.out_ws_precalc_name = 'enginx_focussed_with_precalculations'

    def runTest(self):
        # These lines run a 'Focus' for the instrument EnginX, as an isolated step here
        # The next test about 'CalibrateFull'-'Calibrate' also includes focusing, as
        # 'Calibrate' uses 'Focus' as a child algorithm
        long_calib_ws = Load(Filename = 'ENGINX00193749.nxs')

        van_ws = Load(Filename = 'ENGINX00236516.nxs')

        # note: not giving calibrated detector positions
        # Using just 12 break points which is enough for this test. The normal/default value would be 50
        EnggVanadiumCorrections(VanadiumWorkspace = van_ws,
                                SplineBreakPoints = 12,
                                OutIntegrationWorkspace = self.van_integ_name,
                                OutCurvesWorkspace = self.van_bank_curves_name)

        # do all calculations from raw data
        EnggFocus(InputWorkspace = long_calib_ws,
                  VanadiumWorkspace = van_ws,
                  Bank = '1',
                  OutputWorkspace=self.out_ws_name)

        # Now with pre-calculated curves and integration values. This makes sure that these do not
        # change too much AND the final results do not change too much as a result
        EnggFocus(InputWorkspace = long_calib_ws,
                  VanIntegrationWorkspace = self._precalc_van_integ_tbl,
                  VanCurvesWorkspace = self._precalc_van_ws,
                  Bank = '1',
                  OutputWorkspace = self.out_ws_precalc_name)

    def validate(self):
        out_ws = mtd[self.out_ws_name]
        self.assertEquals(out_ws.getName(), self.out_ws_name)
        self.assertEqual(out_ws.getNumberHistograms(), 1)
        self.assertEqual(out_ws.blocksize(), 10186)
        self.assertEqual(out_ws.getNEvents(), 10186)
        self.assertEqual(out_ws.getNumDims(), 2)
        self.assertEqual(out_ws.YUnit(), 'Counts')
        dimX = out_ws.getXDimension()
        self.assertEqual(dimX.getName(), 'Time-of-flight')
        self.assertEqual(dimX.getUnits(), 'microsecond')
        dimY = out_ws.getYDimension()
        self.assertEqual(dimY.getName(), 'Spectrum')
        self.assertEqual(dimY.getUnits(), '')

        van_out_ws = mtd[self.van_bank_curves_name]
        self.assertEquals(van_out_ws.getName(), self.van_bank_curves_name)
        self.assertTrue(van_out_ws.getNumberHistograms(), 3)
        self.assertEqual(out_ws.blocksize(), 10186)

        dimX = van_out_ws.getXDimension()
        self.assertEqual(dimX.getName(), 'd-Spacing')
        self.assertEqual(dimX.getUnits(), 'Angstrom')
        dimY = van_out_ws.getYDimension()
        self.assertEqual(dimY.getName(), '')
        self.assertEqual(dimY.getUnits(), '')

        # === check the simulated curve from fitted function against previously saved curve ===
        simul = van_out_ws.readY(1)

        # with precalc curve but not precalc integration
        precalc_curve_simul = self._precalc_van_ws.readY(1)
        self.assertEquals(len(simul), len(precalc_curve_simul))

        delta = 1e-5
        for i in range(0, len(simul)):
            self.assertTrue(rel_err_less_delta(simul[i], precalc_curve_simul[i], delta),
                            "Relative difference bigger than acceptable error (%f) when comparing bin %d "
                            "against bank curves previously fitted. got: %f where I expect: %f"%
                            (delta, i, simul[i], precalc_curve_simul[i]))

        # === check the integration table against the previously saved integration table ===
        integ_tbl = mtd[self.van_integ_name]
        precalc_integ_tbl = self._precalc_van_integ_tbl
        self.assertEquals(integ_tbl.rowCount(), 2513)
        self.assertEquals(integ_tbl.rowCount(), precalc_integ_tbl.rowCount())
        self.assertEquals(integ_tbl.columnCount(), precalc_integ_tbl.columnCount())
        for i in range(integ_tbl.rowCount()):
            self.assertTrue((0 == integ_tbl.cell(i, 0) and 0 == precalc_integ_tbl.cell(i, 0)) or
                            rel_err_less_delta(integ_tbl.cell(i, 0), precalc_integ_tbl.cell(i, 0), delta),
                            "Relative difference bigger than gaccepted error (%f) when comparing the "
                            "integration of a spectrum (%f) against the integration previously calculated and "
                            "saved (%f)." % (delta, integ_tbl.cell(i, 0), precalc_integ_tbl.cell(i, 0) ))

        # === check the 'focussed' spectrum ===
        out_precalc_ws = mtd[self.out_ws_precalc_name]
        self.assertEquals(out_precalc_ws.getNumberHistograms(), 1)
        self.assertEquals(out_precalc_ws.blocksize(), out_ws.blocksize())
        focussed_sp = out_ws.readY(0)
        focussed_sp_precalc = out_precalc_ws.readY(0)
        for i in range(0, out_ws.blocksize()):
            self.assertTrue(rel_err_less_delta(simul[i], precalc_curve_simul[i], delta),
                            "Relative difference bigger than accepted delta (%f) when comparing bin %d "
                            "of the focussed spectrum against the focussed spectrum obtained using bank curves "
                            "and spectra integration values pre-calculated. Got: %f where I expected: %f"%
                            (delta, i, focussed_sp[i], focussed_sp_precalc[i]))

    def cleanup(self):
        mtd.remove(self.out_ws_name)
        mtd.remove(self.out_ws_precalc_name)
        mtd.remove(self.van_integ_name)
        mtd.remove(self.van_bank_curves_name)


class EnginXCalibrateFullThenCalibrateTest(stresstesting.MantidStressTest):

    #pylint: disable=too-many-instance-attributes
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        # difc and zero parameters for GSAS
        self.difa = -1
        self.difa_b2 = -1
        self.difc = -1
        self.difc_b2 = -1
        self.zero_b2 = -1
        self.zero = -1
        # table workspace with detector positions
        self.pos_table = None
        # table workspace with parameters of fitted peaks
        self.peaks_info = None
        # expected peaks in d-spacing and fitted peak centers
        self.peaks = []
        self.peaks_b2 = []
        self.peaks_fitted = []
        self.peaks_fitted_b2 = []

    def runTest(self):
        # These lines run a 'CalibrateFull' and then 'Calibrate' for the instrument EnginX

        # This must be the long Ceria (CeO2) run for calibrate-full
        long_calib_ws = Load(Filename = 'ENGINX00193749.nxs')

        # This must be the (big) Vanadium (V-Nb) run for vanadium corrections
        van_ws = Load(Filename = 'ENGINX00194547.nxs')
        # Another vanadium run file available is 'ENGINX00236516.nxs'
        # (more recent but not matching the full calibration ceria run)

        # This needs a relatively long list of peaks if you want it to work
        # for every detector in all (both) banks
        positions, peaks_params = EnggCalibrateFull(Workspace = long_calib_ws,
                                                    VanadiumWorkspace = van_ws,
                                                    Bank = '1',
                                                    ExpectedPeaks = '0.855618487, 0.956610, 1.104599, '
                                                    '1.352852, 1.562138, 1.631600, '
                                                    '1.913221, 2.705702376, 3.124277511')

        # to protect against 'invalidated' variables with algorithm input/output workspaces
        self.pos_table = positions
        self.peaks_info = peaks_params

        # Bank 1
        self.difa, self.difc, self.zero, tbl = EnggCalibrate(InputWorkspace = long_calib_ws,
                                                             VanadiumWorkspace = van_ws,
                                                             Bank = '1',
                                                             ExpectedPeaks =
                                                             '2.7057,1.9132,1.6316,1.5621,1.3528,0.9566',
                                                             DetectorPositions = self.pos_table)
        self.peaks = tbl.column('dSpacing')
        self.peaks_fitted = tbl.column('X0')

        # Bank 2
        self.difa_b2, self.difc_b2, self.zero_b2, tbl_b2 = EnggCalibrate(InputWorkspace = long_calib_ws,
                                                                         VanadiumWorkspace = van_ws,
                                                                         Bank = '2',
                                                                         ExpectedPeaks =
                                                                         '2.7057,1.9132,1.6316,1.5621,1.3528,0.9566',
                                                                         DetectorPositions = self.pos_table)
        self.peaks_b2 = tbl_b2.column('dSpacing')
        self.peaks_fitted_b2 = tbl_b2.column('X0')

    def validate(self):
        # === check detector positions table produced by EnggCalibrateFull
        self.assertTrue(self.pos_table)
        self.assertEquals(self.pos_table.columnCount(), 10)
        self.assertEquals(self.pos_table.rowCount(), 1200)
        self.assertEquals(self.pos_table.cell(88, 0), 100089)   # det ID
        self.assertEquals(self.pos_table.cell(200, 0), 101081)  # det ID
        self.assertEquals(self.pos_table.cell(88, 0), 100089)   # det ID

        # The output table of peak parameters has the expected structure
        self.assertEquals(self.peaks_info.rowCount(), 1200)
        self.assertEquals(self.peaks_info.columnCount(), 2)
        self.assertEquals(self.peaks_info.keys(), ['Detector ID', 'Parameters'])
        self.assertEquals(self.peaks_info.cell(10, 0), 100011)
        self.assertEquals(self.peaks_info.cell(100, 0),100101)
        self.assertEquals(self.peaks_info.cell(123, 0), 101004)
        self.assertEquals(self.peaks_info.cell(517, 0), 104038)
        self.assertEquals(self.peaks_info.cell(987, 0), 108028)
        self.assertEquals(self.peaks_info.cell(1000, 0), 108041)
        for idx in [0, 12, 516, 789, 891, 1112]:
            cell_val = self.peaks_info.cell(idx,1)
            self.assertTrue(isinstance(cell_val, str))
            self.assertEquals(cell_val[0:11], '{"1": {"A":')
            self.assertEquals(cell_val[-2:], '}}')

        self.assertEquals(self.difa, 0)
        self.assertEquals(self.difa_b2, 0)

        # this will be used as a comparison delta in relative terms (percentage)
        exdelta = exdelta_special = exdelta_tzero = 1e-5
        # Mac fitting tests produce differences for some reason.
        import sys
        if "darwin" == sys.platform:
            exdelta = 1e-2
            # Some tests need a bigger delta
            exdelta_tzero = exdelta_special = 1e-1
        if "win32" == sys.platform:
            exdelta = 5e-4 # this is needed especially for the zero parameter (error >=1e-4)
            exdelta_special = exdelta
            # tzero is particularly sensitive on windows, but 2% looks acceptable considering we're
            # not using all the peaks (for speed), and that the important parameter, DIFC, is ok.
            exdelta_tzero = 2e-2

        # Note that the reference values are given with 12 digits more for reference than
        # for assert-comparison purposes (comparisons are not that picky, by far)

        self.assertTrue(rel_err_less_delta(self.pos_table.cell(100, 3), 1.53041625023, exdelta))
        #self.assertDelta(self.pos_table.cell(100, 3), 1.49010562897, delta)
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(400, 4), 1.65264105797, exdelta))
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(200, 5), -0.296705961227, exdelta))
        self.assertEquals(self.pos_table.cell(133, 7), 0)   # DIFA
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(610, 8), 18603.7597656, exdelta))
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(1199, 9), -5.62454175949, exdelta_special))

        # === check difc, zero parameters for GSAS produced by EnggCalibrate

        # Bank 1
        self.assertTrue(rel_err_less_delta(self.difc, 18403.4516907, exdelta_special),
                        "difc parameter for bank 1 is not what was expected, got: %f" % self.difc)
        if "darwin" != sys.platform:
            self.assertTrue(rel_err_less_delta(self.zero, 5.23928765686, exdelta_tzero),
                            "zero parameter for bank 1 is not what was expected, got: %f" % self.zero)

        # Bank 2
        self.assertTrue(rel_err_less_delta(self.difc_b2, 18388.8780161, exdelta_special),
                        "difc parameter for bank 2 is not what was expected, got: %f" % self.difc_b2)
        if "darwin" != sys.platform:
            self.assertTrue(rel_err_less_delta(self.zero_b2, -4.35573786169, exdelta_tzero),
                            "zero parameter for bank 2 is not what was expected, got: %f" % self.zero_b2)

        # === peaks used to fit the difc and zero parameters ===
        expected_peaks = [0.9566, 1.3528, 1.5621, 1.6316, 1.9132, 2.7057]
        # 0.9566 is not found for bank2 (after all, CalibrateFull is only applied on bank 1.
        expected_peaks_b2 = expected_peaks[1:]
        self.assertEquals(len(self.peaks), len(expected_peaks))
        self.assertEquals(len(self.peaks_b2), len(expected_peaks_b2))
        self.assertEquals(self.peaks, expected_peaks)
        self.assertEquals(self.peaks_b2, expected_peaks_b2)
        self.assertEquals(len(self.peaks_fitted), len(expected_peaks))
        self.assertEquals(len(self.peaks_fitted_b2), len(expected_peaks_b2))

        # Check that the individual peaks do not deviate too much from the fitted
        # straight line
        for fit1, expected in zip(self.peaks_fitted, self.peaks):
            REF_COEFF_B1 = 18405
            self.assertTrue(rel_err_less_delta(fit1 / expected, REF_COEFF_B1, 5e-2))

        for fit2, expected_b2 in zip(self.peaks_fitted_b2, self.peaks_b2):
            REF_COEFF_B2 = 18385
            self.assertTrue(rel_err_less_delta(fit2 / expected_b2, REF_COEFF_B2, 5e-2))

    def cleanup(self):
        mtd.remove('long_calib_ws')
        mtd.remove('van_ws')
        mtd.remove('positions')
        mtd.remove('calib_ws')
