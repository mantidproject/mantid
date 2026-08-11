# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for Run Processing on IMAT.

The manual guide asks for calibration and focusing "for all instrument options", but the repository
has no IMAT ceria/vanadium pair, so this module fabricates one (see ``create_imat_ceria_and_vanadium``
in the shared base) and puts it where the interface's own file finder will resolve it from a run
number. The interface is therefore driven exactly as it is for ENGIN-X - nothing reaches past the
view to inject a workspace.

Because the data is fabricated, the assertions are about the *pipeline and the interface*, not the
physics: that the IMAT-specific settings are picked up, that calibration and focusing complete, that
the expected files appear with the expected names, and that the status bar and region-of-interest
options are right. The one check that asks whether PDCalibration genuinely succeeded is kept soft,
so a peak-shape regression reports as one clear line rather than aborting the class.
"""

import math
import os

from eng_diff_gui_test_base import (
    EngDiffGuiTestBase,
    IMAT_CERIA_RUN,
    IMAT_VANADIUM_RUN,
    TAB_RUN_PROCESSING,
    create_imat_ceria_and_vanadium,
)
from qt_interaction_helpers import combo_items

INSTRUMENT = "IMAT"
# the IMAT grouping options offered in place of ENGIN-X's texture groupings
EXPECTED_ROI_OPTIONS = (
    "Custom Grouping File",
    "1 (North)",
    "2 (South)",
    "Crop to Spectra",
    "1 Group per module",
    "4 Groups per module",
    "1 Group per row",
    "4 Groups per row",
)


class EngDiffGuiImatSettingsTest(EngDiffGuiTestBase):
    """The instrument-specific configuration, without running a calibration.

    Cheap - it needs no run data at all - so it stays useful as a fast regression check on the
    per-instrument settings even when the calibration test below is excluded for runtime.
    """

    def test_imat_settings_and_defaults(self):
        self.show_tab(TAB_RUN_PROCESSING)
        self.set_instrument(INSTRUMENT)

        with self.check("Run Processing / IMAT offers its own region of interest options"):
            options = combo_items(self.cropping_view.combo_bank)
            self.assertEqual(list(EXPECTED_ROI_OPTIONS), options)

        with self.check("Run Processing / the ENGIN-X texture groupings are not offered for IMAT"):
            options = combo_items(self.cropping_view.combo_bank)
            self.assertNotIn("Texture20", options)
            self.assertNotIn("Texture30", options)

        with self.check("Run Processing / IMAT uses its own full instrument calibration"):
            from Engineering.common.instrument_config import get_instr_config

            config = get_instr_config(INSTRUMENT)
            self.assertIn("IMAT", config.full_instr_calib)

        with self.check("Run Processing / IMAT uses IkedaCarpenterPV with fixed peak parameters"):
            from Engineering.common.instrument_config import get_instr_config

            config = get_instr_config(INSTRUMENT)
            self.assertEqual("IkedaCarpenterPV", config.peak_func)
            self.assertIn("IkedaCarpenterPV", config.funcs_to_keep_fixed)

        with self.check("Run Processing / IMAT uses its own TOF binning"):
            from Engineering.common.instrument_config import get_instr_config

            self.assertNotEqual(get_instr_config("ENGINX").calibration_tof_binning, get_instr_config(INSTRUMENT).calibration_tof_binning)

        with self.check("Run Processing / switching back to ENGIN-X restores its texture groupings"):
            self.set_instrument("ENGINX")
            options = combo_items(self.cropping_view.combo_bank)
            # The exact labels matter: the combo is repopulated from the instrument config on every
            # instrument change, so if those labels drift from the ones the tab starts with, the
            # same option gets two different names within one session.
            self.assertIn("Texture20", options)
            self.assertIn("Texture30", options)


class EngDiffGuiImatCalibrateAndFocusTest(EngDiffGuiTestBase):
    """A real calibration and focus on IMAT, against fabricated run data."""

    def seeded_settings(self):
        settings = super(EngDiffGuiImatCalibrateAndFocusTest, self).seeded_settings()
        # Fit Gaussians, matching the shape the fixture generates. IMAT's real default is
        # IkedaCarpenterPV with its parameters pinned by the instrument definition, which is both
        # far slower to fit and impossible to satisfy with fabricated data - so this doubles as the
        # manual guide's "changing Default Peak Function" step, asserted from the log below.
        settings["default_peak_IMAT"] = "Gaussian"
        return settings

    def pre_gui_setup(self):
        self.data_dir = os.path.join(self.tmp_root, "imat_data")
        os.makedirs(self.data_dir, exist_ok=True)
        create_imat_ceria_and_vanadium(self.data_dir)
        # this is what lets the interface resolve the fabricated runs from a run number, exactly as
        # it would a real one, instead of the test bypassing the file finder
        self.add_data_search_dir(self.data_dir)

    def test_imat_calibrate_and_focus(self):
        self.show_tab(TAB_RUN_PROCESSING)
        self.set_instrument(INSTRUMENT)
        self.set_region_of_interest(None)

        with self.captured_logs(level="notice") as logs:
            calibration = self.calibrate(ceria=str(IMAT_CERIA_RUN), vanadium=str(IMAT_VANADIUM_RUN))

        # preconditions: nothing below is meaningful without a calibration
        self.assertIsNotNone(calibration, "no calibration was produced")
        self.assertTrue(calibration.is_valid(), "the calibration reported itself invalid")

        self._check_calibration_reported(logs)
        self._check_calibration_files()
        self._check_calibration_succeeded()
        self._check_focus()

    def _check_calibration_reported(self, logs):
        with self.check("Run Processing / the status bar reports the IMAT calibration"):
            self.assertEqual(
                f"CeO2: {IMAT_CERIA_RUN}, V: {IMAT_VANADIUM_RUN}, Instrument: {INSTRUMENT}",
                self.statusbar_text(),
            )

        with self.check("Run Processing / the peak function from the settings is the one used"):
            # the Default Peak Function setting is only observable through this log line
            self.assertIn("Gaussian", logs.text)
            # only IkedaCarpenterPV is in IMAT's funcs_to_keep_fixed, so a Gaussian fit is free
            self.assertIn("RespectFixedPeakParameters: False", logs.text)

        with self.check("Run Processing / the calibration records the peak shape it fitted"):
            self.assertEqual("Gaussian", self.calibration_presenter.current_calibration.get_fit_peak_shape())

    def _check_calibration_files(self):
        calibration_dir = self.calibration_dir()
        written = self.basenames_under(calibration_dir)
        with self.check("Run Processing / a prm and nxs are written for both banks and for all banks"):
            # calibration files are named INSTRUMENT_ceriaRun_suffix for every instrument; the
            # vanadium run appears in the *focused* output names, not these
            for suffix in ("all_banks", "bank_1", "bank_2"):
                for extension in (".prm", ".nxs"):
                    self.assertIn(f"{INSTRUMENT}_{IMAT_CERIA_RUN}_{suffix}{extension}", written)

        with self.check("Run Processing / the prm is built from the IMAT header template"):
            from Engineering.EnggUtils import CALIB_DIR

            prm = os.path.join(calibration_dir, f"{INSTRUMENT}_{IMAT_CERIA_RUN}_all_banks.prm")
            with open(prm) as written_prm:
                contents = written_prm.read()
            with open(os.path.join(CALIB_DIR, "template_IMAT_prm_header.prm")) as template:
                first_line = template.readline().strip()
            self.assertIn(first_line, contents)
            self.assertIn("ICONS", contents)

        with self.check("Run Processing / the written prm parses back into diffractometer constants"):
            from Engineering.EnggUtils import read_diff_constants_from_prm

            prm = os.path.join(calibration_dir, f"{INSTRUMENT}_{IMAT_CERIA_RUN}_all_banks.prm")
            constants = read_diff_constants_from_prm(prm)
            self.assertEqual(2, len(constants), f"expected one row per bank, got {constants}")
            for row in constants:
                for value in row:
                    self.assertFalse(math.isnan(value), "a diffractometer constant is NaN")

        # SOFT, for the same reason as the mask check: run_calibration keeps the uncalibrated DIFC
        # when PDCalibration fails rather than raising, so a zero here means the fabricated peak
        # shape did not fit that bank. That is a data-quality signal about this fixture, not a
        # regression in the interface, which is what the rest of the class is testing.
        with self.check("Run Processing / both IMAT banks got a fitted difc (data quality, soft)"):
            from Engineering.EnggUtils import read_diff_constants_from_prm

            prm = os.path.join(self.calibration_dir(), f"{INSTRUMENT}_{IMAT_CERIA_RUN}_all_banks.prm")
            unfitted = [index for index, row in enumerate(read_diff_constants_from_prm(prm)) if row[0] <= 0.0]
            self.assertEqual([], unfitted, f"PDCalibration produced no difc for bank(s) {unfitted}")

    def _check_calibration_succeeded(self):
        # SOFT on purpose. run_calibration only logs a warning when PDCalibration fails for a
        # spectrum and keeps the uncalibrated DIFC, so this is the only way to notice - but the
        # fabricated peak shape is an approximation of IMAT's real moderator pulse, so a failure
        # here is a data-quality signal rather than a reason to abandon the rest of the class.
        from mantid.api import AnalysisDataService as ADS

        mask_name = "engggui_calibration_all_banks_mask"
        with self.check("Run Processing / PDCalibration fitted the IMAT banks (data quality, soft)"):
            self.assertTrue(ADS.doesExist(mask_name), f"{mask_name} was not produced")
            mask = ADS.retrieve(mask_name)
            total = mask.getNumberHistograms()
            masked = sum(1 for index in range(total) if mask.readY(index)[0] != 0)
            # The mask spans the whole instrument while the fabricated run only populates a subset
            # of detectors, so most entries are legitimately masked. What matters is that the
            # focused banks were fitted at all, i.e. that some spectra came through unmasked.
            self.assertLess(masked, total, "PDCalibration failed for every spectrum")

    def _check_focus(self):
        self.focus(runs=str(IMAT_CERIA_RUN))

        with self.check("Run Processing / focusing produces an output workspace named for the run"):
            focused = self.focused_workspace_names()
            self.assertTrue(focused, "focusing produced no output workspace")
            for name in focused:
                self.assertTrue(name.startswith(str(IMAT_CERIA_RUN)), f"{name} is not named for the focused run")

        with self.check("Run Processing / the focused output files are written"):
            written = self.basenames_under(self.focus_dir())
            self.assertTrue(written, f"nothing under {self.focus_dir()}")
            self.assertTrue(any(name.endswith(".gss") for name in written), f"no gss in {written}")
            self.assertTrue(any(name.endswith(".nxs") for name in written), f"no nxs in {written}")
