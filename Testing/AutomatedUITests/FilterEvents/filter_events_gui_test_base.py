# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the Filter Events automated UI tests.

These replace ``dev-docs/source/Testing/Utility/FilterEventsInterfaceTest.rst``.

The interface is a single Qt Designer window rather than an MVP triple, so unlike most suites here
there is no presenter to reach through - everything is on ``gui.ui``, and the widget names below are
the ones in ``eventFilterGUI.ui``.

Two things have to be neutralised before the first click:

* ``MainWindow._setErrorMsg`` builds a ``QMessageBox`` and calls ``exec()`` on it, which blocks until
  a user presses OK. Every load failure goes through it, so an unattended run that mistypes a file
  name hangs rather than fails.
* the range markers are matplotlib artists on an embedded canvas, so they are driven with
  ``mpl_drag`` rather than ``QTest`` - see ``qt_interaction_helpers``.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import click, mpl_drag, process_events, select_combo, set_line_edit  # noqa: E402

GUI_MODULE = "mantidqtinterfaces.FilterEvents.eventFilterGUI"

# The guide says "CNCS_7860_Event.nxs from the Usage Data set". The file in the repository's data
# store is spelled with a lower case "event" - the same run, reached through the SystemTest data
# directory the harness puts on the search path. The name is spelled here as the store has it,
# because a capitalised name resolves on Windows and not on Linux.
DATA_FILE = "CNCS_7860_event.nxs"
DATA_WORKSPACE = "CNCS_7860_event"
SUMMED_WORKSPACE = f"_Summed_{DATA_WORKSPACE}"

# the log the guide filters on, and the markers it puts either side of it
SAMPLE_LOG = "SampleTemp"
LOG_VALUE_MIN, LOG_VALUE_MAX = 279.91, 279.95

TOF_CORRECTION_WORKSPACE = "TOFCorrTable"


