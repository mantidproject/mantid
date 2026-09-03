# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Run Processing tab on ENGIN-X.

Covers the manual guide's Test 1 (calibrate and focus), its settings steps, the plot-output
checkboxes, the RB number's effect on the save layout, and the "restart the interface" step that
checks the last calibration is restored.

The ceria and vanadium runs are **fabricated** rather than the real 305738/307521 pair, in the same
way as the IMAT tests - see ``create_synthetic_ceria_and_vanadium``. The real runs are large and
slow, and what these tests assert is the interface, the reported state and the on-disk save layout,
none of which depends on the counts being real. The numerical correctness of the calibrate/focus
chain is already covered by ``EnginXScriptTest``, so duplicating it here would only add fixtures that
need regenerating whenever an algorithm changes.

Because the fabricated peaks are Gaussian, every class here sets ENGIN-X's Default Peak Function to
``Gaussian`` so the fitted shape matches the generated one - which doubles as coverage of that
setting, asserted from the captured log.
"""

import math
import os

from eng_diff_gui_test_base import (
    ENGINX_SYNTHETIC_CERIA_RUN,
    ENGINX_SYNTHETIC_VANADIUM_RUN,
    EngDiffGuiTestBase,
    TAB_RUN_PROCESSING,
    create_enginx_ceria_and_vanadium,
)
from qt_interaction_helpers import figure_numbers, process_events, set_checkbox

INSTRUMENT = "ENGINX"
CERIA = str(ENGINX_SYNTHETIC_CERIA_RUN)
VANADIUM = str(ENGINX_SYNTHETIC_VANADIUM_RUN)

# the two ENGIN-X banks, focused together when no region of interest is set
N_BANKS = 2


class _RunProcessingTestBase(EngDiffGuiTestBase):
    """Stages the fabricated ENGIN-X runs and points the interface's file finder at them."""

    def seeded_settings(self):
        settings = super(_RunProcessingTestBase, self).seeded_settings()
        # match the shape the fixture generates; ENGIN-X's real default is BackToBackExponential,
        # which cannot be fitted to a symmetric Gaussian peak
        settings["default_peak_ENGINX"] = "Gaussian"
        return settings

    def pre_gui_setup(self):
        self.data_dir = os.path.join(self.tmp_root, "enginx_data")
        os.makedirs(self.data_dir, exist_ok=True)
        create_enginx_ceria_and_vanadium(self.data_dir)
        # this is what lets the interface resolve the fabricated runs from a run number, exactly as
        # it would a real one, instead of the test bypassing the file finder
        self.add_data_search_dir(self.data_dir)

    # ------------------------------------------------------------------ shared expectations

    def focused_basename(self, suffix, xunit):
        """Focused output files are named ``INSTRUMENT_sample_vanadium_suffix_xunit``."""
        return f"{INSTRUMENT}_{CERIA}_{VANADIUM}_{suffix}_{xunit}"


