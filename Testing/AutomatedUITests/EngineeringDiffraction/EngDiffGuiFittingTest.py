# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Fitting tab.

Nothing here is mocked. The tab is reached the way a user reaches it - by calibrating and focusing
on the Run Processing tab first - so the notifier that prefills the fitting file finder is exercised
rather than assumed, and the workspaces being fitted are the ones focusing actually produced.

The fits are real ``Fit`` calls against the real ``EngDiffFitPropertyBrowser``. Because the
fabricated runs carry Gaussian peaks (see ``create_synthetic_ceria_and_vanadium``), the model fitted
here is a Gaussian on a linear background, positioned from the calibration rather than hard coded, so
the fit has a well conditioned answer and the assertions can be about parameter values rather than
just "something ran".

Two sample runs are fabricated and focused, purely so there are two runs to fit: that is what makes
the serial/sequential distinction and the run ordering observable at all.
"""

import os

from eng_diff_gui_test_base import (
    ENGINX_SYNTHETIC_CERIA_RUN,
    ENGINX_SYNTHETIC_VANADIUM_RUN,
    EngDiffGuiTestBase,
    TAB_FITTING,
    TAB_RUN_PROCESSING,
    create_enginx_ceria_and_vanadium,
)
from qt_interaction_helpers import (
    assert_curve_matches_workspace,
    click,
    curve_by_label,
    curves,
    figure_numbers,
    new_figures,
    plot_labels,
    process_events,
    set_checkbox,
    set_finder_text,
    wait_until,
)

INSTRUMENT = "ENGINX"
CERIA = str(ENGINX_SYNTHETIC_CERIA_RUN)
VANADIUM = str(ENGINX_SYNTHETIC_VANADIUM_RUN)

# a second sample run carrying the same peaks as the ceria one, so there are two runs to fit. The
# vanadium is not usable for this: it is deliberately featureless, so a peak fitted to it lands
# somewhere arbitrary.
SECOND_SAMPLE = str(ENGINX_SYNTHETIC_CERIA_RUN + 1)

# table_selection columns, as built in FittingDataView.add_table_row
COL_RUN, COL_BANK, COL_PLOT, COL_BGSUB, COL_NITER, COL_XWINDOW, COL_SG = range(7)

# a ceria d-spacing well inside the ENGIN-X calibration window and clear of its neighbours, used to
# place the fit range; the actual TOF is derived from the calibration at runtime
FIT_PEAK_D = 2.7059


class _FittingTestBase(EngDiffGuiTestBase):
    """Calibrates and focuses so the fitting tab has something real to work with."""

    def seeded_settings(self):
        settings = super(_FittingTestBase, self).seeded_settings()
        # the fixture generates Gaussian peaks, so both the calibration and the fit look for those
        settings["default_peak_ENGINX"] = "Gaussian"
        return settings

    def pre_gui_setup(self):
        self.data_dir = os.path.join(self.tmp_root, "enginx_data")
        os.makedirs(self.data_dir, exist_ok=True)
        create_enginx_ceria_and_vanadium(self.data_dir, extra_sample_runs=(int(SECOND_SAMPLE),))
        self.add_data_search_dir(self.data_dir)

    # ------------------------------------------------------------------ setup

    def calibrate_and_focus(self):
        """Produce the focused data the fitting tab consumes.

        The North bank only: one spectrum per run keeps both the focus and every subsequent fit
        cheap, and nothing in this tab depends on the number of banks.
        """
        self.show_tab(TAB_RUN_PROCESSING)
        self.set_region_of_interest("1 (North)")
        calibration = self.calibrate(ceria=CERIA, vanadium=VANADIUM)
        self.assertTrue(calibration.is_valid(), "the calibration reported itself invalid")
        # both sample runs are focused so there are two to fit
        self.focus(runs=f"{CERIA}, {SECOND_SAMPLE}")
        return calibration

    # ------------------------------------------------------------------ the tab

    @property
    def data_view(self):
        return self.fitting_presenter.data_widget.view

    @property
    def data_presenter(self):
        return self.fitting_presenter.data_widget.presenter

    @property
    def plot_presenter(self):
        return self.fitting_presenter.plot_widget

    @property
    def plot_view(self):
        return self.fitting_presenter.plot_widget.view

    def table(self):
        return self.data_view.table_selection

    def load_focused_data(self, add_to_plot=True):
        """Press Load on the fitting tab and wait for its worker."""
        self.show_tab(TAB_FITTING)
        set_checkbox(self.data_view.check_addToPlot, add_to_plot)
        click(self.data_view.button_load)
        self.wait_for_async_task(self.data_presenter.worker, what="fitting data load")
        process_events(3)

    def focused_files(self):
        """The focused nexus files, as the prefill notifier offers them."""
        return [name for name in self.basenames_under(self.focus_dir(), ".nxs") if "_TOF" in name]

    def table_run_column(self):
        return [self.table().item(row, COL_RUN).text() for row in range(self.table().rowCount())]


class EngDiffGuiFittingDataTest(_FittingTestBase):
    """Loading focused data, the selection table and the background subtraction."""

    def test_loading_and_plotting_fit_data(self):
        self.calibrate_and_focus()
        self._check_prefill_and_filters()
        self._check_loading()
        self._check_log_tables()
        self._check_plot_checkbox()
        self._check_plot_docking()
        self._check_background_subtraction()
        self._check_plot_background_button()
        self._check_ads_deletion()
        self._check_removal()
        self._check_reload_reuses_log_values()
        self._check_dspacing_files()

    def _check_prefill_and_filters(self):
        self.show_tab(TAB_FITTING)
        finder = self.data_view.finder_data

        with self.subTest("Fitting / focusing prefills the fitting file finder"):
            text = finder.getText()
            self.assertTrue(text, "the fitting finder was not prefilled after focusing")
            for run in (CERIA, SECOND_SAMPLE):
                self.assertIn(run, text, f"{run} is missing from the prefilled file list")

        with self.subTest("Fitting / the finder is prefilled with the TOF files"):
            self.assertIn("_TOF", finder.getText())

        # the browse filter is built from the two combos, and it is what decides which of the many
        # focused outputs a user is offered
        from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_view import (
            _file_filter_generator,
        )

        with self.subTest("Fitting / the Unit and Region filters build the expected file filter"):
            self.assertEqual("*bank_1*_TOF*", _file_filter_generator({"Region": "1 (North)", "Unit": "TOF"}))
            self.assertEqual("*Texture*_dSpacing*", _file_filter_generator({"Region": "Texture", "Unit": "dSpacing"}))
            self.assertEqual("*bank_*", _file_filter_generator({"Region": "Both Banks", "Unit": "No Unit Filter"}))

    def _check_loading(self):
        self.load_focused_data(add_to_plot=True)

        with self.subTest("Fitting / one table row appears per focused file"):
            self.assertEqual(2, self.table().rowCount(), f"expected two rows, got {self.table_run_column()}")

        with self.subTest("Fitting / the table names the run and the bank it came from"):
            self.assertEqual(sorted([CERIA, SECOND_SAMPLE]), sorted(self.table_run_column()))
            # the bank comes from the "bankid" log the focus writes, which has its underscores
            # replaced with spaces so it reads as a label
            banks = {self.table().item(row, COL_BANK).text() for row in range(self.table().rowCount())}
            self.assertEqual({"bank 1"}, banks)

        with self.subTest("Fitting / the loaded workspaces are tracked by the model"):
            loaded = self.data_presenter.get_loaded_ws_list()
            self.assertEqual(2, len(loaded), f"expected two loaded workspaces, got {loaded}")

        with self.subTest("Fitting / a log workspace group is created for the loaded runs"):
            from mantid.api import AnalysisDataService as ADS

            group_name = self.data_presenter.get_log_ws_group_name()
            self.assertTrue(ADS.doesExist(group_name), f"{group_name} was not created")

    def _check_log_tables(self):
        """The log tables line up with the UI table, row for row.

        The correspondence holds because both are filled by enumerating the same dict of loaded
        workspaces.
        """
        from mantid.api import AnalysisDataService as ADS

        group = ADS.retrieve(self.data_presenter.get_log_ws_group_name())
        names = list(group.getNames())
        run_info = next((name for name in names if name.startswith("run_info")), None)
        self.assertIsNotNone(run_info, f"no run_info table in {names}")

        with self.subTest("Fitting / run_info names the runs in the UI table's order"):
            table = ADS.retrieve(run_info)
            self.assertEqual(self.table().rowCount(), table.rowCount(), "run_info has a different number of rows")
            self.assertEqual(self.table_run_column(), [str(table.cell("Run", row)) for row in range(table.rowCount())])

        with self.subTest("Fitting / a table is created per selected sample log, with the same rows"):
            log_tables = [name for name in names if name != run_info]
            self.assertTrue(log_tables, f"no sample log tables in {names}")
            for name in log_tables:
                self.assertEqual(self.table().rowCount(), ADS.retrieve(name).rowCount(), f"{name} has the wrong number of rows")

    def _check_plot_docking(self):
        """The plot is a dock widget that can be floated and put back."""
        dock = self.plot_view.plot_dock

        with self.subTest("Fitting / the plot can be undocked"):
            dock.setFloating(True)
            process_events(3)
            self.assertTrue(dock.isFloating(), "the plot did not undock")

        with self.subTest("Fitting / and docked again"):
            dock.setFloating(False)
            process_events(3)
            self.assertFalse(dock.isFloating(), "the plot did not dock again")

    def _check_plot_checkbox(self):
        with self.subTest("Fitting / with Add To Plot ticked the rows are marked as plotted"):
            for row in range(self.table().rowCount()):
                self.assertTrue(self.data_view.get_item_checked(row, COL_PLOT), f"row {row} is not marked as plotted")

        with self.subTest("Fitting / and the lines really are on the axes"):
            axes = self.plot_view.get_axes()[0]
            self.assertTrue(axes.get_lines(), "nothing was plotted")

        with self.subTest("Fitting / unticking a row's Plot box removes its line"):
            before = len(self.plot_view.get_axes()[0].get_lines())
            self.data_view.set_item_checkstate(0, COL_PLOT, False)
            process_events(3)
            self.assertLess(len(self.plot_view.get_axes()[0].get_lines()), before)

        with self.subTest("Fitting / and re-ticking it puts the line back"):
            before = len(self.plot_view.get_axes()[0].get_lines())
            self.data_view.set_item_checkstate(0, COL_PLOT, True)
            process_events(3)
            self.assertGreater(len(self.plot_view.get_axes()[0].get_lines()), before)

    def _check_background_subtraction(self):
        from mantid.api import AnalysisDataService as ADS

        with self.subTest("Fitting / background subtraction is on by default for a newly loaded run"):
            for row in range(self.table().rowCount()):
                self.assertTrue(self.data_view.get_item_checked(row, COL_BGSUB), f"row {row} has no background subtraction")

        candidates = [name for name in ADS.getObjectNames() if name.endswith("_bgsub")]
        with self.subTest("Fitting / a background subtracted workspace is created for each run"):
            self.assertEqual(2, len(candidates), f"expected two _bgsub workspaces, got {candidates}")

        # work with whichever one belongs to the ceria run, without assuming the exact prefix
        bgsub_name = next((name for name in candidates if CERIA in name), None)
        self.assertIsNotNone(bgsub_name, f"no _bgsub workspace for run {CERIA} in {candidates}")

        with self.subTest("Fitting / the subtracted data really is below the raw data"):
            import numpy as np

            raw_name = bgsub_name[: -len("_bgsub")]
            raw = ADS.retrieve(raw_name).readY(0)
            subtracted = ADS.retrieve(bgsub_name).readY(0)
            self.assertTrue(np.all(subtracted <= raw + 1e-9), "the background subtraction increased the counts")
            self.assertLess(subtracted.sum(), raw.sum(), "the background subtraction removed nothing")

        with self.subTest("Fitting / changing the number of iterations changes the subtracted data"):
            import numpy as np

            before = ADS.retrieve(bgsub_name).readY(0).copy()
            row = self.table_run_column().index(CERIA)
            self.data_view.set_table_column(row, COL_NITER, 200)
            process_events(3)
            wait_until(
                lambda: not np.allclose(before, ADS.retrieve(bgsub_name).readY(0)),
                timeout=60.0,
                msg="the background estimate to be recalculated",
            )

        with self.subTest("Fitting / turning the Savitzky-Golay filter off also changes it"):
            import numpy as np

            before = ADS.retrieve(bgsub_name).readY(0).copy()
            row = self.table_run_column().index(CERIA)
            self.data_view.set_item_checkstate(row, COL_SG, False)
            process_events(3)
            wait_until(
                lambda: not np.allclose(before, ADS.retrieve(bgsub_name).readY(0)),
                timeout=60.0,
                msg="the background estimate to be recalculated without the filter",
            )

        with self.subTest("Fitting / unticking background subtraction puts the raw data back on the plot"):
            row = self.table_run_column().index(CERIA)
            self.data_view.set_item_checkstate(row, COL_BGSUB, False)
            process_events(3)
            raw_name = bgsub_name[: -len("_bgsub")]
            self.assertIn(raw_name, self.data_presenter.plotted)
            self.assertNotIn(bgsub_name, self.data_presenter.plotted)
            # the subtracted workspace itself is deliberately kept, so re-ticking is instant rather
            # than recalculating the background
            self.assertTrue(ADS.doesExist(bgsub_name))

    def _check_plot_background_button(self):
        from mantid.api import AnalysisDataService as ADS

        with self.subTest("Fitting / the Inspect Background button needs a selected row"):
            self.table().clearSelection()
            process_events(2)
            self.assertFalse(self.data_view.button_plotBG.isEnabled())

        with self.subTest("Fitting / selecting a row enables it"):
            self.table().selectRow(0)
            process_events(2)
            self.assertTrue(self.data_view.button_plotBG.isEnabled())

        # re-enable the subtraction on the selected row, which is what there is to inspect
        self.data_view.set_item_checkstate(0, COL_BGSUB, True)
        process_events(3)
        before = figure_numbers()
        click(self.data_view.button_plotBG)
        process_events(3)
        figures = new_figures(before)

        with self.subTest("Fitting / pressing it opens a figure"):
            self.assertTrue(figures, "Inspect Background opened no figure")
        # a precondition for the content checks below
        self.assertEqual(1, len(figures), f"expected one Inspect Background figure, got {len(figures)}")

        raw_name = self.data_presenter.row_numbers[0]
        axes = figures[0].axes
        with self.subTest("Fitting / the figure shows the raw data over the subtracted data"):
            self.assertEqual(2, len(axes), "expected the data and the subtracted data on separate axes")
            assert_curve_matches_workspace(axes[0], ADS.retrieve(raw_name))
            assert_curve_matches_workspace(axes[1], ADS.retrieve(f"{raw_name}_bgsub"))

        with self.subTest("Fitting / the background is drawn over the raw data and named"):
            self.assertIn("background", plot_labels(axes[0]))
            self.assertIn("background subtracted data", plot_labels(axes[1]))

        with self.subTest("Fitting / the background curve really is the raw data minus the subtracted data"):
            import numpy as np

            _x, background = curve_by_label(axes[0], "background")
            _x, subtracted = curve_by_label(axes[1], "background subtracted data")
            raw = next(y for label, _x, y in curves(axes[0]) if label != "background")
            np.testing.assert_allclose(background, raw - subtracted, rtol=1e-6, atol=1e-6)

    def _check_ads_deletion(self):
        """The tab follows workspaces deleted from the ADS.

        Distinct from ``_check_removal``, which goes through the tab's own buttons: here the
        workspace is taken out from underneath the interface and its ADS observer has to notice.
        """
        from mantid.api import AnalysisDataService as ADS

        self.data_view.set_item_checkstate(0, COL_BGSUB, True)
        process_events(3)
        rows_before = self.table().rowCount()
        ws_name = self.data_presenter.row_numbers[0]

        with self.subTest("Fitting / deleting a _bgsub workspace unticks Subtract BG but keeps the row"):
            ADS.remove(f"{ws_name}_bgsub")
            process_events(3)
            self.assertEqual(rows_before, self.table().rowCount(), "the row went with the _bgsub workspace")
            self.assertFalse(self.data_view.get_item_checked(0, COL_BGSUB), "Subtract BG stayed ticked")

        with self.subTest("Fitting / deleting a focused workspace removes its row"):
            ADS.remove(ws_name)
            process_events(3)
            self.assertEqual(rows_before - 1, self.table().rowCount())
            self.assertNotIn(ws_name, self.data_presenter.get_loaded_ws_list())

    def _check_reload_reuses_log_values(self):
        """Loading a run again does not re-average its logs.

        Read off the notice log, which is the only observable: ``AverageLogData`` is a simpleapi
        call, so a fresh average would announce itself there.
        """
        with self.captured_logs(level="notice") as logs:
            self.load_focused_data(add_to_plot=False)

        with self.subTest("Fitting / the runs load again"):
            self.assertEqual(2, self.table().rowCount())

        with self.subTest("Fitting / their log values are remembered rather than re-averaged"):
            self.assertNotIn("AverageLogData", logs.text)

    def _check_dspacing_files(self):
        """The d-spacing focused files load just as the TOF ones do."""
        from mantid.api import AnalysisDataService as ADS

        click(self.data_view.button_removeAll)
        process_events(3)

        focus_dir = self.focus_dir()
        # the per-bank files only: CombinedFiles holds the same spectra under a different name, and
        # loading both just reports the second as already loaded
        dspacing = [name for name in self.files_under(focus_dir, ".nxs") if "_dSpacing" in name and os.sep not in name]
        self.assertTrue(dspacing, "the focus wrote no d-spacing nexus files to load")
        set_finder_text(self.data_view.finder_data, ",".join(os.path.join(focus_dir, name) for name in dspacing))
        click(self.data_view.button_load)
        self.wait_for_async_task(self.data_presenter.worker, what="d-spacing data load")
        process_events(3)

        with self.subTest("Fitting / a row appears for each d-spacing file"):
            self.assertEqual(len(dspacing), self.table().rowCount())

        with self.subTest("Fitting / and what was loaded really is in d-spacing"):
            loaded = self.data_presenter.get_loaded_ws_list()
            self.assertTrue(loaded, "no workspaces were loaded")
            for name in loaded:
                self.assertEqual("dSpacing", ADS.retrieve(name).getAxis(0).getUnit().unitID())

    def _check_removal(self):
        from mantid.api import AnalysisDataService as ADS

        self.table().selectRow(0)
        removed_run = self.table().item(0, COL_RUN).text()
        removed_ws = self.data_presenter.row_numbers[0]
        # counted rather than assumed: an earlier check has already taken a row out through the ADS
        remaining = self.table().rowCount() - 1

        with self.subTest("Fitting / Remove Selected drops just that row"):
            click(self.data_view.button_removeSelected)
            process_events(3)
            self.assertEqual(remaining, self.table().rowCount())
            self.assertNotIn(removed_run, self.table_run_column())

        with self.subTest("Fitting / and its workspaces leave the ADS with it"):
            # both the focused workspace and its background subtracted partner
            self.assertFalse(ADS.doesExist(removed_ws), f"{removed_ws} survived removal")
            self.assertFalse(ADS.doesExist(f"{removed_ws}_bgsub"), f"{removed_ws}_bgsub survived removal")

        with self.subTest("Fitting / Remove All empties the table"):
            click(self.data_view.button_removeAll)
            process_events(3)
            self.assertEqual(0, self.table().rowCount())
            self.assertEqual([], self.data_presenter.get_loaded_ws_list())


class EngDiffGuiFittingSettingsTest(EngDiffGuiTestBase):
    """The sample log settings - which logs are loaded, and the primary log a sequential fit sorts
    by - and that reopening the interface remembers them.

    Deliberately not a ``_FittingTestBase``: the settings dialog and the store behind it need no
    calibration, no focusing and no data at all.
    """

    def test_sample_log_settings_are_remembered(self):
        from qtpy.QtCore import Qt

        view = self.open_settings()
        original = self._checked_logs(view)
        self.assertGreater(len(original), 1, f"expected several sample logs to choose from, got {original}")

        kept, dropped = original[:-1], original[-1]
        primary = kept[0]

        # the checkbox is unticked directly, as a user would: the view's set_checked_logs only ever
        # *checks* the logs it is given, so it cannot express a deselection at all
        view.log_list.findItems(dropped, Qt.MatchExactly)[0].setCheckState(Qt.Unchecked)
        process_events(2)
        view.set_primary_log_combobox(primary)
        view.set_ascending_checked(False)
        click(view.btn_ok)
        process_events(2)

        with self.subTest("Fitting / the dialog stores the chosen logs, primary log and order"):
            self.assertEqual(kept, self._stored_logs())
            self.assertEqual(primary, self.get_engineering_setting("primary_log"))
            self.assertFalse(self.get_engineering_setting("sort_ascending", return_type=bool))

        self.rebuild_gui()
        view = self.open_settings()

        with self.subTest("Fitting / the log selection is remembered when the interface reopens"):
            self.assertEqual(kept, self._checked_logs(view))

        with self.subTest("Fitting / so are the primary log and its direction"):
            self.assertEqual(primary, view.get_primary_log())
            self.assertFalse(view.get_ascending_checked())

    @staticmethod
    def _checked_logs(view):
        return [log for log in view.get_checked_logs().split(",") if log]

    def _stored_logs(self):
        return [log for log in self.get_engineering_setting("logs").split(",") if log]


class EngDiffGuiSequentialFitTest(_FittingTestBase):
    """The fit browser, and the serial and sequential fits driven from the toolbar."""

    RB_NUMBER = "9876543"

    def test_sequential_fit(self):
        self.calibrate_and_focus()
        self.set_rb_number(self.RB_NUMBER)
        self._check_fit_needs_plotted_data()
        self.load_focused_data(add_to_plot=True)
        # the background subtraction would move the peak heights around under the fit; the fits here
        # are about the fitting machinery, so the raw focused data is used
        self._disable_background_subtraction()

        self._prepare_fit_browser()
        self._check_workspace_combo_follows_plot()
        self._check_fit_menu()
        self._check_serial_fit()
        self._check_sequential_fit()
        self._check_fit_outputs()
        self._check_run_ordering()
        # last: it replaces the function the checks above rely on
        self._check_back_to_back_parameters_are_fixed()

    def _disable_background_subtraction(self):
        for row in range(self.table().rowCount()):
            self.data_view.set_item_checkstate(row, COL_BGSUB, False)
        process_events(3)

    # ------------------------------------------------------------------ the fit browser

    def _peak_tof(self):
        """Where the chosen ceria peak sits in TOF, from the calibration rather than hard coded."""
        from mantid.api import AnalysisDataService as ADS
        from mantid.kernel import DeltaEModeType, UnitConversion

        ws_name = self.data_presenter.get_loaded_ws_list()[0]
        diff_consts = ADS.retrieve(ws_name).spectrumInfo().diffractometerConstants(0)
        return UnitConversion.run("dSpacing", "TOF", FIT_PEAK_D, 0, DeltaEModeType.Elastic, diff_consts)

    def _check_fit_needs_plotted_data(self):
        """With nothing plotted, the Fit toolbar button does nothing.

        Driven before any data is loaded, which is the only point in the run where that is true.
        """
        self.show_tab(TAB_FITTING)
        self.plot_presenter.fit_toggle()
        process_events(3)

        with self.subTest("Fitting / the Fit button does nothing while nothing is plotted"):
            self.assertFalse(self.plot_view.is_fit_browser_visible(), "the fit browser opened with no data plotted")

    def _check_workspace_combo_follows_plot(self):
        """The browser offers exactly the spectra that are on the plot."""
        browser = self.plot_view.fit_browser

        with self.subTest("Fitting / every plotted spectrum is offered in the Workspace combo"):
            self.assertEqual(sorted(self.data_presenter.get_loaded_ws_list()), sorted(browser.getWorkspaceNames()))

        with self.subTest("Fitting / unticking a row's Plot box takes it out of the combo"):
            unplotted = self.data_presenter.row_numbers[0]
            self.data_view.set_item_checkstate(0, COL_PLOT, False)
            process_events(3)
            self.assertNotIn(unplotted, list(browser.getWorkspaceNames()))

        # put it back; the fits below expect both runs to be plotted
        self.data_view.set_item_checkstate(0, COL_PLOT, True)
        process_events(3)

    def _check_fit_menu(self):
        """The plot's right-click menu, as far as it can be driven unattended.

        The real right-click handler ends in ``menu.exec()``, which blocks until a user picks
        something. The menu is therefore built here as that handler builds it and its entries
        asserted, which covers its contents but not the clicking.
        """
        from qtpy.QtWidgets import QMenu

        menu = self.plot_view.fit_browser.add_to_menu(QMenu())
        labels = [action.text() for action in menu.actions()]

        with self.subTest("Fitting / the plot menu offers the peak and background entries"):
            self.assertIn("Add peak", labels)
            self.assertIn("Select peak type", labels)
            self.assertIn("Add background", labels)

    def _check_back_to_back_parameters_are_fixed(self):
        """ENGIN-X pins BackToBackExponential's A and B.

        The instrument parameter file fixes them, not the interface, so no fit is needed. It has to
        go through ``addFunction`` - what the menu's "Add peak" calls - because that creates the peak
        against the selected workspace, which is what applies the instrument's parameters; the same
        function loaded from a string arrives unfixed. The rest of the suite fits Gaussians to match
        the fixture, so this is the only place the real default peak shape is exercised.
        """
        browser = self.plot_view.fit_browser
        prefix = browser.addFunction("BackToBackExponential")
        process_events(3)

        handler = browser.getPeakHandler(prefix)
        self.assertIsNotNone(handler, f"no peak handler for the added function at '{prefix}'")
        function = handler.ifun()
        fixed = {function.parameterName(i) for i in range(function.nParams()) if function.isFixed(i)}

        with self.subTest("Fitting / A and B are fixed automatically for ENGIN-X data"):
            self.assertIn("A", fixed, f"A was not fixed; fixed parameters were {sorted(fixed)}")
            self.assertIn("B", fixed, f"B was not fixed; fixed parameters were {sorted(fixed)}")

    def _prepare_fit_browser(self):
        browser = self.plot_view.fit_browser
        centre = self._peak_tof()
        # the fixture writes sigma as a fixed fraction of the peak position
        sigma = 0.002 * centre

        # open the fit browser the way the toolbar does
        self.plot_presenter.fit_toggle()
        process_events(3)

        with self.subTest("Fitting / the fit browser opens once data is plotted"):
            self.assertTrue(browser.isVisible(), "the fit browser did not open")

        with self.subTest("Fitting / the browser's default peak comes from the instrument setting"):
            self.assertEqual("Gaussian", browser.defaultPeakType())

        browser.loadFunction(
            f"name=LinearBackground,A0=100,A1=0;name=Gaussian,Height=1000,PeakCentre={centre},Sigma={sigma}",
        )
        browser.setStartX(centre - 8.0 * sigma)
        browser.setEndX(centre + 8.0 * sigma)
        process_events(2)

        # a precondition: without a readable fit setup neither fit below does anything at all
        fitprop = self.plot_view.read_fitprop_from_browser()
        self.assertIsNotNone(fitprop, "the fit browser has no usable fit setup")
        self.assertIn("Gaussian", fitprop["properties"]["Function"])

    def _do_fit_all(self, sequential):
        if sequential:
            self.plot_presenter.do_seq_fit()
        else:
            self.plot_presenter.do_serial_fit()
        self.wait_for_async_task(self.plot_presenter.worker, what="sequential fit" if sequential else "serial fit")
        process_events(3)
        return self.plot_presenter.fitprop_list

    def _check_serial_fit(self):
        with self.captured_logs(level="notice") as logs:
            fitprops = self._do_fit_all(sequential=False)

        with self.subTest("Fitting / a serial fit fits every loaded run"):
            self.assertEqual(2, len(fitprops), f"expected one result per run, got {fitprops}")

        with self.subTest("Fitting / and reports itself as a serial fit"):
            self.assertIn("Serial fitting finished", logs.text)

        with self.subTest("Fitting / each serial fit converged"):
            for fitprop in fitprops:
                self.assertTrue(self._converged(fitprop["status"]), f"fit status was {fitprop['status']}")

        with self.subTest("Fitting / the runs are fitted in the order of the table"):
            # a serial fit does no sorting, so the log should retrace the table
            self.assertEqual(self.data_presenter.get_active_ws_list(), self._fit_order_from_log(logs.text))

        with self.subTest("Fitting / the fitted peak centre is the one the fixture generated"):
            centre = self._peak_tof()
            for fitprop in fitprops:
                fitted = self._fitted_parameter(fitprop["properties"]["Function"], "PeakCentre")
                self.assertAlmostEqual(centre, fitted, delta=0.01 * centre)

    def _check_sequential_fit(self):
        with self.captured_logs(level="notice") as logs:
            fitprops = self._do_fit_all(sequential=True)

        with self.subTest("Fitting / a sequential fit also fits every loaded run"):
            self.assertEqual(2, len(fitprops), f"expected one result per run, got {fitprops}")

        with self.subTest("Fitting / and reports itself as a sequential fit"):
            self.assertIn("Sequential fitting finished", logs.text)

        with self.subTest("Fitting / each sequential fit converged"):
            for fitprop in fitprops:
                self.assertTrue(self._converged(fitprop["status"]), f"fit status was {fitprop['status']}")

        with self.subTest("Fitting / the browser is left holding the last fitted function"):
            self.assertIn("Gaussian", self.plot_view.read_fitprop_from_browser()["properties"]["Function"])

        with self.subTest("Fitting / the progress bar reports a converged fit as a success"):
            # every run after the first starts from the previous result, so it converges on a
            # tolerance-limited stop rather than an exact "success" - which must still read as done
            self.assertEqual(100, self.plot_view.fit_progress_bar.value())
            self.assertTrue(self._converged(self.plot_view.fit_progress_bar.toolTip()))

    def _check_fit_outputs(self):
        from mantid.api import AnalysisDataService as ADS

        group_name = self.data_presenter.get_log_ws_group_name().split("_log")[0] + "_fits"

        with self.subTest("Fitting / the fit results are grouped together"):
            self.assertTrue(ADS.doesExist(group_name), f"{group_name} was not created")

        with self.subTest("Fitting / a matrix workspace is produced per fitted parameter"):
            # named for the function the parameter belongs to, so a model with two peaks of the same
            # type stays unambiguous
            members = list(ADS.retrieve(group_name).getNames())
            for parameter in ("Gaussian_PeakCentre", "Gaussian_Height", "Gaussian_Sigma", "LinearBackground_A0"):
                self.assertIn(parameter, members, f"{parameter} is missing from {members}")

        with self.subTest("Fitting / the peak width is reported as an FWHM as well"):
            self.assertIn("Gaussian_fwhm", list(ADS.retrieve(group_name).getNames()))

        with self.subTest("Fitting / the peak centre is also reported in d-spacing"):
            members = list(ADS.retrieve(group_name).getNames())
            d_parameters = [name for name in members if name.endswith("_dSpacing")]
            self.assertTrue(d_parameters, f"no d-spacing conversion in {members}")

        with self.subTest("Fitting / the d-spacing conversion is of the peak that was fitted"):
            import numpy as np

            members = list(ADS.retrieve(group_name).getNames())
            d_name = next(name for name in members if name.endswith("_dSpacing"))
            values = ADS.retrieve(d_name).extractY()
            finite = values[np.isfinite(values)]
            self.assertTrue(finite.size, f"{d_name} holds no finite values")
            self.assertTrue(np.allclose(FIT_PEAK_D, finite, rtol=0.02), f"expected d = {FIT_PEAK_D}, got {finite}")

        with self.subTest("Fitting / the model summary table has one row per fitted run"):
            table = ADS.retrieve("model")
            self.assertEqual(["Workspace", "chisq/DOF", "status", "Model"], list(table.getColumnNames()))
            self.assertEqual(2, table.rowCount())
            for row in range(table.rowCount()):
                self.assertTrue(self._converged(table.cell("status", row)), f"row {row} reports {table.cell('status', row)}")

        with self.subTest("Fitting / a fit parameter table is saved for each run"):
            saved = self.basenames_under(os.path.join(self.save_dir, "User", self.RB_NUMBER, "FitParameters"))
            self.assertTrue(saved, "no fit parameter files were saved")
            for run in (CERIA, SECOND_SAMPLE):
                self.assertTrue(
                    any(run in name and name.endswith("_Fit_Parameters.nxs") for name in saved),
                    f"no fit parameter file for run {run} in {saved}",
                )

    def _check_run_ordering(self):
        """A sequential fit feeds each result into the next, so the order it visits the runs is
        part of the result rather than an implementation detail."""
        from mantid.api import AnalysisDataService as ADS

        # the primary log has to be one the tab actually tabulates - the combo in the settings only
        # ever offers those, and the lookup is by name with no fallback
        # the group also holds a run_info table, which has no per-run average and so cannot be
        # sorted on; only the tables with an "avg" column are candidates
        group = ADS.retrieve(self.data_presenter.get_log_ws_group_name())
        log_tables = [name for name in group.getNames() if name.endswith("_Fitting") and "avg" in ADS.retrieve(name).getColumnNames()]
        self.assertTrue(log_tables, f"no sortable log tables in {group.name()}")
        primary_log = log_tables[0][: -len("_Fitting")]

        self.set_engineering_setting("primary_log", primary_log)
        self.set_engineering_setting("sort_ascending", True)
        ascending = self.data_presenter.get_sorted_active_ws_list()

        with self.subTest("Fitting / sorting by a primary log keeps every run"):
            self.assertEqual(sorted(self.data_presenter.get_active_ws_list()), sorted(ascending))

        # the order actually fitted is only observable on the notice log, so that is what is checked
        # here rather than only the order the presenter intends
        with self.captured_logs(level="notice") as logs:
            self._do_fit_all(sequential=True)

        with self.subTest("Fitting / the runs are fitted in the primary log's order"):
            self.assertEqual(ascending, self._fit_order_from_log(logs.text))

        with self.subTest("Fitting / unticking Ascending reverses the order"):
            # asserted as a reversal rather than against specific log values, because two runs can
            # legitimately share a value for a given log and then no absolute order is defined
            self.set_engineering_setting("sort_ascending", False)
            self.assertEqual(ascending[::-1], self.data_presenter.get_sorted_active_ws_list())

        with self.captured_logs(level="notice") as logs:
            self._do_fit_all(sequential=True)

        with self.subTest("Fitting / and the reversed order is the one actually fitted"):
            self.assertEqual(ascending[::-1], self._fit_order_from_log(logs.text))

        with self.subTest("Fitting / with no primary log the loaded order is kept"):
            self.set_engineering_setting("primary_log", "")
            self.set_engineering_setting("sort_ascending", True)
            self.assertEqual(self.data_presenter.get_active_ws_list(), self.data_presenter.get_sorted_active_ws_list())

    @staticmethod
    def _fit_order_from_log(text):
        """The runs a fit visited, in order, read off the notice log."""
        import re

        return re.findall(r"Starting to fit workspace (\S+)", text)

    @staticmethod
    def _converged(status):
        """Whether a Fit output status means the minimizer converged.

        Deliberately the framework's own test rather than a substring match here, so this asserts
        the same notion of convergence the tab itself uses.
        """
        from mantid.api import MinimizerStatus

        return MinimizerStatus.isConverged(status)

    @staticmethod
    def _fitted_parameter(function_string, name):
        """Pull one parameter out of the function string a fit reports back."""
        for term in function_string.split(","):
            key, _, value = term.partition("=")
            if key.strip() == name:
                try:
                    return float(value)
                except ValueError:
                    # ties and constraints put the same name on the left of an expression rather
                    # than a number, e.g. "ties=(Sigma=0.5*Height)"
                    raise AssertionError(f"{name} is not a fitted value in '{function_string}'")
        raise AssertionError(f"{name} is not in the fitted function '{function_string}'")
