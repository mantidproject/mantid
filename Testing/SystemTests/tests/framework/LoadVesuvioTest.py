# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init,too-many-public-methods,too-many-arguments
import systemtesting

from mantid.api import FileFinder, MatrixWorkspace, mtd
import mantid.simpleapi as ms

import math
import unittest

DIFF_PLACES = 12


class VesuvioTests(unittest.TestCase):
    ws_name = "evs_raw"

    def tearDown(self):
        if self.ws_name in mtd:
            mtd.remove(self.ws_name)
        monitor_name = self.ws_name + "_monitors"
        if monitor_name in mtd:
            mtd.remove(monitor_name)

    # ==================== Test spectrum list validation ============================

    def test_spectrum_list_single_range(self):
        diff_mode = "DoubleDifference"
        self._run_load("14188", "10-20", diff_mode)

        # check workspace created
        self.assertTrue(mtd.doesExist(self.ws_name))

    def test_spectrum_list_comma_separated_list(self):
        diff_mode = "DoubleDifference"
        self._run_load("14188", "10,20,30,40", diff_mode)

        # check workspace created
        self.assertTrue(mtd.doesExist(self.ws_name))

    def test_spectrum_list_comma_separated_ranges(self):
        diff_mode = "DoubleDifference"
        self._run_load("14188", "10-20;30-40", diff_mode, do_size_check=False)

        # check workspace created
        self.assertTrue(mtd.doesExist(self.ws_name))

    # ================== Success cases ================================

    def test_filename_accepts_full_filepath(self):
        diff_mode = "FoilOut"
        rawfile = FileFinder.getFullPath("EVS14188.raw")
        self._run_load(rawfile, "3", diff_mode)
        self.assertTrue(mtd.doesExist("evs_raw"))
        self.assertEqual(mtd["evs_raw"].getNumberHistograms(), 1)

    def test_filename_accepts_filename_no_path(self):
        diff_mode = "FoilOut"
        self._run_load("EVS14188.raw", "3", diff_mode)
        self.assertTrue(mtd.doesExist("evs_raw"))
        self.assertEqual(mtd["evs_raw"].getNumberHistograms(), 1)

    def test_filename_accepts_run_and_ext(self):
        diff_mode = "FoilOut"
        self._run_load("14188.raw", "3", diff_mode)
        self.assertTrue(mtd.doesExist("evs_raw"))
        self.assertEqual(mtd["evs_raw"].getNumberHistograms(), 1)

    def test_load_with_back_scattering_spectra_produces_correct_workspace_using_double_difference(self):
        diff_mode = "DoubleDifference"
        self._run_load("14188", "3-134", diff_mode)

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.13015715643321046, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.12122048601642356, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.017172642169849039, evs_raw.readY(131)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.063124106780391834, evs_raw.readE(131)[1188], places=DIFF_PLACES)

        self._verify_correct_parameters_loaded(evs_raw, forward_scatter=False, diff_mode=diff_mode)

    def test_monitors_loaded_into_ADS_when_monitor_load_is_true_for_back_scattering(self):
        diff_mode = "SingleDifference"
        self._run_load("14188", "3-134", diff_mode, load_mon=True)
        self.assertTrue(mtd.doesExist("evs_raw_monitors"))
        monitor_ws = mtd["evs_raw_monitors"]
        self.assertTrue(isinstance(monitor_ws, MatrixWorkspace))
        self.assertEqual(monitor_ws.readX(0)[0], 5)
        self.assertEqual(monitor_ws.readX(0)[-1], 19990)

    def test_monitors_loaded_into_ADS_when_monitor_load_is_true_for_forward_scattering(self):
        diff_mode = "SingleDifference"
        self._run_load("14188", "135-198", diff_mode, load_mon=True)
        self.assertTrue(mtd.doesExist("evs_raw_monitors"))
        self.assertTrue(isinstance(mtd["evs_raw_monitors"], MatrixWorkspace))

    def test_monitors_loaded_when_LoadMonitors_is_true_for_multiple_runs_back_scattering(self):
        diff_mode = "SingleDifference"
        self._run_load("14188-14190", "3-134", diff_mode, load_mon=True)
        self.assertTrue(mtd.doesExist("evs_raw_monitors"))
        self.assertTrue(isinstance(mtd["evs_raw_monitors"], MatrixWorkspace))

    def test_monitors_loaded_when_LoadMonitors_is_true_for_multiple_runs_forward_scattering(self):
        diff_mode = "SingleDifference"
        self._run_load("14188-14190", "135-198", diff_mode, load_mon=True)
        self.assertTrue(mtd.doesExist("evs_raw_monitors"))
        self.assertTrue(isinstance(mtd["evs_raw_monitors"], MatrixWorkspace))

    def test_monitor_is_not_loaded_when_LoadMonitors_is_false(self):
        diff_mode = "SingleDifference"
        self._run_load("14188-14190", "3-134", diff_mode, load_mon=False)
        self.assertFalse(mtd.doesExist("evs_raw_monitors"))

    def test_monitor_is_loaded_for_non_differencing_mode(self):
        diff_mode = "FoilOut"
        self._run_load("14188", "135-198", diff_mode, load_mon=True)
        self.assertTrue(mtd.doesExist("evs_raw_monitors"))
        self.assertTrue(isinstance(mtd["evs_raw_monitors"], MatrixWorkspace))

    def test_monitor_is_not_loaded_for_non_differencing_mode_when_LoadMonitors_false(self):
        diff_mode = "FoilOut"
        self._run_load("14188", "135-198", diff_mode, load_mon=False)
        self.assertFalse(mtd.doesExist("evs_raw_monitors"))

    def test_monitor_loaded_in_ws_when_mon_in_spectra_input_and_LoadMonitor_is_true(self):
        diff_mode = "FoilOut"
        self._run_load("14188", "1-198", diff_mode, load_mon=True)
        self.assertTrue(mtd.doesExist("evs_raw"))
        self.assertEqual(mtd["evs_raw"].getNumberHistograms(), 198)
        self.assertFalse(mtd.doesExist("evs_raw_monitors"))

    def test_load_with_back_scattering_spectra_produces_correct_workspace_using_single_difference(self):
        diff_mode = "SingleDifference"
        self._run_load("14188", "3-134", diff_mode)

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.16805529043135614, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.13602628474190004, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.056426592449087654, evs_raw.readY(131)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.070774572171486652, evs_raw.readE(131)[1188], places=DIFF_PLACES)

        self._verify_correct_parameters_loaded(evs_raw, forward_scatter=False, diff_mode=diff_mode)

    def test_load_with_forward_scattering_spectra_produces_correct_workspace(self):
        diff_mode = "SingleDifference"
        self._run_load("14188", "135-198", diff_mode)

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(-0.4421157823659172, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.23849110331150025, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.030129475930755989, evs_raw.readY(63)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.23849110331150025, evs_raw.readE(0)[1], places=DIFF_PLACES)

        self._verify_correct_parameters_loaded(evs_raw, forward_scatter=True, diff_mode=diff_mode)

    def test_consecutive_runs_with_back_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188-14190", "3-134", "DoubleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.14595792251532602, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.069974931114835631, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.034269132557441905, evs_raw.readY(131)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.036773635912201605, evs_raw.readE(131)[1188], places=DIFF_PLACES)

    def test_non_consecutive_runs_with_back_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188,14190", "3-134", "DoubleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.17587447223331631, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.085647015119071523, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.031951030862195084, evs_raw.readY(131)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.044999174645580703, evs_raw.readE(131)[1188], places=DIFF_PLACES)

    def test_consecutive_runs_with_forward_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188-14190", "135-198", "SingleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(-0.33023675686822429, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.13839181298987582, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.0005762703884557574, evs_raw.readY(63)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.022314627606989094, evs_raw.readE(63)[1188], places=DIFF_PLACES)

    def test_non_consecutive_runs_with_forward_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188,14190", "135-198", "SingleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(-0.31382658620745474, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.16935354944452052, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.0013599866184859088, evs_raw.readY(63)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.16935354944452052, evs_raw.readE(0)[1], places=DIFF_PLACES)

    def test_foilout_mode_gives_expected_numbers(self):
        self._run_load("14188", "3", "FoilOut")

        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(18753.00, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(136.94159338929865, evs_raw.readE(0)[1], places=DIFF_PLACES)

    def test_foilin_mode_gives_expected_numbers(self):
        self._run_load("14188", "3", "FoilIn")

        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(37594.0, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(193.89172236070317, evs_raw.readE(0)[1], places=DIFF_PLACES)

    def test_using_ip_file_adjusts_instrument_and_attaches_parameters_foil_mode(self):
        self._run_load("14188", "3", "FoilOut", "IP0005.dat")

        # Check some data
        evs_raw = mtd[self.ws_name]
        det0 = evs_raw.getDetector(0)
        param = det0.getNumberParameter("t0")
        self.assertEqual(1, len(param))
        self.assertAlmostEqual(-0.4157, param[0], places=4)

    def test_sumspectra_set_to_true_gives_single_spectra_summed_over_all_inputs(self):
        self._run_load("14188", "135-142", "SingleDifference", "IP0005.dat", sum_runs=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEqual(1, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(599.5, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(-1.5288171762918328, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.079412793053402098, evs_raw.readY(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.52109203357613976, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.10617318614513051, evs_raw.readE(0)[-1], places=DIFF_PLACES)

    def test_sumspectra_with_multiple_groups_gives_number_output_spectra_as_input_groups(self):
        self._run_load("14188", "135-148;152-165", "SingleDifference", "IP0005.dat", sum_runs=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEqual(2, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(5.0, evs_raw.readX(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(599.5, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(599.5, evs_raw.readX(1)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.713877795283, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(-3.00125465604, evs_raw.readY(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.6219299465, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.676913729914, evs_raw.readE(1)[0], places=DIFF_PLACES)

        # Spectrum numbers
        self._verify_spectra_numbering(evs_raw.getSpectrum(0), 135, range(3101, 3115))
        self._verify_spectra_numbering(evs_raw.getSpectrum(1), 152, range(3118, 3132))

    def test_sumspectra_set_to_true_gives_single_spectra_summed_over_all_inputs_with_foil_in(self):
        self._run_load("14188", "3-15", "FoilIn", "IP0005.dat", sum_runs=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEqual(1, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(19990.0, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(497722.0, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(2072.0, evs_raw.readY(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(705.49415305869115, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(45.519226706964169, evs_raw.readE(0)[-1], places=DIFF_PLACES)

        self._verify_spectra_numbering(evs_raw.getSpectrum(0), 3, range(2101, 2114))

    def test_sumspectra_with_multiple_groups_gives_number_output_spectra_as_input_groups_with_foil_in(self):
        self._run_load("14188", "3-15;30-50", "FoilIn", "IP0005.dat", sum_runs=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEqual(2, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(5.0, evs_raw.readX(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(19990.0, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(19990.0, evs_raw.readX(1)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(497722.0, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(1332812.0, evs_raw.readY(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(705.49415305869115, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(1154.4747723532116, evs_raw.readE(1)[0], places=DIFF_PLACES)

        self._verify_spectra_numbering(evs_raw.getSpectrum(0), 3, range(2101, 2114))
        self._verify_spectra_numbering(evs_raw.getSpectrum(1), 30, list(range(2128, 2145)) + list(range(2201, 2205)))

    def _verify_spectra_numbering(self, spectrum, expected_no, expected_ids):
        self.assertEqual(expected_no, spectrum.getSpectrumNo())
        det_ids = spectrum.getDetectorIDs()
        for expected_id, det_id in zip(expected_ids, det_ids):
            self.assertEqual(expected_id, det_id)

    def _verify_correct_parameters_loaded(self, workspace, forward_scatter, diff_mode):
        nhist = workspace.getNumberHistograms()
        for i in range(nhist):
            self._verify_correct_detector_parameters(workspace.getDetector(i), forward_scatter, diff_mode)

    def _verify_correct_detector_parameters(self, detector, forward_scatter, diff_mode):
        # resolution
        tol = 1e-04
        # using decimal 'places' keyword, as delta= is not supported on Python < 2.7
        tol_places = round(-math.log10(tol), ndigits=0)
        sigma_l1 = detector.getNumberParameter("sigma_l1")[0]
        sigma_l2 = detector.getNumberParameter("sigma_l2")[0]
        sigma_tof = detector.getNumberParameter("sigma_tof")[0]
        sigma_theta = detector.getNumberParameter("sigma_theta")[0]
        sigma_gauss = detector.getNumberParameter("sigma_gauss")[0]
        hwhm_lorentz = detector.getNumberParameter("hwhm_lorentz")[0]
        self.assertAlmostEqual(sigma_l1, 0.021, places=tol_places)
        self.assertAlmostEqual(sigma_l2, 0.023, places=tol_places)
        self.assertAlmostEqual(sigma_tof, 0.370, places=tol_places)
        if forward_scatter:
            self.assertAlmostEqual(sigma_theta, 0.016, places=tol_places)
            if diff_mode == "DoubleDifference":
                raise ValueError("Double difference is not compataible with forward scattering spectra")
            else:
                self.assertAlmostEqual(sigma_gauss, 73, places=tol_places)
                self.assertAlmostEqual(hwhm_lorentz, 24, places=tol_places)
        else:
            self.assertAlmostEqual(sigma_theta, 0.016, places=tol_places)
            if diff_mode == "DoubleDifference":
                self.assertAlmostEqual(sigma_gauss, 88.7, places=tol_places)
                self.assertAlmostEqual(hwhm_lorentz, 40.3, places=tol_places)
            else:
                self.assertAlmostEqual(sigma_gauss, 52.3, places=tol_places)
                self.assertAlmostEqual(hwhm_lorentz, 141.2, places=tol_places)

    def _run_load(self, runs, spectra, diff_opt, ip_file="", sum_runs=False, load_mon=False, do_size_check=True):
        ms.LoadVesuvio(
            Filename=runs,
            OutputWorkspace=self.ws_name,
            SpectrumList=spectra,
            Mode=diff_opt,
            InstrumentParFile=ip_file,
            SumSpectra=sum_runs,
            LoadMonitors=load_mon,
        )

        self._do_ads_check(self.ws_name)

        def expected_size():
            if sum_runs:
                if ";" in spectra:
                    return 2
                else:
                    return 1
            elif "-" in spectra:
                elements = spectra.split("-")
                min_e, max_e = (int(elements[0]), int(elements[1]))
                return max_e - min_e + 1
            elif "," in spectra:
                elements = spectra.strip().split(",")
                return len(elements)
            else:
                return 1

        if do_size_check:
            self._do_size_check(self.ws_name, expected_size())

        loaded_data = mtd[self.ws_name]
        if "Difference" in diff_opt:
            self.assertTrue(not loaded_data.isHistogramData())
        else:
            self.assertTrue(loaded_data.isHistogramData())

    def _do_ads_check(self, name):
        self.assertTrue(name in mtd)
        self.assertTrue(isinstance(mtd[name], MatrixWorkspace))

    def _do_size_check(self, name, expected_nhist):
        loaded_data = mtd[name]
        self.assertEqual(expected_nhist, loaded_data.getNumberHistograms())

    # ================== Failure cases ================================

    def test_run_range_bad_order_raises_error(self):
        self.assertRaises(TypeError, ms.LoadVesuvio, Filename="14188-14187", OutputWorkspace=self.ws_name)

    def test_missing_spectra_property_raises_error(self):
        self.assertRaises(TypeError, ms.LoadVesuvio, Filename="14188", OutputWorkspace=self.ws_name)

    def test_load_with_invalid_spectra_raises_error(self):
        self.assertRaises(RuntimeError, ms.LoadVesuvio, Filename="14188", OutputWorkspace=self.ws_name, SpectrumList="200")

    def test_load_with_spectra_that_are_just_monitors_raises_error(self):
        self.assertRaises(RuntimeError, ms.LoadVesuvio, Filename="14188", OutputWorkspace=self.ws_name, SpectrumList="1")
        self.assertRaises(RuntimeError, ms.LoadVesuvio, Filename="14188", OutputWorkspace=self.ws_name, SpectrumList="1-2")

    def test_load_with_spectra_mixed_from_forward_backward_raises_error(self):
        self.assertRaises(
            RuntimeError, ms.LoadVesuvio, Filename="14188", Mode="SingleDifference", OutputWorkspace=self.ws_name, SpectrumList="135,134"
        )
        self.assertRaises(
            RuntimeError,
            ms.LoadVesuvio,
            Filename="14188",
            Mode="SingleDifference",
            OutputWorkspace=self.ws_name,
            SpectrumList="3,134,136,198",
        )
        self.assertRaises(
            RuntimeError,
            ms.LoadVesuvio,
            Filename="14188",
            Mode="SingleDifference",
            OutputWorkspace=self.ws_name,
            SpectrumList="20-50,180-192",
        )

    def test_load_with_invalid_difference_option_raises_error(self):
        self.assertRaises(ValueError, ms.LoadVesuvio, Filename="14188", OutputWorkspace=self.ws_name, Mode="Unknown", SpectrumList="3-134")

    def test_load_with_difference_option_not_applicable_to_current_spectra_raises_error(self):
        self.assertRaises(ValueError, ms.LoadVesuvio, Filename="14188", OutputWorkspace=self.ws_name, Mode="", SpectrumList="3-134")

    def test_forward_scattering_spectra_with_double_difference_mode_raises_error(self):
        self.assertRaises(
            RuntimeError, ms.LoadVesuvio, Filename="14188", Mode="DoubleDifference", OutputWorkspace=self.ws_name, SpectrumList="140-150"
        )

    def test_raising_error_removes_temporary_raw_workspaces(self):
        self.assertRaises(
            RuntimeError,
            ms.LoadVesuvio,
            Filename="14188,14199",  # Second run is invalid
            OutputWorkspace=self.ws_name,
            Mode="SingleDifference",
            SpectrumList="3-134",
        )

        self._do_test_temp_raw_workspaces_not_left_around()

    def _do_test_temp_raw_workspaces_not_left_around(self):
        self.assertTrue("__loadraw_evs" not in mtd)
        self.assertTrue("__loadraw_evs_monitors" not in mtd)


# ====================================================================================


class LoadVesuvioTest(systemtesting.MantidSystemTest):
    _success = False

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(VesuvioTests, "test"))
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success