class EngDiffGuiCalibrateAndFocusTest(_RunProcessingTestBase):
    """Guide Test 1: a new calibration with no region of interest, then focusing the ceria run.

    The bulk of the coverage - the calibration state, the files written by both the calibration and
    the focus, and the focused workspaces themselves.
    """

    def test_calibrate_and_focus(self):
        self.show_tab(TAB_RUN_PROCESSING)
        self._check_initial_state()

        self.set_region_of_interest(None)
        with self.captured_logs(level="notice") as logs:
            calibration = self.calibrate(ceria=CERIA, vanadium=VANADIUM)

        # preconditions: nothing below is meaningful without a calibration
        self.assertIsNotNone(calibration, "no calibration was produced")
        self.assertTrue(calibration.is_valid(), "the calibration reported itself invalid")

        self._check_calibration_state(calibration, logs)
        self._check_calibration_files()
        self._check_calibration_succeeded()
        self._check_focus()

    def _check_initial_state(self):
        view = self.run_processing_view
        with self.subTest("Test 1 / step 3 (Create New Calibration is preselected on a clean setup)"):
            # nothing has been calibrated in this isolated settings store, so there is no last
            # calibration to restore and the interface must offer to make one
            self.assertTrue(view.get_new_checked())
            self.assertFalse(view.get_load_checked())

        with self.subTest("Test 1 / step 3 (the existing calibration path field is disabled)"):
            self.assertFalse(view.finder_path.isEnabled())

        with self.subTest("Test 1 / step 3 (no calibration is reported in the status bar)"):
            self.assertEqual("No Calibration Loaded.", self.statusbar_text())

        with self.subTest("Test 1 / step 4 (the save location reported is the one in settings)"):
            self.assertIn(self.save_dir, self.savedir_text())

    def _check_calibration_state(self, calibration, logs):
        from Engineering.common.instrument_config import ENGINX_GROUP

        with self.subTest("Test 1 / step 11 (the status bar reports the new calibration)"):
            self.assertEqual(
                f"CeO2: {CERIA}, V: {VANADIUM}, Instrument: {INSTRUMENT}",
                self.statusbar_text(),
            )

        with self.subTest("Test 1 / step 11 (the calibration records both banks and both runs)"):
            self.assertEqual(ENGINX_GROUP.BOTH, calibration.get_group())
            self.assertEqual(CERIA, calibration.get_ceria_runno())
            self.assertEqual(VANADIUM, calibration.get_vanadium_runno())
            self.assertEqual(INSTRUMENT, calibration.get_instrument())

        with self.subTest("Test 1 / steps 4-7 (the Default Peak Function setting is the one used)"):
            # the setting is only observable through this log line and the calibration's own record
            self.assertIn("Gaussian", logs.text)
            self.assertEqual("Gaussian", calibration.get_fit_peak_shape())

        with self.subTest("Test 1 / step 11 (the diffractometer constants table is produced)"):
            from mantid.api import AnalysisDataService as ADS
            from Engineering.EnggUtils import DIFF_CONSTS_TABLE_NAME

            self.assertTrue(ADS.doesExist(DIFF_CONSTS_TABLE_NAME), f"{DIFF_CONSTS_TABLE_NAME} was not produced")
            self.assertEqual(N_BANKS, ADS.retrieve(DIFF_CONSTS_TABLE_NAME).rowCount())

    def _check_calibration_files(self):
        calibration_dir = self.calibration_dir()
        written = self.basenames_under(calibration_dir)

        with self.subTest("Test 1 / step 12 (a prm and nxs are written for each bank and for both)"):
            for suffix in ("all_banks", "bank_1", "bank_2"):
                for extension in (".prm", ".nxs"):
                    self.assertIn(f"{INSTRUMENT}_{CERIA}_{suffix}{extension}", written)

        with self.subTest("Test 1 / step 12 (nothing is written under User/ without an RB number)"):
            self.assertEqual([], self.files_under(os.path.join(self.save_dir, "User")))

        with self.subTest("Test 1 / step 12 (the prm is built from the ENGIN-X header template)"):
            from Engineering.EnggUtils import CALIB_DIR

            with open(self._all_banks_prm()) as written_prm:
                contents = written_prm.read()
            with open(os.path.join(CALIB_DIR, "template_ENGINX_prm_header.prm")) as template:
                first_line = template.readline().strip()
            self.assertIn(first_line, contents)
            self.assertIn("ICONS", contents)
            # the header carries the run number of the ceria run it was made from
            self.assertIn(CERIA, contents)

        with self.subTest("Test 1 / step 12 (the written prm parses back into diffractometer constants)"):
            from Engineering.EnggUtils import read_diff_constants_from_prm

            constants = read_diff_constants_from_prm(self._all_banks_prm())
            self.assertEqual(N_BANKS, len(constants), f"expected one row per bank, got {constants}")
            for row in constants:
                for value in row:
                    self.assertFalse(math.isnan(value), "a diffractometer constant is NaN")

        with self.subTest("Test 1 / step 12 (the per-bank prm files hold one bank each)"):
            from Engineering.EnggUtils import read_diff_constants_from_prm

            for bank in (1, 2):
                prm = os.path.join(calibration_dir, f"{INSTRUMENT}_{CERIA}_bank_{bank}.prm")
                self.assertEqual(1, len(read_diff_constants_from_prm(prm)), f"{prm} should describe one bank")

        # SOFT: run_calibration keeps the uncalibrated DIFC and only logs a warning when
        # PDCalibration fails, so a zero here means the fabricated peaks did not fit that bank.
        # That is a signal about this fixture rather than a regression in the interface, which is
        # what the rest of the class tests.
        with self.subTest("Test 1 / both banks got a fitted difc (data quality, soft)"):
            from Engineering.EnggUtils import read_diff_constants_from_prm

            unfitted = [index for index, row in enumerate(read_diff_constants_from_prm(self._all_banks_prm())) if row[0] <= 0.0]
            self.assertEqual([], unfitted, f"PDCalibration produced no difc for bank(s) {unfitted}")

    def _check_calibration_succeeded(self):
        # SOFT, for the same reason as the difc check above.
        from mantid.api import AnalysisDataService as ADS

        mask_name = "engggui_calibration_all_banks_mask"
        with self.subTest("Test 1 / PDCalibration fitted the ENGIN-X banks (data quality, soft)"):
            self.assertTrue(ADS.doesExist(mask_name), f"{mask_name} was not produced")
            mask = ADS.retrieve(mask_name)
            total = mask.getNumberHistograms()
            masked = sum(1 for index in range(total) if mask.readY(index)[0] != 0)
            # the mask spans the whole instrument while the fabricated run populates a subset of the
            # detectors, so most entries are legitimately masked; what matters is that the focused
            # banks were fitted at all
            self.assertLess(masked, total, "PDCalibration failed for every spectrum")

    def _check_focus(self):
        self.focus(runs=CERIA)

        with self.subTest("Test 1 / step 15 (focusing produces one workspace with one spectrum per bank)"):
            from mantid.api import AnalysisDataService as ADS

            focused = self.focused_workspace_names()
            self.assertEqual(1, len(focused), f"expected one focused workspace, got {focused}")
            self.assertTrue(focused[0].startswith(CERIA), f"{focused[0]} is not named for the focused run")
            self.assertEqual(N_BANKS, ADS.retrieve(focused[0]).getNumberHistograms())

        with self.subTest("Test 1 / step 15 (the focused output is left in TOF)"):
            from mantid.api import AnalysisDataService as ADS

            focused = ADS.retrieve(self.focused_workspace_names()[0])
            self.assertEqual("TOF", focused.getAxis(0).getUnit().unitID())

        focus_dir = self.focus_dir()
        written = self.basenames_under(focus_dir)

        with self.subTest("Test 1 / step 15 (ASCII output is written for the whole run in TOF)"):
            self.assertIn(self.focused_basename("all_banks", "TOF") + ".gss", written)
            self.assertIn(self.focused_basename("all_banks", "TOF") + ".abc", written)

        with self.subTest("Test 1 / step 15 (ASCII output is also written in d-spacing)"):
            self.assertIn(self.focused_basename("all_banks", "dSpacing") + ".gss", written)
            self.assertIn(self.focused_basename("all_banks", "dSpacing") + ".abc", written)

        with self.subTest("Test 1 / step 15 (a nexus file is written per bank, in both units)"):
            for bank in (1, 2):
                for xunit in ("TOF", "dSpacing"):
                    self.assertIn(self.focused_basename(f"bank_{bank}", xunit) + ".nxs", written)

        with self.subTest("Test 1 / step 15 (the d-spacing spectra are also saved combined)"):
            combined = self.basenames_under(os.path.join(focus_dir, "CombinedFiles"))
            self.assertEqual([self.focused_basename("bank", "dSpacing") + ".nxs"], combined)

        with self.subTest("Test 1 / step 15 (the focused workspace records the vanadium it was normalised by)"):
            from mantid.api import AnalysisDataService as ADS

            run = ADS.retrieve(self.focused_workspace_names()[0]).run()
            self.assertEqual(VANADIUM, run.getLogData("Vanadium Run").value)
            self.assertEqual("bank", run.getLogData("Grouping").value)

    def _all_banks_prm(self):
        return os.path.join(self.calibration_dir(), f"{INSTRUMENT}_{CERIA}_all_banks.prm")


