# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Filter Events interface.

Replaces ``dev-docs/source/Testing/Utility/FilterEventsInterfaceTest.rst``, whose 21 numbered steps
are one continuous session. They are split here into three classes by what they need:

* ``FilterEventsGuiLoadingTest`` - steps 2-5 and 9, loading and the workspace list.
* ``FilterEventsGuiPlotAndMarkersTest`` - steps 6-8 and 10-11, the range markers and their line
  edits, which is the only genuinely interactive part of the interface.
* ``FilterEventsGuiFilteringTest`` - steps 12-21, filtering by log value and then by time, the Fast
  Log option, and the check that doing it by hand with the ``FilterEvents`` algorithm gives the same
  answer.

The colours the guide names ("dashed green", "dot-dashed blue") are not asserted: they are set once
where the markers are constructed and cannot change without the construction changing. What is
checked instead is that the right marker drives the right pair of line edits, which is the thing the
colours were there to help the tester identify.
"""

import unittest

from filter_events_gui_test_base import (
    DATA_WORKSPACE,
    LOG_VALUE_MAX,
    LOG_VALUE_MIN,
    SAMPLE_LOG,
    SUMMED_WORKSPACE,
    TOF_CORRECTION_WORKSPACE,
    FilterEventsGuiTestBase,
)
from qt_interaction_helpers import process_events, select_tab, set_checkbox, set_line_edit

# tab titles as eventFilterGUI.ui spells them. The guide calls the second one "Filtered by Time";
# the tab bar says "Filter By Time", and the tab bar is what a test has to match.
TAB_FILTER_BY_TIME = "Filter By Time"
TAB_ADVANCED_SETUP = "Advanced Setup"

FILTERED_TEMP = "FilteredTemp"
FILTERED_TIME = "FilteredTime"
FILTERED_REDUX = "FilteredTimeRedux"

TIME_FILTER_START, TIME_FILTER_STOP, TIME_INTERVAL = 80.0, 100.0, 10


class FilterEventsGuiLoadingTest(FilterEventsGuiTestBase):
    """Guide steps 2-5: browse, load, and the workspace drop-down."""

    def test_loading_a_run(self):
        self.load_data_file()

        with self.subTest("Step 4 (the loaded workspace appears in the ADS)"):
            from mantid.api import AnalysisDataService as ADS

            self.assertTrue(ADS.doesExist(DATA_WORKSPACE))

        with self.subTest("Step 4 (a _Summed_ workspace is generated alongside it)"):
            from mantid.api import AnalysisDataService as ADS

            self.assertTrue(ADS.doesExist(SUMMED_WORKSPACE))

        with self.subTest("Step 3 (the plot shows summed counts against time)"):
            x_values, y_values = self.plotted_data()
            self.assertGreater(len(x_values), 1, "the plot still holds its two placeholder points")
            self.assertEqual(len(x_values), len(y_values))
            self.assertEqual(("Time(s)", "Counts"), self.axis_labels())

        with self.subTest("Step 5 (Refresh lists the loaded workspace)"):
            self.assertIn(DATA_WORKSPACE, self.refresh_workspace_list())

        with self.subTest("Step 3 (loading populates the sample log drop-down)"):
            logs = [self.ui.comboBox_2.itemText(i) for i in range(self.ui.comboBox_2.count())]
            self.assertIn(SAMPLE_LOG, logs)

        with self.subTest("Step 3 (loading raises no error popup)"):
            self.assertEqual("", self.error_message_text())


class FilterEventsGuiPlotAndMarkersTest(FilterEventsGuiTestBase):
    """Guide steps 6-8 and 10-11: the range markers, their line edits, and the log plot."""

    def setUp(self):
        super(FilterEventsGuiPlotAndMarkersTest, self).setUp()
        self.load_data_file()

    def test_range_markers(self):
        self._check_time_marker_drag()
        self._check_value_marker_drag()
        self._check_line_edits_move_the_markers()
        self._check_crossed_range_is_refused()
        self._check_non_numeric_input_is_refused()

    def _check_time_marker_drag(self):
        """Step 6: dragging the vertical markers updates Starting Time and Stopping Time."""
        x_low, x_high = self.axes.get_xlim()
        target = x_low + 0.3 * (x_high - x_low)
        before = self.time_range_text()

        self.drag_marker(self.time_marker, "min", target)

        with self.subTest("Step 6 (dragging the vertical marker moves it)"):
            self.assertAlmostEqual(target, self.time_marker.get_minimum(), delta=0.05 * (x_high - x_low))

        with self.subTest("Step 6 (Starting Time follows the vertical marker)"):
            self.assertNotEqual(before[0], self.ui.leStartTime.text())
            self.assertAlmostEqual(self.time_marker.get_minimum(), float(self.ui.leStartTime.text()), places=3)

        with self.subTest("Step 6 (dragging one edge leaves the other alone)"):
            self.assertEqual(before[1], self.ui.leStopTime.text())

    def _check_value_marker_drag(self):
        """Step 7: dragging the horizontal markers updates Minimum Value and Maximum Value."""
        y_low, y_high = self.axes.get_ylim()
        target = y_low + 0.7 * (y_high - y_low)
        before = self.value_range_text()

        self.drag_marker(self.value_marker, "max", target)

        with self.subTest("Step 7 (dragging the horizontal marker moves it)"):
            self.assertAlmostEqual(target, self.value_marker.get_maximum(), delta=0.05 * (y_high - y_low))

        with self.subTest("Step 7 (Maximum Value follows the horizontal marker)"):
            self.assertNotEqual(before[1], self.ui.leMaximumValue.text())
            self.assertAlmostEqual(self.value_marker.get_maximum(), float(self.ui.leMaximumValue.text()), places=3)

    def _check_line_edits_move_the_markers(self):
        """Step 8, first bullet: typing a value and pressing Enter moves the marker."""
        x_low, x_high = self.axes.get_xlim()
        target = x_low + 0.4 * (x_high - x_low)

        set_line_edit(self.ui.leStartTime, f"{target:.6f}")

        with self.subTest("Step 8 (typing into Starting Time moves the vertical marker)"):
            self.assertAlmostEqual(target, self.time_marker.get_minimum(), places=3)

    def _check_crossed_range_is_refused(self):
        """Step 8, second bullet: 'Setting a value on Starting Time larger than the current value on
        Stopping Time is not allowed, and vice versa.'

        Both halves are checked from a known state rather than from whatever the drags above left
        behind, so that a failure reports one comparison rather than an arithmetic puzzle.

        The first half currently fails, and deliberately so - this suite replicates the manual guide
        rather than working around it. ``update_line_edits`` is what sets each field's validator
        range from the other field's value, and it runs when a marker is dragged or the plot is
        redrawn, but *not* when the paired field is typed into. So after the Stopping Time is typed
        down to 20, the Starting Time's validator still allows anything up to the old maximum and
        accepts 50. See FUTURE_WORK.md.
        """
        x_low, x_high = self.axes.get_xlim()
        span = x_high - x_low

        # put the marker back over most of the range first. The checks above left it wherever they
        # dragged it to, and each field's validator range comes from the *other* edge, so without
        # this the "stop" below could be refused for being outside the marker rather than for the
        # reason under test.
        self.set_marker_range(self.time_marker, x_low + 0.05 * span, x_low + 0.90 * span)

        stop = f"{x_low + 0.15 * span:.6f}"
        set_line_edit(self.ui.leStopTime, stop, expect_valid=False)
        self.assertEqual(stop, self.ui.leStopTime.text(), "the Stopping Time could not be moved into range")

        set_line_edit(self.ui.leStartTime, f"{x_low + 0.50 * span:.6f}", expect_valid=False)

        with self.subTest("Step 8 (a Starting Time past the Stopping Time is not allowed)"):
            self.assertLessEqual(
                float(self.ui.leStartTime.text()),
                float(self.ui.leStopTime.text()),
                "the Starting Time was accepted beyond the Stopping Time",
            )

        # the marker swapping its two edges round is the same defect seen from the other side, so it
        # is described in FUTURE_WORK.md rather than reported as a second failed observation

        before = self.ui.leStopTime.text()
        set_line_edit(self.ui.leStopTime, f"{x_low + 0.01 * span:.6f}", expect_valid=False)

        with self.subTest("Step 8 (and a Stopping Time before the Starting Time likewise)"):
            self.assertEqual(before, self.ui.leStopTime.text(), "the Stopping Time was accepted before the Starting Time")

    def _check_non_numeric_input_is_refused(self):
        """Step 8, third bullet: non-numeric characters cannot be entered."""
        from qtpy.QtCore import Qt
        from qtpy.QtTest import QTest

        for field, label in ((self.ui.leStartTime, "Starting Time"), (self.ui.leMinimumValue, "Minimum Value")):
            field.setFocus()
            field.selectAll()
            QTest.keyClick(field, Qt.Key_Backspace)
            QTest.keyClicks(field, "abc")
            process_events()
            with self.subTest(f"Step 8 ({label} refuses non-numeric characters)"):
                self.assertEqual("", field.text())

    def test_plotting_a_sample_log(self):
        """Guide steps 10-11: plotting the sample log and placing the markers around it."""
        counts_x, counts_y = self.plotted_data()

        self.plot_sample_log(SAMPLE_LOG)

        with self.subTest("Step 11 (the plot updates with the log against time)"):
            log_x, log_y = self.plotted_data()
            self.assertNotEqual(list(counts_y), list(log_y), "the plot still shows the summed counts")
            self.assertEqual(len(log_x), len(log_y))

        with self.subTest("Step 11 (the y axis is labelled for the log)"):
            self.assertIn(SAMPLE_LOG, self.axes.get_ylabel())

        with self.subTest("Step 11 (the log's statistics are revealed)"):
            self.assertTrue(self.ui.label_lognamevalue.isVisible())
            self.assertEqual(SAMPLE_LOG, self.ui.label_lognamevalue.text())
            self.assertTrue(self.ui.label_meanvalue.text())

        self.set_marker_range(self.value_marker, LOG_VALUE_MIN, LOG_VALUE_MAX)

        with self.subTest("Step 12 (the value markers can be placed around the log range)"):
            minimum, maximum = self.value_range_text()
            self.assertAlmostEqual(LOG_VALUE_MIN, float(minimum), places=3)
            self.assertAlmostEqual(LOG_VALUE_MAX, float(maximum), places=3)


class FilterEventsGuiFilteringTest(FilterEventsGuiTestBase):
    """Guide steps 12-21: filtering by log value, then by time, Fast Log, and the algorithm check."""

    def setUp(self):
        super(FilterEventsGuiFilteringTest, self).setUp()
        self.load_data_file()

    def test_filtering(self):
        self._check_filter_by_log_value()
        self._check_filtered_workspace_is_reusable()
        self._check_filter_by_time()
        self._check_fast_log()
        self._check_algorithm_gives_the_same_answer()
        self._check_final_refresh()

    def _check_filter_by_log_value(self):
        """Steps 10-14: filter on SampleTemp between the markers."""
        from mantid.api import AnalysisDataService as ADS

        self.plot_sample_log(SAMPLE_LOG)
        self.set_marker_range(self.value_marker, LOG_VALUE_MIN, LOG_VALUE_MAX)
        self.filter_by_log_value(FILTERED_TEMP)

        splitters, info = self.splitter_names(DATA_WORKSPACE)

        with self.subTest("Step 13 (a splitters and an info table are produced)"):
            self.assertTrue(ADS.doesExist(splitters), f"{splitters} was not created")
            self.assertTrue(ADS.doesExist(info), f"{info} was not created")

        with self.subTest("Step 13 (the group holds a single filtered workspace)"):
            self.assertTrue(ADS.doesExist(FILTERED_TEMP), f"{FILTERED_TEMP} was not created")
            self.assertEqual([f"{FILTERED_TEMP}_0"], self.group_members(FILTERED_TEMP))

        with self.subTest("Step 13 (a TOF correction table is produced)"):
            self.assertTrue(ADS.doesExist(TOF_CORRECTION_WORKSPACE))

        with self.subTest("Step 14 (the filtered run's log lies within the selected range)"):
            # the guide has the tester open the Sample Logs window and eyeball this; the same
            # statement is made here about the log itself
            filtered = ADS.retrieve(f"{FILTERED_TEMP}_0")
            values = filtered.getRun().getProperty(SAMPLE_LOG).value
            self.assertGreaterEqual(min(values), LOG_VALUE_MIN - 0.01)
            self.assertLessEqual(max(values), LOG_VALUE_MAX + 0.01)

    def _check_filtered_workspace_is_reusable(self):
        """Steps 15-16: Refresh lists the filtered run, and Use loads it into the interface."""
        with self.subTest("Step 15 (Refresh lists both the original and the filtered run)"):
            listed = self.refresh_workspace_list()
            self.assertIn(DATA_WORKSPACE, listed)
            self.assertIn(f"{FILTERED_TEMP}_0", listed)

        before_x, before_y = self.plotted_data()
        self.use_workspace(f"{FILTERED_TEMP}_0")

        with self.subTest("Step 16 (Use switches the interface to the filtered run)"):
            self.assertEqual(f"{FILTERED_TEMP}_0", str(self.gui._dataWS))

        with self.subTest("Step 16 (and the plot updates)"):
            after_x, after_y = self.plotted_data()
            self.assertNotEqual((list(before_x), list(before_y)), (list(after_x), list(after_y)))

    def _check_filter_by_time(self):
        """Steps 17-19: filter the filtered run into 10 second slices."""
        from mantid.api import AnalysisDataService as ADS

        set_line_edit(self.ui.leStartTime, f"{TIME_FILTER_START:.6f}", expect_valid=False)
        set_line_edit(self.ui.leStopTime, f"{TIME_FILTER_STOP:.6f}", expect_valid=False)
        select_tab(self.ui.filterTab, TAB_FILTER_BY_TIME)
        self.filter_by_time(FILTERED_TIME, time_interval=TIME_INTERVAL)

        with self.subTest("Step 18 (the group holds two filtered workspaces)"):
            self.assertTrue(ADS.doesExist(FILTERED_TIME), f"{FILTERED_TIME} was not created")
            self.assertEqual([f"{FILTERED_TIME}_0", f"{FILTERED_TIME}_1"], self.group_members(FILTERED_TIME))

        splitters, info = self.splitter_names(f"{FILTERED_TEMP}_0")

        with self.subTest("Step 18 (the splitters are named after the run being filtered)"):
            self.assertTrue(ADS.doesExist(splitters), f"{splitters} was not created")
            self.assertTrue(ADS.doesExist(info), f"{info} was not created")

        with self.subTest("Step 19 (the info table has one row per time interval)"):
            table = ADS.retrieve(info)
            self.assertEqual(2, table.rowCount())

        with self.subTest("Step 19 (each interval is about ten seconds long)"):
            # from the splitters rather than from the filtered runs: FilterEvents does not crop a
            # slice's run start and end times, so those still describe the whole run. The splitter
            # table is where the intervals themselves are. Its columns are in nanoseconds whatever
            # UnitOfTime the interface asked for, so they are converted here rather than compared
            # against a number nobody would recognise as ten seconds.
            table = ADS.retrieve(splitters)
            for row in range(table.rowCount()):
                interval = (float(table.cell("stop", row)) - float(table.cell("start", row))) / 1e9
                self.assertAlmostEqual(TIME_INTERVAL, interval, delta=1.0)

        with self.subTest("Step 19 (each interval names its workspace group index)"):
            table = ADS.retrieve(info)
            self.assertEqual([0, 1], sorted(int(table.cell("workspacegroup", row)) for row in range(table.rowCount())))

    def _check_fast_log(self):
        """Step 20: with Fast Log ticked the splitters come back as a matrix workspace."""
        from mantid.api import AnalysisDataService as ADS
        from mantid.api import ITableWorkspace

        splitters, _info = self.splitter_names(f"{FILTERED_TEMP}_0")

        with self.subTest("Step 20 (without Fast Log the splitters are a table workspace)"):
            self.assertIsInstance(ADS.retrieve(splitters), ITableWorkspace)

        select_tab(self.ui.filterTab, TAB_ADVANCED_SETUP)
        set_checkbox(self.ui.checkBox_fastLog, True)
        select_tab(self.ui.filterTab, TAB_FILTER_BY_TIME)
        self.filter_by_time(FILTERED_TIME, time_interval=TIME_INTERVAL)

        with self.subTest("Step 20 (with Fast Log the splitters are a matrix workspace)"):
            self.assertNotIsInstance(ADS.retrieve(splitters), ITableWorkspace)

        with self.subTest("Step 20 (everything else is unchanged)"):
            self.assertEqual([f"{FILTERED_TIME}_0", f"{FILTERED_TIME}_1"], self.group_members(FILTERED_TIME))

        select_tab(self.ui.filterTab, TAB_ADVANCED_SETUP)
        set_checkbox(self.ui.checkBox_fastLog, False)
        select_tab(self.ui.filterTab, TAB_FILTER_BY_TIME)

    def _check_algorithm_gives_the_same_answer(self):
        """Step 21: running FilterEvents by hand must reproduce what the interface produced.

        The guide has the tester drive the generated algorithm dialog; here the algorithm is called
        directly with the same inputs, because what the step is really checking is that the
        interface passes on the arguments a user would have typed - not that the dialog works.
        """
        from mantid.api import AnalysisDataService as ADS
        from mantid.simpleapi import CompareWorkspaces, FilterEvents

        # the interface last filtered with Fast Log on and then off again, so regenerate the
        # splitters it would be using now
        self.filter_by_time(FILTERED_TIME, time_interval=TIME_INTERVAL)
        splitters, info = self.splitter_names(f"{FILTERED_TEMP}_0")

        FilterEvents(
            InputWorkspace=f"{FILTERED_TEMP}_0",
            SplitterWorkspace=ADS.retrieve(splitters),
            InformationWorkspace=ADS.retrieve(info),
            OutputWorkspaceBaseName=FILTERED_REDUX,
            GroupWorkspaces=True,
            OutputTOFCorrectionWorkspace=TOF_CORRECTION_WORKSPACE,
        )

        with self.subTest("Step 21 (the algorithm produces a matching group)"):
            self.assertTrue(ADS.doesExist(FILTERED_REDUX))
            self.assertEqual(2, len(self.group_members(FILTERED_REDUX)))

        for index, name in enumerate(self.group_members(FILTERED_TIME)):
            with self.subTest(f"Step 21 ({name} matches the workspace the algorithm produced)"):
                by_hand = self.group_members(FILTERED_REDUX)[index]
                match, _messages = CompareWorkspaces(Workspace1=name, Workspace2=by_hand, CheckSample=False)
                self.assertTrue(match, f"{name} and {by_hand} differ")

    def _check_final_refresh(self):
        """Step 22: Refresh lists every event workspace now in the ADS."""
        listed = self.refresh_workspace_list()
        expected = [DATA_WORKSPACE, f"{FILTERED_TEMP}_0", f"{FILTERED_TIME}_0", f"{FILTERED_TIME}_1"]
        for name in expected:
            with self.subTest(f"Step 22 (Refresh lists {name})"):
                self.assertIn(name, listed)


if __name__ == "__main__":
    unittest.main()
