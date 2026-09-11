# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Shared setup for the Muon Analysis automated UI tests.

These replace the guides under ``dev-docs/source/Testing/MuonAnalysis_test_guides/``.

**Data.** Every guide but one works from ISIS archive runs (MUSR 62260, EMU 20889-20900,
HIFI 134028-39, ARGUS, and the ~100 runs the ALC guide needs), which the weekly runners do not have.
Those suites declare their runs by name and skip cleanly without them - the observations in them are
fitted parameters and guessed alphas, so fabricating data would make the numbers meaningless. The
exception is the PSI guide, whose ``deltat_tdc_dolly_1529.bin`` is in the unit test data, so that
suite runs everywhere and is the one that actually exercises the interface on the weekly run.

Muon Analysis refuses to load anything unless the facility is one it supports, so the facility is
set for the duration of each test through ``config_settings`` and restored afterwards - it is
process-wide configuration, not a Qt setting, so ``_isolate_qsettings`` does not cover it.
"""

import os
import sys

_PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PARENT_DIR not in sys.path:
    sys.path.insert(0, _PARENT_DIR)

from automated_ui_test_base import AutomatedUITestBase  # noqa: E402
from qt_interaction_helpers import process_events, select_combo  # noqa: E402

# tabs, as MuonAnalysisGui.setup_tabs adds them
TAB_HOME = "Home"
TAB_GROUPING = "Grouping"
TAB_CORRECTIONS = "Corrections"
TAB_PHASE_TABLE = "Phase Table"
TAB_FITTING = "Fitting"
TAB_SEQUENTIAL_FITTING = "Sequential Fitting"
TAB_RESULTS = "Results"

# the PSI run the PSI guide loads, and the four groups it produces
PSI_FILE = "deltat_tdc_dolly_1529.bin"
PSI_GROUPS = ("Forw", "Back", "Left", "Rite")


class MuonGuiTestBase(AutomatedUITestBase):
    """Builds Muon Analysis and exposes its tabs, its loader and its contexts.

    Subclasses set ``FACILITY`` and ``INSTRUMENT`` and override ``required_files``.
    """

    FACILITY = "ISIS"
    INSTRUMENT = "MUSR"

    def required_files(self):
        return ()

    def setUp(self):
        super(MuonGuiTestBase, self).setUp()
        self.require_files(*self.required_files())
        self.gui = None

        # the facility has to be right *before* the interface is built: MuonAnalysisGui checks it in
        # its constructor and refuses to start on an unsupported one
        config = self.config_settings(default_facility=self.FACILITY, default_instrument=self.INSTRUMENT)
        config.__enter__()
        self.addCleanup(config.__exit__, None, None, None)

        # Import the interface before patching, so that the sweep below has every Muon module to
        # sweep: they are pulled in by this import, not before it. Imported here rather than at
        # module scope so a build without the interface skips cleanly.
        from mantidqtinterfaces.Muon.GUI.MuonAnalysis.muon_analysis_2 import MuonAnalysisGui

        # must happen before anything is loaded: the load path raises a modal warning for a run
        # whose instrument definition is missing, and an unattended test then hangs on it
        self.patch_muon_message_boxes()

        self._build_gui(MuonAnalysisGui)

    def patch_muon_message_boxes(self):
        """Stop Muon Analysis's modal popups from blocking, and record what they would have said.

        ``Muon.GUI.Common.message_box.warning`` ends in ``QMessageBox.warning``, which blocks until a
        user presses OK. Patching it where it is defined is not enough: around twenty Muon modules do
        ``from ... message_box import warning``, so each holds its own reference and a patch of the
        definition intercepts none of them. The imported modules are therefore swept and every such
        reference replaced - which is the same reasoning as ``patch_error_messages``, done by
        discovery because the list is long and changes.

        ``question`` is patched to answer "yes", which is what a tester following the guide does.
        """
        from unittest import mock

        from mantidqtinterfaces.Muon.GUI.Common import message_box

        def record_warning(error, parent=None):
            self.message_box_messages.append(str(error))

        def record_question(question, parent=None):
            self.message_box_messages.append(str(question))
            return True

        targets = [(message_box, "warning", record_warning), (message_box, "question", record_question)]
        for name, module in list(sys.modules.items()):
            if not name.startswith("mantidqtinterfaces.Muon") or module is None:
                continue
            for attribute, replacement in (("warning", record_warning), ("question", record_question)):
                if getattr(module, attribute, None) is getattr(message_box, attribute):
                    targets.append((module, attribute, replacement))

        for module, attribute, replacement in targets:
            patcher = mock.patch.object(module, attribute, side_effect=replacement)
            patcher.start()
            self.addCleanup(patcher.stop)

    def _build_gui(self, MuonAnalysisGui):
        self.gui = MuonAnalysisGui()
        self.gui.show()
        process_events(3)

        self.context = self.gui.context
        self.tabs = self.gui.tabs
        self.home_tab = self.gui.home_tab
        self.grouping_tab = self.gui.grouping_tab_widget
        self.corrections_tab = self.gui.corrections_tab
        self.fitting_tab = self.gui.fitting_tab
        self.load_widget = self.gui.load_widget

    def tearDown(self):
        gui = getattr(self, "gui", None)
        if gui is not None:
            self._drain_async_tasks()
            # the context holds an ADS observer that repopulates the tabs on a clear, so the clear
            # has to happen while the interface is still there to receive it
            self._clear_ads()
            process_events(2)
            gui.close()
            process_events(2)
            self.gui = None
        super(MuonGuiTestBase, self).tearDown()

    # ------------------------------------------------------------------ tabs

    def show_tab(self, title):
        """Make one of the interface's tabs current, by its title."""
        for index in range(self.tabs.count()):
            if self.tabs.tabText(index) == title:
                self.tabs.setCurrentIndex(index)
                process_events(2)
                return self.tabs.widget(index)
        raise AssertionError(f"no tab titled '{title}'; found {[self.tabs.tabText(i) for i in range(self.tabs.count())]}")

    # ------------------------------------------------------------------ loading

    @property
    def instrument_view(self):
        """The instrument section of the Home tab, which owns the instrument, time zero and rebin
        controls the guides drive."""
        return self.home_tab.inst_view

    def set_instrument(self, instrument):
        """Choose the instrument on the Home tab, as the guides' first step does.

        The combo may already be showing the wanted instrument - it is initialised from
        ``default.instrument`` - and ``setCurrentIndex`` to the index already current emits nothing,
        so the presenter is never told and the *context* keeps whatever instrument it started with.
        Selecting something else first guarantees the change signal that carries the choice through
        to the context, which is what the loader then checks the data against.
        """
        self.show_tab(TAB_HOME)
        selector = self.instrument_view.instrument_selector
        if selector.currentText() == instrument:
            others = [selector.itemText(i) for i in range(selector.count()) if selector.itemText(i) != instrument]
            if others:
                select_combo(selector, others[0])
                process_events(2)
        select_combo(selector, instrument)
        process_events(3)
        if self.context.data_context.instrument != instrument:
            raise AssertionError(
                f"the interface is still on instrument '{self.context.data_context.instrument}' after selecting '{instrument}'"
            )

    def set_fixed_rebin(self, steps):
        """Set Rebin to 'Fixed' with the given number of steps, as the PSI guide's step 5 does."""
        self.show_tab(TAB_HOME)
        select_combo(self.instrument_view.rebin_selector, "Fixed")
        process_events(2)
        self.instrument_view.rebin_steps_edit.setText(str(steps))
        self.instrument_view.rebin_steps_edit.editingFinished.emit()
        process_events(3)

    def load_file(self, filename):
        """Load a data file through the interface's Browse path, without its background thread.

        ``handle_load_no_threading`` is the loader's own synchronous entry point, and it is used
        rather than the threaded one so the test does not have to wait on a worker that reports back
        through a blocking queued connection. Everything it calls - the model, the context update,
        the tabs repopulating - is the same either way.

        The load is repeated once if the interface asks for it. Loading a run from an instrument
        other than the one currently selected does not load it: the interface switches the Home tab
        to that instrument and says "Please reload your data". A tester following the guide reloads
        at that point, so the test does too, rather than reporting a failure the guide does not
        describe.
        """
        from mantid.api import FileFinder

        path = FileFinder.getFullPath(filename)
        if not path:
            raise AssertionError(f"{filename} could not be found on the data search path")

        for _attempt in range(2):
            self.load_widget.file_widget.handle_load_no_threading([path])
            process_events(5)
            if self.loaded_run_names():
                return path
            if not any("reload your data" in message for message in self.message_box_messages):
                break
        if not self.loaded_run_names():
            raise AssertionError(f"{filename} did not load: {self.message_box_messages}")
        return path

    def loaded_run_names(self):
        return list(self.context.data_context.current_runs)

    # ------------------------------------------------------------------ groups and plots

    def group_names(self):
        return list(self.context.group_pair_context.group_names)

    def plotted_curve_labels(self):
        """Labels of every curve currently drawn in the plotting dock.

        The guides' plot observations are nearly all counts of lines ("you will get 4 lines", "each
        subplot will now contain just 2 lines"), so the labels are what those are counted from.
        """
        labels = []
        for figure in self._figures():
            for axes in figure.axes:
                labels.extend(line.get_label() for line in axes.get_lines())
        return labels

    def _figures(self):
        plot_widget = self.gui.plot_widget
        figures = []
        for pane in getattr(plot_widget, "get_plot_panes", lambda: [])():
            canvas = getattr(pane.view, "fig", None)
            if canvas is not None:
                figures.append(canvas)
        return figures