class EngDiffGuiPlotOutputTest(_RunProcessingTestBase):
    """Guide Test 1: the 'Plot Calibrated Workspace' and 'Plot Focused Workspace' checkboxes.

    Uses the North bank only - one group instead of two makes both the calibration and the focus
    noticeably cheaper, and the checkbox behaviour does not depend on the region of interest.
    """

    def test_plot_output(self):
        self.set_region_of_interest("1 (North)")

        before = figure_numbers()
        self.calibrate(ceria=CERIA, vanadium=VANADIUM, plot_output=False)
        with self.subTest("Test 1 / step 9 (no plot appears when Plot Calibrated Workspace is off)"):
            self.assertEqual(before, figure_numbers())

        self.focus(runs=CERIA, plot_output=False)
        with self.subTest("Test 1 / step 14 (no plot appears when Plot Focused Workspace is off)"):
            self.assertEqual(before, figure_numbers())

        # calibrate again with the box ticked; the calibration itself is unchanged, so any new
        # figure can only have come from the checkbox
        before = figure_numbers()
        self.calibrate(ceria=CERIA, vanadium=VANADIUM, plot_output=True)
        with self.subTest("Test 1 / step 9 (a plot appears when Plot Calibrated Workspace is on)"):
            self.assertTrue(figure_numbers() - before, "no new figure was created by the calibration")

        before = figure_numbers()
        self.focus(runs=CERIA, plot_output=True)
        with self.subTest("Test 1 / step 14 (a plot appears when Plot Focused Workspace is on)"):
            self.assertTrue(figure_numbers() - before, "no new figure was created by the focus")

        with self.subTest("Test 1 / step 9 (the checkbox state is what the view reports)"):
            view = self.run_processing_view
            self.assertTrue(view.get_plot_output())
            set_checkbox(view.check_plotOutput, False)
            self.assertFalse(view.get_plot_output())


