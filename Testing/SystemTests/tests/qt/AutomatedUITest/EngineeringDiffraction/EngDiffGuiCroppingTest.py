# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""System tests for the calibration region of interest on ENGIN-X.

Split out from ``EngDiffGuiRunProcessingTest`` because a class is the unit of both skipping and
runtime, and the region-of-interest cases are the most expensive part of the suite: each one is a
separate real ``PDCalibration``.

Three classes, in increasing cost:

* ``EngDiffGuiRoiOptionsTest`` drives the cropping widget without calibrating at all, so it covers
  every option and every widget interaction cheaply and can gate pull requests.
* ``EngDiffGuiCroppedCalibrationTest`` calibrates and focuses for a single bank and for a custom
  spectrum range, which is where the file naming actually differs.
* ``EngDiffGuiTextureRoiTest`` calibrates and focuses a texture grouping, which additionally has its
  own save layout and its own rule about the RB number.

The ceria and vanadium runs are fabricated - see ``create_synthetic_ceria_and_vanadium`` and the
module docstring of ``EngDiffGuiRunProcessingTest`` for why.
"""

import os

from eng_diff_gui_test_base import (
    ENGINX_SYNTHETIC_CERIA_RUN,
    ENGINX_SYNTHETIC_VANADIUM_RUN,
    EngDiffGuiTestBase,
    TAB_RUN_PROCESSING,
    create_enginx_ceria_and_vanadium,
)
from qt_interaction_helpers import combo_items, process_events, select_combo

INSTRUMENT = "ENGINX"
CERIA = str(ENGINX_SYNTHETIC_CERIA_RUN)
VANADIUM = str(ENGINX_SYNTHETIC_VANADIUM_RUN)

# the region of interest options ENGIN-X offers, in the order the combo lists them
EXPECTED_ROI_OPTIONS = (
    "Custom Grouping File",
    "1 (North)",
    "2 (South)",
    "Crop to Spectra",
    "Texture20",
    "Texture30",
)

CUSTOM_SPECTRA = "1200-2400"
TEXTURE_GROUPS = 20


class _CroppingTestBase(EngDiffGuiTestBase):
    """Stages the fabricated ENGIN-X runs and points the interface's file finder at them."""

    def requiredMemoryMB(self):
        return 4000

    def seeded_settings(self):
        settings = super(_CroppingTestBase, self).seeded_settings()
        # the fixture generates Gaussian peaks, so the fit has to look for the same shape
        settings["default_peak_ENGINX"] = "Gaussian"
        return settings

    def pre_gui_setup(self):
        self.data_dir = os.path.join(self.tmp_root, "enginx_data")
        os.makedirs(self.data_dir, exist_ok=True)
        create_enginx_ceria_and_vanadium(self.data_dir)
        self.add_data_search_dir(self.data_dir)

    def custom_grouping_file(self):
        """A grouping file to feed the 'Custom Grouping File' option.

        The shipped North bank grouping is used rather than a fabricated one: it is a real file of
        the right form, and the point of the option is that an arbitrary file is accepted, not what
        is in it. Its trailing name part is what ends up in the output file names.
        """
        from Engineering.EnggUtils import CALIB_DIR

        return os.path.join(CALIB_DIR, "ENGINX_North_grouping.xml")

    def focused_basename(self, suffix, xunit):
        return f"{INSTRUMENT}_{CERIA}_{VANADIUM}_{suffix}_{xunit}"


