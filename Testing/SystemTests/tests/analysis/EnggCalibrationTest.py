# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.simpleapi import *
from six import PY2


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
    check = (abs((ref - val) / ref) < epsilon)
    if not check:
        print ("Value '{0}' differs from reference '{1}' by more than required epsilon '{2}' (relative)"
               .format(val, ref, epsilon))

    return check


class EnginXFocusWithVanadiumCorrection(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)

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
        long_calib_ws = Load(Filename='ENGINX00193749.nxs')

        van_ws = Load(Filename='ENGINX00236516.nxs')

        # note: not giving calibrated detector positions
        # Using just 12 break points which is enough for this test. The normal/default value would be 50
        EnggVanadiumCorrections(VanadiumWorkspace=van_ws,
                                SplineBreakPoints=12,
                                OutIntegrationWorkspace=self.van_integ_name,
                                OutCurvesWorkspace=self.van_bank_curves_name)

        # do all calculations from raw data
        EnggFocus(InputWorkspace=long_calib_ws,
                  VanadiumWorkspace=van_ws,
                  Bank='1',
                  OutputWorkspace=self.out_ws_name)

        # Now with pre-calculated curves and integration values. This makes sure that these do not
        # change too much AND the final results do not change too much as a result
        EnggFocus(InputWorkspace=long_calib_ws,
                  VanIntegrationWorkspace=self._precalc_van_integ_tbl,
                  VanCurvesWorkspace=self._precalc_van_ws,
                  Bank='1',
                  OutputWorkspace=self.out_ws_precalc_name)

    def validate(self):
        out_ws = mtd[self.out_ws_name]
        self.assertEqual(out_ws.name(), self.out_ws_name)
        self.assertEqual(out_ws.getNumberHistograms(), 1)
        self.assertEqual(out_ws.blocksize(), 10186)
        self.assertEqual(out_ws.getNEvents(), 10186)
        self.assertEqual(out_ws.getNumDims(), 2)
        self.assertEqual(out_ws.YUnit(), 'Counts')
        dimX = out_ws.getXDimension()
        self.assertEqual(dimX.name, 'Time-of-flight')
        self.assertEqual(dimX.getUnits(), 'microsecond')
        dimY = out_ws.getYDimension()
        self.assertEqual(dimY.getName(), 'Spectrum')
        self.assertEqual(dimY.getUnits(), '')

        van_out_ws = mtd[self.van_bank_curves_name]
        self.assertEqual(van_out_ws.name(), self.van_bank_curves_name)
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
        self.assertEqual(len(simul), len(precalc_curve_simul))

        delta = 1e-5
        for i in range(0, len(simul)):
            self.assertTrue(rel_err_less_delta(simul[i], precalc_curve_simul[i], delta),
                            "Relative difference bigger than acceptable error (%f) when comparing bin %d "
                            "against bank curves previously fitted. got: %f where I expect: %f" %
                            (delta, i, simul[i], precalc_curve_simul[i]))

        # === check the integration table against the previously saved integration table ===
        integ_tbl = mtd[self.van_integ_name]
        precalc_integ_tbl = self._precalc_van_integ_tbl
        self.assertEqual(integ_tbl.rowCount(), 2513)
        self.assertEqual(integ_tbl.rowCount(), precalc_integ_tbl.rowCount())
        self.assertEqual(integ_tbl.columnCount(), precalc_integ_tbl.columnCount())
        for i in range(integ_tbl.rowCount()):
            self.assertEqual((0,  integ_tbl.cell(i, 0) and 0 == precalc_integ_tbl.cell(i, 0)) or
                             rel_err_less_delta(integ_tbl.cell(i, 0), precalc_integ_tbl.cell(i, 0), delta),
                             "Relative difference bigger than gaccepted error (%f) when comparing the "
                             "integration of a spectrum (%f) against the integration previously calculated and "
                             "saved (%f)." % (delta, integ_tbl.cell(i, 0), precalc_integ_tbl.cell(i, 0)))

        # === check the 'focussed' spectrum ===
        out_precalc_ws = mtd[self.out_ws_precalc_name]
        self.assertEqual(out_precalc_ws.getNumberHistograms(), 1)
        self.assertEqual(out_precalc_ws.blocksize(), out_ws.blocksize())
        focussed_sp = out_ws.readY(0)
        focussed_sp_precalc = out_precalc_ws.readY(0)
        for i in range(0, out_ws.blocksize()):
            self.assertTrue(rel_err_less_delta(simul[i], precalc_curve_simul[i], delta),
                            "Relative difference bigger than accepted delta (%f) when comparing bin %d "
                            "of the focussed spectrum against the focussed spectrum obtained using bank curves "
                            "and spectra integration values pre-calculated. Got: %f where I expected: %f" %
                            (delta, i, focussed_sp[i], focussed_sp_precalc[i]))

    def cleanup(self):
        mtd.remove(self.out_ws_name)
        mtd.remove(self.out_ws_precalc_name)
        mtd.remove(self.van_integ_name)
        mtd.remove(self.van_bank_curves_name)