class EngDiffGuiLoadExistingCalibrationTest(_RunProcessingTestBase):
    """Guide Test 1: closing and reopening the interface, then loading a calibration by path."""

    def test_load_existing_calibration(self):
        self.set_region_of_interest(None)
        calibration = self.calibrate(ceria=CERIA, vanadium=VANADIUM)
        self.assertTrue(calibration.is_valid(), "the calibration reported itself invalid")

        all_banks_prm = calibration.get_prm_filepath()
        self.assertTrue(os.path.exists(all_banks_prm), f"{all_banks_prm} was not written")

        self.rebuild_gui()

        with self.subTest("Test 1 / step 16 (reopening preselects Load Existing Calibration)"):
            self.assertTrue(self.run_processing_view.get_load_checked())
            self.assertFalse(self.run_processing_view.get_new_checked())

        with self.subTest("Test 1 / step 16 (the path field is prefilled with the last calibration)"):
            from qt_interaction_helpers import wait_for_file_finder

            wait_for_file_finder(self.run_processing_view.finder_path, msg="restored calibration path")
            self.assertEqual(
                os.path.normcase(all_banks_prm),
                os.path.normcase(self.run_processing_view.get_path_filename()),
            )

        with self.subTest("Test 1 / step 16 (the last vanadium run is restored too)"):
            from qt_interaction_helpers import wait_for_file_finder

            wait_for_file_finder(self.run_processing_view.finder_vanadium, msg="restored vanadium run")
            self.assertIn(VANADIUM, self.run_processing_view.finder_vanadium.getText())

        with self.subTest("Test 1 / step 16 (the calibrate button is relabelled for the load path)"):
            self.assertEqual("Load", self.run_processing_view.button_calibrate.text())

        # now browse to a single bank instead, which is the guide's "load a different calibration"
        bank_2_prm = os.path.join(self.calibration_dir(), f"{INSTRUMENT}_{CERIA}_bank_2.prm")
        loaded = self.load_calibration(bank_2_prm)

        with self.subTest("Test 1 / step 17 (loading a bank prm reports that calibration)"):
            from Engineering.common.instrument_config import ENGINX_GROUP

            self.assertIsNotNone(loaded, "no calibration was loaded")
            self.assertTrue(loaded.is_valid(), "the loaded calibration reported itself invalid")
            self.assertEqual(ENGINX_GROUP.SOUTH, loaded.get_group())
            self.assertEqual(CERIA, loaded.get_ceria_runno())
            self.assertEqual(INSTRUMENT, loaded.get_instrument())

        with self.subTest("Test 1 / step 17 (the status bar reports the loaded calibration)"):
            self.assertIn(f"CeO2: {CERIA}", self.statusbar_text())
            self.assertIn(f"Instrument: {INSTRUMENT}", self.statusbar_text())

        with self.subTest("Test 1 / step 17 (focusing against the loaded calibration gives one spectrum)"):
            from mantid.api import AnalysisDataService as ADS

            self.focus(runs=CERIA)
            focused = self.focused_workspace_names()
            self.assertEqual(1, len(focused), f"expected one focused workspace, got {focused}")
            self.assertEqual(1, ADS.retrieve(focused[0]).getNumberHistograms())


