# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for Muon Analysis on PSI data.

Replaces ``dev-docs/source/Testing/MuonAnalysis_test_guides/Muon_Analysis_PSI.rst``.

This is the only Muon guide whose data is in the repository (``deltat_tdc_dolly_1529.bin``, in the
unit test data), so it is the only one that actually runs on the weekly job rather than skipping.
The other Muon guides are in ``MuonGuiArchiveTest.py``.

The guide's later sections ask for judgements about how the curves look - "you will get 4 lines that
all curve upwards", "the plots will now average around 0", "the plots that you changed will drift
away from zero at large times". Where a number is given (the four background values) it is checked;
the shape descriptions are replaced by the nearest numeric statement and recorded in FUTURE_WORK.md.
"""

import unittest

from muon_gui_test_base import PSI_FILE, PSI_GROUPS, TAB_CORRECTIONS, MuonGuiTestBase
from qt_interaction_helpers import process_events, select_combo

# the guide's expected auto background values, in the order it lists them
EXPECTED_BACKGROUNDS = {"Forw": 19.2, "Back": 93.6, "Left": 104.4, "Rite": 77.2}

# The guide gives the four values to one decimal place, so they are checked to within a couple of
# counts rather than exactly - pinning them tighter than the guide states would fail on a change to
# the fit that the manual test would have passed.
BACKGROUND_TOLERANCE = 2.0


class MuonGuiPsiLoadingTest(MuonGuiTestBase):
    """Guide section 'Loading Data Test'."""

    FACILITY = "SmuS"
    INSTRUMENT = "PSI"

    def required_files(self):
        return (PSI_FILE,)

    def load_psi_run(self):
        """Attempt the guide's load, reporting whether it worked rather than raising."""
        try:
            self.load_file(PSI_FILE)
        except AssertionError:
            return False
        return bool(self.loaded_run_names())

    def test_loading_psi_data(self):
        self.set_instrument("PSI")

        with self.subTest("Loading / the load current run button is greyed out for PSI"):
            # PSI is not an ISIS instrument, so there is no current run to load
            self.assertFalse(self.load_widget.load_run_view.load_current_run_button.isVisible())

        with self.subTest("Loading / the PSI run loads"):
            # This currently fails. Mantid ships no instrument definition for PSI, so the workspace
            # LoadPSIMuonBin produces does not carry the instrument the interface is set to, and the
            # loader rejects it with "This data is for a different instrument and has not been
            # loaded". Reloading, which is what its message asks for, does not help: the mismatch is
            # in the file rather than in the selection. See FUTURE_WORK.md.
            self.assertTrue(self.load_psi_run(), f"{PSI_FILE} did not load: {self.message_box_messages}")

        if not self.loaded_run_names():
            # nothing below this point can say anything without the run
            return

        with self.subTest("Loading / the run produces the four PSI groups"):
            self.assertEqual(sorted(PSI_GROUPS), sorted(self.group_names()))

        with self.subTest("Loading / a curve is plotted for each group"):
            # the guide's "you will get 4 lines"
            self.assertEqual(len(PSI_GROUPS), len(self.plotted_curve_labels()))

        self.set_fixed_rebin(5)

        with self.subTest("Loading / setting a fixed rebin of 5 is accepted"):
            self.assertEqual("Fixed", self.instrument_view.rebin_selector.currentText())
            self.assertEqual("5", self.instrument_view.rebin_steps_edit.text())

        with self.subTest("Loading / the plot still shows one curve per group after rebinning"):
            process_events(3)
            self.assertEqual(len(PSI_GROUPS), len(self.plotted_curve_labels()))

        # The guide then ticks "Plot raw" and says "the data will change". That box lives on the
        # plotting dock rather than on any tab, and the observation is a shape judgement about the
        # curves; it is recorded in FUTURE_WORK.md.


