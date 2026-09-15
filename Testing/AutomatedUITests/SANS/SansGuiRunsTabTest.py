# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the ISIS SANS Runs tab.

Replaces scenarios 1, 2, 3 and 13 of ``dev-docs/source/Testing/SANSGUI/ISISSANSGUITests.rst``: the
automatic save selection, editing and exporting the runs table, reloading a user file, and the
tooltips.

These are the parts of that guide whose observations are about the interface rather than about a
particular reduction, so they need no run data and run everywhere. The reduction scenarios (5-8),
whose observations are the guide's exact workspace and file counts, are in
``SansGuiReductionTest.py`` and skip without the Training data.

Scenario 13 is worth reading before changing: the guide says of the tooltips that "users rely on
them a lot and really do notice each missing one", and it names one known gap (the Wavelength
section) that must *not* be reported as a problem.
"""

import csv
import os
import unittest

from sans_gui_test_base import SansGuiTestBase
from qt_interaction_helpers import process_events, set_checkbox


class SansGuiSaveSelectionTest(SansGuiTestBase):
    """Guide scenario 1: 'Automatic Save Selection'."""

    def test_automatic_save_selection(self):
        self._check_defaults_per_dimensionality()
        self._check_manual_choice_reverts()
        self._check_memory_disables_everything()
        self._check_can_sas_stays_disabled_in_2d()

    def _check_defaults_per_dimensionality(self):
        """With File or Both selected, 1D ticks CanSAS and NxCanSAS; 2D ticks only NxCanSAS."""
        for mode in ("File", "Both"):
            self.set_output_mode(mode)

            self.set_reduction_dimensionality("1D")
            with self.subTest(f"Scenario 1 / {mode} + 1D ticks CanSAS (1D) and NxCanSAS (1D/2D)"):
                checked = {name for name, (_enabled, is_checked) in self.save_checkbox_states().items() if is_checked}
                self.assertEqual({"CanSAS (1D)", "NxCanSAS (1D/2D)"}, checked)

            self.set_reduction_dimensionality("2D")
            with self.subTest(f"Scenario 1 / {mode} + 2D ticks only NxCanSAS (1D/2D)"):
                checked = {name for name, (_enabled, is_checked) in self.save_checkbox_states().items() if is_checked}
                self.assertEqual({"NxCanSAS (1D/2D)"}, checked)

    def _check_manual_choice_reverts(self):
        """Ticking RKH and then changing the reduction mode puts the defaults back."""
        self.set_output_mode("File")
        self.set_reduction_dimensionality("1D")
        set_checkbox(self.save_checkboxes()["RKH (1D/2D)"], True)
        process_events(2)

        self.set_reduction_dimensionality("2D")

        with self.subTest("Scenario 1 / changing the reduction mode reverts to the defaults"):
            checked = {name for name, (_enabled, is_checked) in self.save_checkbox_states().items() if is_checked}
            self.assertEqual({"NxCanSAS (1D/2D)"}, checked, "RKH (1D/2D) survived the change of reduction mode")

    def _check_memory_disables_everything(self):
        """Memory output has nothing to save, so all three boxes are disabled."""
        self.set_output_mode("Memory")

        with self.subTest("Scenario 1 / Memory disables all three save formats"):
            for name, (enabled, _checked) in self.save_checkbox_states().items():
                self.assertFalse(enabled, f"{name} should be disabled when saving to memory")

    def _check_can_sas_stays_disabled_in_2d(self):
        """'Swap between Memory and File with a 2D reduction mode. CanSAS (1D) should always stay
        disabled.'"""
        self.set_reduction_dimensionality("2D")
        for mode in ("Memory", "File", "Memory", "File"):
            self.set_output_mode(mode)
            with self.subTest(f"Scenario 1 / CanSAS (1D) stays disabled in 2D with output mode {mode}"):
                self.assertFalse(self.save_checkboxes()["CanSAS (1D)"].isEnabled())