class EngDiffGuiRoiOptionsTest(_CroppingTestBase):
    """Every region of interest option, driven through the widget but stopping short of calibrating.

    Cheap enough to run everywhere - it needs no run data - so it is the regression check that the
    combo, the conditional inputs and the grouping each option produces all stay correct.
    """

    def excludeInPullRequests(self):
        return False

    def pre_gui_setup(self):
        # no calibration is run here, so the fabricated runs are not needed
        pass

    def _run_checks(self):
        self.show_tab(TAB_RUN_PROCESSING)
        self._check_combo_and_visibility()
        self._check_selected_groups()
        self._check_spectra_validation()
        self._check_grouping_workspaces()
        self._check_output_file_names()

    def _check_combo_and_visibility(self):
        view = self.run_processing_view

        with self.check("Cropping / the region of interest inputs are hidden until it is ticked"):
            self.assertFalse(self.cropping_view.isVisible())

        self.set_region_of_interest("1 (North)")

        with self.check("Cropping / ticking the box reveals the region of interest widget"):
            self.assertTrue(self.cropping_view.isVisible())

        with self.check("Cropping / ENGIN-X offers exactly its own grouping options"):
            self.assertEqual(list(EXPECTED_ROI_OPTIONS), combo_items(self.cropping_view.combo_bank))

        with self.check("Cropping / a bank option needs no extra input"):
            self.assertFalse(self.cropping_view.widget_custom.isVisible())
            self.assertFalse(self.cropping_view.widget_crop.isVisible())

        select_combo(self.cropping_view.combo_bank, "Custom Grouping File")
        with self.check("Cropping / 'Custom Grouping File' reveals the file finder only"):
            self.assertTrue(self.cropping_view.widget_custom.isVisible())
            self.assertFalse(self.cropping_view.widget_crop.isVisible())

        select_combo(self.cropping_view.combo_bank, "Crop to Spectra")
        with self.check("Cropping / 'Crop to Spectra' reveals the spectrum number field only"):
            self.assertTrue(self.cropping_view.widget_crop.isVisible())
            self.assertFalse(self.cropping_view.widget_custom.isVisible())

        select_combo(self.cropping_view.combo_bank, "Texture20")
        with self.check("Cropping / a texture option needs no extra input either"):
            self.assertFalse(self.cropping_view.widget_custom.isVisible())
            self.assertFalse(self.cropping_view.widget_crop.isVisible())

        # unticking must put the widget away again, otherwise the next calibration would silently
        # keep using whatever was last selected
        self.set_region_of_interest(None)
        with self.check("Cropping / unticking the box hides the region of interest widget"):
            self.assertFalse(self.cropping_view.isVisible())
            self.assertFalse(view.get_crop_checked())

    def _check_selected_groups(self):
        from Engineering.common.instrument_config import ENGINX_GROUP

        expected = {
            "Custom Grouping File": ENGINX_GROUP.CUSTOM,
            "1 (North)": ENGINX_GROUP.NORTH,
            "2 (South)": ENGINX_GROUP.SOUTH,
            "Crop to Spectra": ENGINX_GROUP.CROPPED,
            "Texture20": ENGINX_GROUP.TEXTURE20,
            "Texture30": ENGINX_GROUP.TEXTURE30,
        }
        cropping = self.calibration_presenter.cropping_widget
        for description, group in expected.items():
            self.set_region_of_interest(description)
            with self.check(f"Cropping / selecting '{description}' selects {group}"):
                self.assertEqual(group, cropping.get_group())

    def _check_spectra_validation(self):
        cropping = self.calibration_presenter.cropping_widget
        self.set_region_of_interest("Crop to Spectra")

        with self.check("Cropping / a valid spectrum range is accepted and cleaned up"):
            self.cropping_view.edit_crop.setText(CUSTOM_SPECTRA)
            process_events()
            self.assertTrue(cropping.is_spectra_valid())
            self.assertEqual(CUSTOM_SPECTRA, cropping.get_custom_spectra())
            self.assertFalse(self.cropping_view.label_cropValid.isVisible())

        with self.check("Cropping / an invalid spectrum range is rejected and flagged"):
            self.cropping_view.edit_crop.setText("not a spectrum list")
            process_events()
            self.assertFalse(cropping.is_spectra_valid())
            self.assertIsNone(cropping.get_custom_spectra())
            self.assertTrue(self.cropping_view.label_cropValid.isVisible())
            # the reason is offered as a tooltip rather than a popup, so nothing blocks
            self.assertTrue(self.cropping_view.label_cropValid.toolTip())

        with self.check("Cropping / correcting the spectrum range clears the warning"):
            self.cropping_view.edit_crop.setText(CUSTOM_SPECTRA)
            process_events()
            self.assertTrue(cropping.is_spectra_valid())
            self.assertFalse(self.cropping_view.label_cropValid.isVisible())

        with self.check("Cropping / a custom grouping file is resolved and reported valid"):
            self.set_region_of_interest("Custom Grouping File", custom_grouping_file=self.custom_grouping_file())
            self.assertTrue(cropping.is_groupingfile_valid())
            self.assertEqual(
                os.path.normcase(self.custom_grouping_file()),
                os.path.normcase(cropping.get_custom_groupingfile()),
            )

    def _check_grouping_workspaces(self):
        """Each option must produce a grouping workspace with the right number of groups.

        This is the part of the calibration that the region of interest actually decides, so it can
        be checked without running PDCalibration at all.
        """
        from Engineering.common.calibration_info import CalibrationInfo
        from Engineering.common.instrument_config import ENGINX_GROUP

        expected_groups = {
            ENGINX_GROUP.BOTH: 2,
            ENGINX_GROUP.NORTH: 1,
            ENGINX_GROUP.SOUTH: 1,
            ENGINX_GROUP.TEXTURE20: TEXTURE_GROUPS,
            ENGINX_GROUP.TEXTURE30: 30,
        }
        for group, count in expected_groups.items():
            with self.check(f"Cropping / {group} produces {count} focused group(s)"):
                calibration = CalibrationInfo(group=group, instrument=INSTRUMENT)
                calibration.set_calibration_paths(INSTRUMENT, CERIA, VANADIUM)
                calibration.update_group_ws_from_group()
                grouping = calibration.get_group_ws()
                self.assertEqual(count, len(set(grouping.extractY().flatten())) - (0 in grouping.extractY()))

    def _check_output_file_names(self):
        """The region of interest is what names the calibration output, so check each form."""
        from Engineering.common.calibration_info import CalibrationInfo
        from Engineering.common.instrument_config import ENGINX_GROUP

        cases = {
            ENGINX_GROUP.BOTH: f"{INSTRUMENT}_{CERIA}_all_banks.prm",
            ENGINX_GROUP.NORTH: f"{INSTRUMENT}_{CERIA}_bank_1.prm",
            ENGINX_GROUP.SOUTH: f"{INSTRUMENT}_{CERIA}_bank_2.prm",
            ENGINX_GROUP.TEXTURE20: f"{INSTRUMENT}_{CERIA}_Texture20.prm",
            ENGINX_GROUP.TEXTURE30: f"{INSTRUMENT}_{CERIA}_Texture30.prm",
        }
        for group, expected in cases.items():
            with self.check(f"Cropping / {group} names its output '{expected}'"):
                calibration = CalibrationInfo(group=group, instrument=INSTRUMENT)
                calibration.set_calibration_paths(INSTRUMENT, CERIA, VANADIUM)
                self.assertEqual(expected, calibration.generate_output_file_name())

        with self.check("Cropping / a cropped calibration carries the spectrum range in its name"):
            calibration = CalibrationInfo(group=ENGINX_GROUP.CROPPED, instrument=INSTRUMENT)
            calibration.set_calibration_paths(INSTRUMENT, CERIA, VANADIUM)
            calibration.set_spectra_list(CUSTOM_SPECTRA)
            calibration.set_extra_group_suffix()
            self.assertEqual(f"{INSTRUMENT}_{CERIA}_Cropped_{CUSTOM_SPECTRA}.prm", calibration.generate_output_file_name())