class FilterEventsGuiTestBase(AutomatedUITestBase):
    """Builds the Filter Events interface and drives its load, plot and filter buttons."""

    def required_files(self):
        return (DATA_FILE,)

    def setUp(self):
        super(FilterEventsGuiTestBase, self).setUp()
        self.require_files(*self.required_files())
        self.gui = None
        # the error popup is modal and would hang an unattended run; the mock also records what it
        # would have said, which is how a rejected input is asserted on
        self.error_box = self.patch_confirmation_box(GUI_MODULE, answer=True)
        self._build_gui()

    def _build_gui(self):
        # imported here rather than at module scope so a build without the interface skips cleanly
        from mantidqtinterfaces.FilterEvents.eventFilterGUI import MainWindow

        self.gui = MainWindow()
        self.gui.show()
        process_events(2)
        self.ui = self.gui.ui
        self.axes = self.ui.mainplot
        # markers[0] is the vertical (time) pair, markers[1] the horizontal (log value) pair; the
        # order is fixed by MainWindow.__init__ and the line edits are paired with them there
        self.time_marker, self.value_marker = self.gui.markers

    def tearDown(self):
        gui = getattr(self, "gui", None)
        if gui is not None:
            self._drain_async_tasks()
            self._clear_ads()
            process_events(2)
            gui.close()
            process_events(2)
            self.gui = None
        super(FilterEventsGuiTestBase, self).tearDown()

    # ------------------------------------------------------------------ error popup

    def error_message_text(self):
        """What the (patched) error box was last told to display, or "" if it never appeared.

        ``_setErrorMsg`` constructs the box and calls ``setText`` on it, so the recorded call on the
        mock's instance is where the message ends up rather than in ``message_box_messages``.
        """
        set_text = self.error_box.return_value.setText
        if not set_text.call_args_list:
            return ""
        return str(set_text.call_args_list[-1].args[0])

    # ------------------------------------------------------------------ loading

    def load_data_file(self):
        """Type the file name in and press Load, as the guide's steps 2-3 do.

        The path is used rather than the bare name so the load does not depend on the default
        instrument, which ``_loadFile`` consults for anything that looks like a run number.
        """
        from mantid.api import FileFinder

        set_line_edit(self.ui.lineEdit, FileFinder.getFullPath(DATA_FILE))
        click(self.ui.pushButton_load)
        process_events(3)
        if self.gui._dataWS is None:
            raise RuntimeError(f"the interface did not load {DATA_FILE}: {self.error_message_text()}")
        return self.gui._dataWS

    def refresh_workspace_list(self):
        click(self.ui.pushButton_refreshWS)
        process_events(2)
        return [self.ui.comboBox.itemText(i) for i in range(self.ui.comboBox.count())]

    def use_workspace(self, name):
        """Select an event workspace from the drop-down and press Use."""
        self.refresh_workspace_list()
        select_combo(self.ui.comboBox, name)
        click(self.ui.pushButton_3)
        process_events(3)

    # ------------------------------------------------------------------ plotting

    def plot_sample_log(self, log_name=SAMPLE_LOG):
        select_combo(self.ui.comboBox_2, log_name)
        click(self.ui.pushButton_4)
        process_events(3)

    def plotted_data(self):
        """The single line the interface draws, as ``(x, y)``.

        ``update_plot`` reuses one line for everything - the summed counts, then the sample log - so
        "the plot updated" is checked as its data changing rather than as a new artist appearing.
        """
        return self.gui.mainline[0].get_xdata(), self.gui.mainline[0].get_ydata()

    def axis_labels(self):
        return self.axes.get_xlabel(), self.axes.get_ylabel()

    # ------------------------------------------------------------------ markers

    def drag_marker(self, marker, edge, to_value):
        """Drag one edge of a range marker to a new position, in data coordinates.

        ``edge`` is "min" or "max". The cross-axis coordinate is the middle of the other axis, which
        is inside the plot for both marker kinds - the vertical marker spans the full height and the
        horizontal one the full width, so any point on the line will do.
        """
        x_low, x_high = self.axes.get_xlim()
        y_low, y_high = self.axes.get_ylim()
        current = marker.get_minimum() if edge == "min" else marker.get_maximum()

        if marker is self.time_marker:
            start, end = (current, 0.5 * (y_low + y_high)), (to_value, 0.5 * (y_low + y_high))
        else:
            start, end = (0.5 * (x_low + x_high), current), (0.5 * (x_low + x_high), to_value)
        mpl_drag(self.axes, start, end)
        process_events(2)
        return marker.get_range()

    def set_marker_range(self, marker, minimum, maximum):
        """Put a marker where the guide wants it, without dragging.

        Used where the guide's instruction is "move the marker to approximately 279.95" - the value
        is what matters to the step that follows, and the dragging itself is checked once, on its
        own, in ``FilterEventsGuiPlotAndMarkersTest``.
        """
        marker.set_range(minimum, maximum)
        min_edit, max_edit = self.gui.marker_line_edits[self.gui.markers.index(marker)]
        self.gui.update_line_edits(min_edit, max_edit, self.axes.get_xlim() if marker is self.time_marker else self.axes.get_ylim(), marker)
        process_events(2)

    def time_range_text(self):
        return self.ui.leStartTime.text(), self.ui.leStopTime.text()

    def value_range_text(self):
        return self.ui.leMinimumValue.text(), self.ui.leMaximumValue.text()

    # ------------------------------------------------------------------ filtering

    def filter_by_log_value(self, output_name):
        set_line_edit(self.ui.lineEdit_outwsname, output_name)
        click(self.ui.pushButton_filterLog)
        process_events(3)

    def filter_by_time(self, output_name, time_interval=None):
        set_line_edit(self.ui.lineEdit_outwsname, output_name)
        if time_interval is not None:
            # returnPressed on this field also triggers the filter, so setting it with a typed Enter
            # would run the filter twice; assign it and let the button do the work
            self.ui.lineEdit_timeInterval.setText(str(time_interval))
            process_events()
        click(self.ui.pushButton_filterTime)
        process_events(3)

    def splitter_names(self, data_workspace):
        """The two workspaces ``GenerateEventsFilter`` writes, named after the loaded workspace."""
        return f"{data_workspace}_splitters", f"{data_workspace}_info"

    @staticmethod
    def group_members(name):
        from mantid.api import AnalysisDataService as ADS

        return sorted(ADS.retrieve(name).getNames())