class SansGuiRunsTableTest(SansGuiTestBase):
    """Guide scenario 2: editing the runs table and exporting it."""

    def setUp(self):
        super(SansGuiRunsTableTest, self).setUp()
        self.load_user_file()
        self.load_batch_file()

    def test_batch_file_populates_the_table(self):
        with self.subTest("Scenario 2 / loading a batch file fills the runs table"):
            self.assertGreater(self.table_row_count(), 0, "the batch file produced no rows")

        with self.subTest("Scenario 2 / the first row carries the sample scatter run"):
            self.assertTrue(self.gui.get_cell(0, 0), "the Sample Scatter cell is empty")

    def test_row_insertion_and_deletion(self):
        """The guide's Insert, Delete and Erase icons."""
        before = self.table_row_count()

        self.presenter.on_insert_row()
        process_events(2)

        with self.subTest("Scenario 2 / Insert adds a row"):
            self.assertEqual(before + 1, self.table_row_count())

        self.gui.clear_table()
        self.presenter.on_rows_removed([before])
        process_events(2)

        self.load_batch_file()

        with self.subTest("Scenario 2 / reloading the batch file restores the table"):
            self.assertEqual(before, self.table_row_count())

    def test_sample_geometry_reveals_extra_columns(self):
        """'Ticking Sample Geometry - some extra columns should appear.'"""
        set_checkbox(self.gui.sample_geometry_checkbox, False)
        process_events(2)
        without = set(self.visible_columns())

        set_checkbox(self.gui.sample_geometry_checkbox, True)
        process_events(2)
        with_geometry = set(self.visible_columns())

        with self.subTest("Scenario 2 / ticking Sample Geometry reveals extra columns"):
            self.assertTrue(with_geometry > without, "no extra columns appeared")

        with self.subTest("Scenario 2 / and they are the sample shape and size columns"):
            self.assertEqual({"Sample Shape", "Sample Height", "Sample Width"}, with_geometry - without)

        set_checkbox(self.gui.sample_geometry_checkbox, False)
        process_events(2)

        with self.subTest("Scenario 2 / unticking it hides them again"):
            self.assertEqual(without, set(self.visible_columns()))

    def test_multi_period_reveals_six_more_columns(self):
        """Scenario 6's tail: 'Multi-period - six additional columns should appear.'"""
        self.gui.hide_period_columns()
        process_events(2)
        without = set(self.visible_columns())

        self.gui.show_period_columns()
        process_events(2)
        with_periods = set(self.visible_columns())

        with self.subTest("Scenario 6 / Multi-period reveals six additional columns"):
            self.assertEqual(6, len(with_periods - without))

        with self.subTest("Scenario 6 / and they are the period columns"):
            self.assertEqual(set(self.gui.MULTI_PERIOD_COLUMNS), with_periods - without)

    def test_exported_table_round_trips(self):
        """'Export Table' writes a key,value CSV that reloads into the same columns.

        The guide's checks are that every column except Options and Sample Shape is written, that
        the set written does not depend on which columns are currently *shown*, and that reloading
        puts the values back in the right columns.
        """
        exported = os.path.join(self.output_dir, "exported_table.csv")

        self.gui.show_period_columns()
        set_checkbox(self.gui.sample_geometry_checkbox, True)
        process_events(2)
        self.export_table_to(exported)

        with self.subTest("Scenario 2 / Export Table writes a file"):
            self.assertTrue(os.path.isfile(exported), "no file was exported")

        with self.subTest("Scenario 2 / it is written as key,value pairs"):
            with open(exported, newline="") as handle:
                fields = next(csv.reader(handle))
            self.assertEqual(0, len(fields) % 2, f"an odd number of fields: {fields}")

        with self.subTest("Scenario 2 / Options and Sample Shape are not exported"):
            with open(exported, newline="") as handle:
                keys = set(next(csv.reader(handle))[::2])
            self.assertNotIn("user_file", keys - keys)  # placeholder for the shape below
            for excluded in ("options_column_model", "sample_shape"):
                self.assertNotIn(excluded, keys)

        # the same table with the extra columns hidden must export the same keys
        self.gui.hide_period_columns()
        set_checkbox(self.gui.sample_geometry_checkbox, False)
        process_events(2)
        hidden_export = os.path.join(self.output_dir, "exported_table_hidden.csv")
        self.export_table_to(hidden_export)

        with self.subTest("Scenario 2 / the exported columns do not depend on which are displayed"):
            with open(exported, newline="") as handle:
                shown_keys = next(csv.reader(handle))[::2]
            with open(hidden_export, newline="") as handle:
                hidden_keys = next(csv.reader(handle))[::2]
            self.assertEqual(shown_keys, hidden_keys)

        self.gui.clear_table()
        self.gui.batch_line_edit.setText(exported)
        self.presenter.on_batch_file_load()
        process_events(3)

        with self.subTest("Scenario 2 / the exported table can be loaded back in"):
            self.assertEqual(1, self.table_row_count())


class SansGuiUserFileTest(SansGuiTestBase):
    """Guide scenario 3: reloading a user file resets what it owns and nothing else."""

    def setUp(self):
        super(SansGuiUserFileTest, self).setUp()
        self.load_user_file()

    def test_reloading_a_user_file_reverts_its_settings(self):
        # wavelength_min is a property on the view rather than a widget, so it is set through the
        # property; that is also the path the user file's own loading takes
        original = self.gui.wavelength_min
        changed = 5.5 if original != 5.5 else 6.5
        self.gui.wavelength_min = changed
        process_events(2)
        self.assertEqual(changed, self.gui.wavelength_min, "the wavelength minimum could not be changed")

        self.load_user_file()

        with self.subTest("Scenario 3 / reloading the user file reverts a changed setting"):
            self.assertEqual(original, self.gui.wavelength_min)

    def test_beam_centre_options_do_not_revert(self):
        """'The inputs in the Centre Position section should revert ... the inputs in the Options
        section (such as the radius limits) should not revert.'"""
        beam_centre = self.presenter._beam_centre_presenter._view
        beam_centre.r_min = 1234.0
        process_events(2)

        self.load_user_file()

        with self.subTest("Scenario 3 / the beam centre Options section does not revert"):
            self.assertEqual(1234.0, beam_centre.r_min)


class SansGuiTooltipTest(SansGuiTestBase):
    """Guide scenario 13: the table, process and load buttons all carry tooltips."""

    # the buttons the guide points at by name
    BUTTONS = (
        "process_selected_button",
        "process_all_button",
        "load_button",
        "export_table_button",
        "insert_row_button",
        "delete_row_button",
        "copy_button",
        "cut_button",
        "paste_button",
        "erase_button",
        "save_other_pushButton",
    )

    def test_buttons_have_tooltips(self):
        for name in self.BUTTONS:
            button = getattr(self.gui, name, None)
            with self.subTest(f"Scenario 13 / {name} has a tooltip"):
                self.assertIsNotNone(button, f"the interface has no {name}")
                self.assertTrue(button.toolTip(), f"{name} has no tooltip")

    def test_named_settings_widgets_have_tooltips(self):
        """The guide singles out Zero Error Free and Use Optimizations."""
        for name in ("save_zero_error_free", "use_optimizations_checkbox"):
            widget = getattr(self.gui, name, None)
            with self.subTest(f"Scenario 13 / {name} has a tooltip"):
                self.assertIsNotNone(widget, f"the interface has no {name}")
                self.assertTrue(widget.toolTip(), f"{name} has no tooltip")


if __name__ == "__main__":
    unittest.main()