class MuonGuiPsiAutoBackgroundTest(MuonGuiTestBase):
    """Guide section 'Auto Background Corrections Test'.

    The four background values are the only hard numbers in any of the Muon guides that can be
    checked without the archive, which makes this the most valuable Muon test in the suite.
    """

    FACILITY = "SmuS"
    INSTRUMENT = "PSI"

    def required_files(self):
        return (PSI_FILE,)

    def setUp(self):
        super(MuonGuiPsiAutoBackgroundTest, self).setUp()
        self.set_instrument("PSI")
        try:
            self.load_file(PSI_FILE)
        except AssertionError as error:
            # The load failure itself is reported by MuonGuiPsiLoadingTest, as the guide's own first
            # observation. Repeating it here as three more failures would bury it rather than
            # emphasise it, so these skip and point at the test that reports it.
            self.skipTest(f"the PSI run does not load, so the corrections cannot be exercised - see MuonGuiPsiLoadingTest ({error})")
        self.set_fixed_rebin(5)
        self.show_tab(TAB_CORRECTIONS)

    def test_auto_background_corrections(self):
        self.select_background_mode("Auto")

        backgrounds = self.background_values()

        with self.subTest("Auto background / a background is fitted for every group"):
            self.assertEqual(sorted(EXPECTED_BACKGROUNDS), sorted(backgrounds))

        for group, expected in EXPECTED_BACKGROUNDS.items():
            with self.subTest(f"Auto background / {group} is about {expected}"):
                self.assertAlmostEqual(expected, backgrounds[group], delta=BACKGROUND_TOLERANCE)

        # The guide goes on to tick "use Raw" and check the values move by less than 1. That box is
        # a per-row cell widget in the corrections table rather than a control on the tab, so it is
        # recorded in FUTURE_WORK.md rather than driven here.

    def test_flat_background_reports_an_error(self):
        """Guide section 'Flat Background Corrections Test', first half.

        Its later steps have the tester read the peak position off the plot by eye and type an End X
        "just before the peak"; that is recorded in FUTURE_WORK.md. What is checked here is the
        first, unambiguous observation: with the default range the flat background fit fails and the
        interface says so, on the tab as well as in the table.
        """
        self.select_background_mode("Auto")
        self.select_background_function("Flat Background")
        process_events(5)

        with self.subTest("Flat background / the tab reports the failure"):
            statuses = self.background_statuses()
            self.assertTrue(statuses, "the corrections table reported no status at all")
            self.assertTrue(
                any(status.strip() and status.strip().lower() != "success" for status in statuses.values()),
                f"the flat background fit reported no error: {statuses}",
            )

    # ------------------------------------------------------------------ corrections tab

    @property
    def background_view(self):
        """The background corrections section of the Corrections tab."""
        return self.corrections_tab.corrections_tab_view.background_corrections_view

    def select_background_mode(self, mode):
        """Choose None, Auto or Manual for the background correction."""
        select_combo(self.background_view.mode_combo_box, mode)
        process_events(5)

    def select_background_function(self, function):
        select_combo(self.background_view.function_combo_box, function)
        process_events(5)

    def _background_table(self):
        return self.background_view.correction_options_table

    def background_values(self):
        """The fitted background for each group, keyed by group name."""
        table = self._background_table()
        values = {}
        for row in range(table.rowCount()):
            group = table.item(row, self._column("Group")).text()
            values[group] = float(table.item(row, self._column("Background")).text())
        return values

    def background_statuses(self):
        table = self._background_table()
        column = self._column("Status")
        return {table.item(row, self._column("Group")).text(): table.item(row, column).text() for row in range(table.rowCount())}

    def _column(self, heading):
        table = self._background_table()
        for col in range(table.columnCount()):
            item = table.horizontalHeaderItem(col)
            if item is not None and heading.lower() in item.text().lower():
                return col
        headings = [table.horizontalHeaderItem(c).text() if table.horizontalHeaderItem(c) else "" for c in range(table.columnCount())]
        raise AssertionError(f"no '{heading}' column in the background corrections table; found {headings}")


if __name__ == "__main__":
    unittest.main()