class EngDiffGuiCroppedCalibrationTest(_CroppingTestBase):
    """A real calibrate and focus for a single bank and for a custom spectrum range."""

    def excludeInPullRequests(self):
        return True

    def _run_checks(self):
        self._check_single_bank()
        self._check_cropped_to_spectra()

    def _check_single_bank(self):
        from Engineering.common.instrument_config import ENGINX_GROUP
        from mantid.api import AnalysisDataService as ADS

        self.set_region_of_interest("1 (North)")
        calibration = self.calibrate(ceria=CERIA, vanadium=VANADIUM)
        self.assertTrue(calibration.is_valid(), "the North bank calibration reported itself invalid")

        with self.check("Cropping / a North bank calibration records that group"):
            self.assertEqual(ENGINX_GROUP.NORTH, calibration.get_group())

        with self.check("Cropping / a North bank calibration writes exactly one prm and one nxs"):
            written = self.basenames_under(self.calibration_dir())
            self.assertEqual(
                sorted([f"{INSTRUMENT}_{CERIA}_bank_1.prm", f"{INSTRUMENT}_{CERIA}_bank_1.nxs"]),
                sorted(written),
                "a cropped calibration must not write the other banks",
            )

        self.focus(runs=CERIA)
        with self.check("Cropping / focusing a single bank gives a single spectrum"):
            focused = self.focused_workspace_names()
            self.assertEqual(1, len(focused), f"expected one focused workspace, got {focused}")
            self.assertEqual(1, ADS.retrieve(focused[0]).getNumberHistograms())

        with self.check("Cropping / the focused files are named for the bank"):
            written = self.basenames_under(self.focus_dir())
            self.assertIn(self.focused_basename("bank_1", "TOF") + ".nxs", written)
            self.assertIn(self.focused_basename("bank_1", "TOF") + ".gss", written)

    def _check_cropped_to_spectra(self):
        from Engineering.common.instrument_config import ENGINX_GROUP
        from mantid.api import AnalysisDataService as ADS

        self.set_region_of_interest("Crop to Spectra", custom_spectra=CUSTOM_SPECTRA)
        calibration = self.calibrate(ceria=CERIA, vanadium=VANADIUM)
        self.assertTrue(calibration.is_valid(), "the cropped calibration reported itself invalid")

        with self.check("Cropping / a cropped calibration records the group and the spectrum range"):
            self.assertEqual(ENGINX_GROUP.CROPPED, calibration.get_group())
            self.assertEqual(CUSTOM_SPECTRA, calibration.spectra_list_str)

        expected_stem = f"{INSTRUMENT}_{CERIA}_Cropped_{CUSTOM_SPECTRA}"
        written = self.basenames_under(self.calibration_dir())

        with self.check("Cropping / a cropped calibration names its output for the spectrum range"):
            self.assertIn(expected_stem + ".prm", written)
            self.assertIn(expected_stem + ".nxs", written)

        with self.check("Cropping / the grouping used is saved alongside, so the crop is reproducible"):
            # only custom and cropped groupings are saved - the bank and texture ones ship with the
            # instrument, so there would be nothing to record
            self.assertIn(expected_stem + ".xml", written)

        self.focus(runs=CERIA)
        with self.check("Cropping / focusing a cropped calibration gives a single spectrum"):
            from Engineering.EnggUtils import FOCUSED_OUTPUT_WORKSPACE_NAME

            # named for the region of interest, so it sits alongside the North bank output from the
            # first half of this test rather than replacing it
            expected_ws = f"{CERIA}_{FOCUSED_OUTPUT_WORKSPACE_NAME}Cropped_{CUSTOM_SPECTRA}"
            self.assertIn(expected_ws, self.focused_workspace_names())
            self.assertEqual(1, ADS.retrieve(expected_ws).getNumberHistograms())

        with self.check("Cropping / the focused files carry the cropped suffix too"):
            focused_written = self.basenames_under(self.focus_dir())
            self.assertIn(self.focused_basename(f"Cropped_{CUSTOM_SPECTRA}", "TOF") + ".nxs", focused_written)