class EngDiffGuiSaveLocationAndRbNumberTest(_RunProcessingTestBase):
    """Guide Test 1: the RB number and changing the save location mid-session.

    North bank only, for the same reason as the plot test - what is being checked is where the
    output lands, not what is in it.
    """

    RB_NUMBER = "1234567"

    def test_save_location_and_rb_number(self):
        self.set_rb_number(self.RB_NUMBER)
        self.set_region_of_interest("1 (North)")
        calibration = self.calibrate(ceria=CERIA, vanadium=VANADIUM)
        self.assertTrue(calibration.is_valid(), "the calibration reported itself invalid")
        self.focus(runs=CERIA)

        self._check_rb_layout()
        self._check_save_location_change()

    def _check_rb_layout(self):
        expected_prm = f"{INSTRUMENT}_{CERIA}_bank_1.prm"

        with self.subTest("Test 1 / step 8 (with an RB number the calibration is saved in both places)"):
            # a non-texture group is written to the plain directory *and* the RB one
            self.assertIn(expected_prm, self.basenames_under(self.calibration_dir()))
            self.assertIn(expected_prm, self.basenames_under(self.calibration_dir(self.RB_NUMBER)))

        with self.subTest("Test 1 / step 8 (the focused output is saved in both places too)"):
            expected_nxs = self.focused_basename("bank_1", "TOF") + ".nxs"
            self.assertIn(expected_nxs, self.basenames_under(self.focus_dir()))
            self.assertIn(expected_nxs, self.basenames_under(self.focus_dir(self.RB_NUMBER)))

        with self.subTest("Test 1 / step 8 (the RB directory is named for the number that was entered)"):
            self.assertTrue(
                os.path.isdir(os.path.join(self.save_dir, "User", self.RB_NUMBER)),
                f"no User/{self.RB_NUMBER} directory under {self.save_dir}",
            )

    def _check_save_location_change(self):
        new_save_dir = os.path.join(self.tmp_root, "relocated_output")
        os.makedirs(new_save_dir, exist_ok=True)
        before = set(self.files_under(self.save_dir))

        # through the real settings dialog rather than by writing QSettings, so the presenter's
        # validation and its save-directory notification are exercised as well
        self.apply_settings(save_location=new_save_dir)

        with self.subTest("Test 1 / steps 4-7 (the interface reports the new save location)"):
            self.assertIn(new_save_dir, self.savedir_text())

        with self.subTest("Test 1 / steps 4-7 (the setting was persisted)"):
            self.assertEqual(new_save_dir, self.get_engineering_setting("save_location"))

        self.focus(runs=CERIA)
        process_events(2)

        with self.subTest("Test 1 / steps 4-7 (subsequent output lands under the new save location)"):
            relocated = self.basenames_under(os.path.join(new_save_dir, "User", self.RB_NUMBER, "Focus"))
            self.assertIn(self.focused_basename("bank_1", "TOF") + ".nxs", relocated)

        with self.subTest("Test 1 / steps 4-7 (nothing further is written under the old save location)"):
            self.assertEqual(before, set(self.files_under(self.save_dir)))