class EnginXCalibrateFullThenCalibrateTest(systemtesting.MantidSystemTest):
    # pylint: disable=too-many-instance-attributes
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
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
        long_calib_ws = Load(Filename='ENGINX00193749.nxs')

        # This must be the (big) Vanadium (V-Nb) run for vanadium corrections
        van_ws = Load(Filename='ENGINX00194547.nxs')
        # Another vanadium run file available is 'ENGINX00236516.nxs'
        # (more recent but not matching the full calibration ceria run)

        # This needs a relatively long list of peaks if you want it to work
        # for every detector in all (both) banks
        positions, peaks_params = EnggCalibrateFull(Workspace=long_calib_ws,
                                                    VanadiumWorkspace=van_ws,
                                                    Bank='1',
                                                    ExpectedPeaks='0.855618487, 0.956610, 1.104599, '
                                                                  '1.352852, 1.562138, 1.631600, '
                                                                  '1.913221, 2.705702376, 3.124277511')

        # to protect against 'invalidated' variables with algorithm input/output workspaces
        self.pos_table = positions
        self.peaks_info = peaks_params

        # Bank 1
        self.difa, self.difc, self.zero, tbl = EnggCalibrate(InputWorkspace=long_calib_ws,
                                                             VanadiumWorkspace=van_ws,
                                                             Bank='1',
                                                             ExpectedPeaks=
                                                             '2.7057,1.9132,1.6316,1.5621,1.3528,1.1046',
                                                             DetectorPositions=self.pos_table)
        self.peaks = tbl.column('dSpacing')
        self.peaks_fitted = tbl.column('X0')

        # Bank 2
        self.difa_b2, self.difc_b2, self.zero_b2, tbl_b2 = EnggCalibrate(InputWorkspace=long_calib_ws,
                                                                         VanadiumWorkspace=van_ws,
                                                                         Bank='2',
                                                                         ExpectedPeaks=
                                                                         '2.7057,1.9132,1.6316,1.5621,1.3528,1.1046',
                                                                         DetectorPositions=self.pos_table)
        self.peaks_b2 = tbl_b2.column('dSpacing')
        self.peaks_fitted_b2 = tbl_b2.column('X0')

    def validate(self):
        # === check detector positions table produced by EnggCalibrateFull
        self.assertTrue(self.pos_table)
        self.assertEqual(self.pos_table.columnCount(), 10)
        self.assertEqual(self.pos_table.rowCount(), 1200)
        self.assertEqual(self.pos_table.cell(88, 0), 100089)  # det ID
        self.assertEqual(self.pos_table.cell(200, 0), 101081)  # det ID
        self.assertEqual(self.pos_table.cell(88, 0), 100089)  # det ID

        # The output table of peak parameters has the expected structure
        self.assertEqual(self.peaks_info.rowCount(), 1200)
        self.assertEqual(self.peaks_info.columnCount(), 2)
        self.assertEqual(self.peaks_info.keys(), ['Detector ID', 'Parameters'])
        self.assertEqual(self.peaks_info.cell(10, 0), 100011)
        self.assertEqual(self.peaks_info.cell(100, 0), 100101)
        self.assertEqual(self.peaks_info.cell(123, 0), 101004)
        self.assertEqual(self.peaks_info.cell(517, 0), 104038)
        self.assertEqual(self.peaks_info.cell(987, 0), 108028)
        self.assertEqual(self.peaks_info.cell(1000, 0), 108041)
        for idx in [0, 12, 516, 789, 891, 1112]:
            cell_val = self.peaks_info.cell(idx, 1)
            self.assertTrue(isinstance(cell_val, str))
            if PY2:  # This test depends on consistent ordering of a dict which is not guaranteed for python 3
                self.assertEqual(cell_val[0:11], '{"1": {"A":')
            self.assertEqual(cell_val[-2:], '}}')

        self.assertEqual(self.difa, 0)
        self.assertEqual(self.difa_b2, 0)

        # this will be used as a comparison delta in relative terms (percentage)
        exdelta_special = 5e-4
        # Mac fitting tests produce large differences for some reason.
        # Windows results are different but within reasonable bounds
        import sys
        if "darwin" == sys.platform:
            # Some tests need a bigger delta
            exdelta_special = 1e-1

        # Note that the reference values are given with 12 digits more for reference than
        # for assert-comparison purposes (comparisons are not that picky, by far)
        single_spectrum_delta = 5e-3
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(100, 3), 1.53041625023, single_spectrum_delta))
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(400, 4), 1.65264105797, single_spectrum_delta))
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(200, 5), -0.296705961227, single_spectrum_delta))
        # DIFA column
        self.assertEqual(self.pos_table.cell(133, 7), 0)
        # DIFC column
        self.assertTrue(rel_err_less_delta(self.pos_table.cell(610, 8), 18603.7597656, single_spectrum_delta))
        # TZERO column
        self.assertTrue(abs(self.pos_table.cell(1199, 9)) < 10)

        # === check difc, zero parameters for GSAS produced by EnggCalibrate

        # Bank 1
        self.assertTrue(rel_err_less_delta(self.difc, 18401.881659, exdelta_special),
                        "difc parameter for bank 1 is not what was expected, got: %f" % self.difc)
        if "darwin" != sys.platform:
            self.assertTrue(abs(self.zero) < 9,
                            "zero parameter for bank 1 is not what was expected, got: %f" % self.zero)

        # Bank 2
        self.assertTrue(rel_err_less_delta(self.difc_b2, 18389.841183, exdelta_special),
                        "difc parameter for bank 2 is not what was expected, got: %f" % self.difc_b2)
        if "darwin" != sys.platform:
            self.assertTrue(abs(self.zero_b2) < 18,
                            "zero parameter for bank 2 is not what was expected, got: %f" % self.zero_b2)

        # === peaks used to fit the difc and zero parameters ===
        expected_peaks = [1.1046, 1.3528, 1.5621, 1.6316, 1.9132, 2.7057]
        # Note that CalibrateFull is not applied on bank 2. These peaks are not too difficult and
        # fitted successfully though (but note the increased DIFC).
        self.assertEqual(len(self.peaks), len(expected_peaks))
        self.assertEqual(len(self.peaks_b2), len(expected_peaks))
        self.assertEqual(self.peaks, expected_peaks)
        self.assertEqual(self.peaks_b2, expected_peaks)
        self.assertEqual(len(self.peaks_fitted), len(expected_peaks))
        self.assertEqual(len(self.peaks_fitted_b2), len(expected_peaks))

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