class EngDiffGuiTextureRoiTest(_CroppingTestBase):
    """A real calibrate and focus for a texture grouping, and its RB number rule.

    Texture groupings save differently from every other option: the focused output goes into its own
    subdirectory, and once an RB number is set the output goes *only* to the RB directory, to keep
    the number of files down.
    """

    RB_NUMBER = "7654321"

    def excludeInPullRequests(self):
        return True

    def _run_checks(self):
        from Engineering.common.instrument_config import ENGINX_GROUP
        from mantid.api import AnalysisDataService as ADS

        self.set_region_of_interest("Texture20")
        calibration = self.calibrate(ceria=CERIA, vanadium=VANADIUM)
        self.assertTrue(calibration.is_valid(), "the texture calibration reported itself invalid")

        with self.check("Cropping / a texture calibration records the texture group"):
            self.assertEqual(ENGINX_GROUP.TEXTURE20, calibration.get_group())
            self.assertTrue(calibration.is_texture_group())

        with self.check("Cropping / a texture calibration writes one prm and nxs named for the grouping"):
            written = self.basenames_under(self.calibration_dir())
            self.assertIn(f"{INSTRUMENT}_{CERIA}_Texture20.prm", written)
            self.assertIn(f"{INSTRUMENT}_{CERIA}_Texture20.nxs", written)

        with self.check("Cropping / the texture prm describes every group in the grouping"):
            from Engineering.EnggUtils import read_diff_constants_from_prm

            prm = os.path.join(self.calibration_dir(), f"{INSTRUMENT}_{CERIA}_Texture20.prm")
            self.assertEqual(TEXTURE_GROUPS, len(read_diff_constants_from_prm(prm)))

        self.focus(runs=CERIA)

        with self.check("Cropping / focusing a texture calibration gives one spectrum per group"):
            focused = self.focused_workspace_names()
            self.assertEqual(1, len(focused), f"expected one focused workspace, got {focused}")
            self.assertEqual(TEXTURE_GROUPS, ADS.retrieve(focused[0]).getNumberHistograms())

        texture_focus_dir = os.path.join(self.focus_dir(), "Texture20")
        with self.check("Cropping / texture focused output goes into its own subdirectory"):
            written = self.basenames_under(texture_focus_dir)
            self.assertIn(self.focused_basename("Texture20", "TOF") + ".gss", written)
            for group in (1, TEXTURE_GROUPS):
                self.assertIn(self.focused_basename(f"Texture20_{group}", "TOF") + ".nxs", written)

        with self.check("Cropping / a nexus file is written for every texture group"):
            written = self.basenames_under(texture_focus_dir, ".nxs")
            per_group = [name for name in written if "_Texture20_" in name and "_TOF" in name]
            self.assertEqual(TEXTURE_GROUPS, len(per_group))

        self._check_rb_number_rule(texture_focus_dir)

    def _check_rb_number_rule(self, texture_focus_dir):
        before = set(self.files_under(texture_focus_dir))

        self.set_rb_number(self.RB_NUMBER)
        self.focus(runs=CERIA)
        process_events(2)

        with self.check("Cropping / with an RB number texture output goes to the RB directory"):
            rb_dir = os.path.join(self.focus_dir(self.RB_NUMBER), "Texture20")
            self.assertIn(self.focused_basename("Texture20", "TOF") + ".gss", self.basenames_under(rb_dir))

        with self.check("Cropping / and *only* there, to limit the number of files saved"):
            # unlike a non-texture grouping, which is written to both places
            self.assertEqual(before, set(self.files_under(texture_focus_dir)))
