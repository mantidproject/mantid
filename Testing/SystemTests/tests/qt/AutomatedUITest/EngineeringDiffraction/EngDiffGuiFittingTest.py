# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""System tests for the Fitting tab.

Nothing here is mocked. The tab is reached the way the guide reaches it - by calibrating and
focusing on the Run Processing tab first - so the notifier that prefills the fitting file finder is
exercised rather than assumed, and the workspaces being fitted are the ones focusing actually
produced.

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
    click,
    figure_numbers,
    process_events,
    set_checkbox,
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

    def requiredMemoryMB(self):
        return 4000

    def excludeInPullRequests(self):
        return True

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
    """Guide Test 5: loading focused data, the selection table and the background subtraction."""

    def _run_checks(self):
        self.calibrate_and_focus()
        self._check_prefill_and_filters()
        self._check_loading()
        self._check_plot_checkbox()
        self._check_background_subtraction()
        self._check_plot_background_button()
        self._check_removal()

    def _check_prefill_and_filters(self):
        self.show_tab(TAB_FITTING)
        finder = self.data_view.finder_data

        with self.check("Test 5 / focusing prefills the fitting file finder"):
            text = finder.getText()
            self.assertTrue(text, "the fitting finder was not prefilled after focusing")
            for run in (CERIA, SECOND_SAMPLE):
                self.assertIn(run, text, f"{run} is missing from the prefilled file list")

        with self.check("Test 5 / the finder is prefilled with the TOF files"):
            self.assertIn("_TOF", finder.getText())

        # the browse filter is built from the two combos, and it is what decides which of the many
        # focused outputs a user is offered
        from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_view import (
            _file_filter_generator,
        )

        with self.check("Test 5 / the Unit and Region filters build the expected file filter"):
            self.assertEqual("*bank_1*_TOF*", _file_filter_generator({"Region": "1 (North)", "Unit": "TOF"}))
            self.assertEqual("*Texture*_dSpacing*", _file_filter_generator({"Region": "Texture", "Unit": "dSpacing"}))
            self.assertEqual("*bank_*", _file_filter_generator({"Region": "Both Banks", "Unit": "No Unit Filter"}))

    def _check_loading(self):
        self.load_focused_data(add_to_plot=True)

        with self.check("Test 5 / one table row appears per focused file"):
            self.assertEqual(2, self.table().rowCount(), f"expected two rows, got {self.table_run_column()}")

        with self.check("Test 5 / the table names the run and the bank it came from"):
            self.assertEqual(sorted([CERIA, SECOND_SAMPLE]), sorted(self.table_run_column()))
            # the bank comes from the "bankid" log the focus writes, which has its underscores
            # replaced with spaces so it reads as a label
            banks = {self.table().item(row, COL_BANK).text() for row in range(self.table().rowCount())}
            self.assertEqual({"bank 1"}, banks)

        with self.check("Test 5 / the loaded workspaces are tracked by the model"):
            loaded = self.data_presenter.get_loaded_ws_list()
            self.assertEqual(2, len(loaded), f"expected two loaded workspaces, got {loaded}")

        with self.check("Test 5 / a log workspace group is created for the loaded runs"):
            from mantid.api import AnalysisDataService as ADS

            group_name = self.data_presenter.get_log_ws_group_name()
            self.assertTrue(ADS.doesExist(group_name), f"{group_name} was not created")

    def _check_plot_checkbox(self):
        with self.check("Test 5 / with Add To Plot ticked the rows are marked as plotted"):
            for row in range(self.table().rowCount()):
                self.assertTrue(self.data_view.get_item_checked(row, COL_PLOT), f"row {row} is not marked as plotted")

        with self.check("Test 5 / and the lines really are on the axes"):
            axes = self.plot_view.get_axes()[0]
            self.assertTrue(axes.get_lines(), "nothing was plotted")

        with self.check("Test 5 / unticking a row's Plot box removes its line"):
            before = len(self.plot_view.get_axes()[0].get_lines())
            self.data_view.set_item_checkstate(0, COL_PLOT, False)
            process_events(3)
            self.assertLess(len(self.plot_view.get_axes()[0].get_lines()), before)

        with self.check("Test 5 / and re-ticking it puts the line back"):
            before = len(self.plot_view.get_axes()[0].get_lines())
            self.data_view.set_item_checkstate(0, COL_PLOT, True)
            process_events(3)
            self.assertGreater(len(self.plot_view.get_axes()[0].get_lines()), before)

    def _check_background_subtraction(self):
        from mantid.api import AnalysisDataService as ADS

        with self.check("Test 5 / background subtraction is on by default for a newly loaded run"):
            for row in range(self.table().rowCount()):
                self.assertTrue(self.data_view.get_item_checked(row, COL_BGSUB), f"row {row} has no background subtraction")

        candidates = [name for name in ADS.getObjectNames() if name.endswith("_bgsub")]
        with self.check("Test 5 / a background subtracted workspace is created for each run"):
            self.assertEqual(2, len(candidates), f"expected two _bgsub workspaces, got {candidates}")

        # work with whichever one belongs to the ceria run, without assuming the exact prefix
        bgsub_name = next((name for name in candidates if CERIA in name), None)
        self.assertIsNotNone(bgsub_name, f"no _bgsub workspace for run {CERIA} in {candidates}")

        with self.check("Test 5 / the subtracted data really is below the raw data"):
            import numpy as np

            raw_name = bgsub_name[: -len("_bgsub")]
            raw = ADS.retrieve(raw_name).readY(0)
            subtracted = ADS.retrieve(bgsub_name).readY(0)
            self.assertTrue(np.all(subtracted <= raw + 1e-9), "the background subtraction increased the counts")
            self.assertLess(subtracted.sum(), raw.sum(), "the background subtraction removed nothing")

        with self.check("Test 5 / changing the number of iterations changes the subtracted data"):
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

        with self.check("Test 5 / turning the Savitzky-Golay filter off also changes it"):
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

        with self.check("Test 5 / unticking background subtraction puts the raw data back on the plot"):
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
        with self.check("Test 5 / the Inspect Background button needs a selected row"):
            self.table().clearSelection()
            process_events(2)
            self.assertFalse(self.data_view.button_plotBG.isEnabled())

        with self.check("Test 5 / selecting a row enables it"):
            self.table().selectRow(0)
            process_events(2)
            self.assertTrue(self.data_view.button_plotBG.isEnabled())

        with self.check("Test 5 / and pressing it opens a figure"):
            # re-enable the subtraction on the selected row, which is what there is to inspect
            row = 0
            self.data_view.set_item_checkstate(row, COL_BGSUB, True)
            process_events(3)
            before = figure_numbers()
            click(self.data_view.button_plotBG)
            process_events(3)
            self.assertTrue(figure_numbers() - before, "Inspect Background opened no figure")

    def _check_removal(self):
        from mantid.api import AnalysisDataService as ADS

        self.table().selectRow(0)
        removed_run = self.table().item(0, COL_RUN).text()
        removed_ws = self.data_presenter.row_numbers[0]

        with self.check("Test 5 / Remove Selected drops just that row"):
            click(self.data_view.button_removeSelected)
            process_events(3)
            self.assertEqual(1, self.table().rowCount())
            self.assertNotIn(removed_run, self.table_run_column())

        with self.check("Test 5 / and its workspaces leave the ADS with it"):
            # both the focused workspace and its background subtracted partner
            self.assertFalse(ADS.doesExist(removed_ws), f"{removed_ws} survived removal")
            self.assertFalse(ADS.doesExist(f"{removed_ws}_bgsub"), f"{removed_ws}_bgsub survived removal")

        with self.check("Test 5 / Remove All empties the table"):
            click(self.data_view.button_removeAll)
            process_events(3)
            self.assertEqual(0, self.table().rowCount())
            self.assertEqual([], self.data_presenter.get_loaded_ws_list())


class EngDiffGuiSequentialFitTest(_FittingTestBase):
    """Guide Test 5: fitting from the plot toolbar, and the outputs a fit produces."""

    RB_NUMBER = "9876543"

    def _run_checks(self):
        self.calibrate_and_focus()
        self.set_rb_number(self.RB_NUMBER)
        self.load_focused_data(add_to_plot=True)
        # the background subtraction would move the peak heights around under the fit; the fits here
        # are about the fitting machinery, so the raw focused data is used
        self._disable_background_subtraction()

        self._prepare_fit_browser()
        self._check_serial_fit()
        self._check_sequential_fit()
        self._check_fit_outputs()
        self._check_run_ordering()

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

    def _prepare_fit_browser(self):
        browser = self.plot_view.fit_browser
        centre = self._peak_tof()
        # the fixture writes sigma as a fixed fraction of the peak position
        sigma = 0.002 * centre

        # open the fit browser the way the toolbar does
        self.plot_presenter.fit_toggle()
        process_events(3)

        with self.check("Test 5 / the fit browser opens once data is plotted"):
            self.assertTrue(browser.isVisible(), "the fit browser did not open")

        with self.check("Test 5 / the browser's default peak comes from the instrument setting"):
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

        with self.check("Test 5 / a serial fit fits every loaded run"):
            self.assertEqual(2, len(fitprops), f"expected one result per run, got {fitprops}")

        with self.check("Test 5 / and reports itself as a serial fit"):
            self.assertIn("Serial fitting finished", logs.text)

        with self.check("Test 5 / each serial fit converged"):
            for fitprop in fitprops:
                self.assertTrue(self._converged(fitprop["status"]), f"fit status was {fitprop['status']}")

        with self.check("Test 5 / the fitted peak centre is the one the fixture generated"):
            centre = self._peak_tof()
            for fitprop in fitprops:
                fitted = self._fitted_parameter(fitprop["properties"]["Function"], "PeakCentre")
                self.assertAlmostEqual(centre, fitted, delta=0.01 * centre)

    def _check_sequential_fit(self):
        with self.captured_logs(level="notice") as logs:
            fitprops = self._do_fit_all(sequential=True)

        with self.check("Test 5 / a sequential fit also fits every loaded run"):
            self.assertEqual(2, len(fitprops), f"expected one result per run, got {fitprops}")

        with self.check("Test 5 / and reports itself as a sequential fit"):
            self.assertIn("Sequential fitting finished", logs.text)

        with self.check("Test 5 / each sequential fit converged"):
            for fitprop in fitprops:
                self.assertTrue(self._converged(fitprop["status"]), f"fit status was {fitprop['status']}")

        with self.check("Test 5 / the browser is left holding the last fitted function"):
            self.assertIn("Gaussian", self.plot_view.read_fitprop_from_browser()["properties"]["Function"])

        with self.check("Test 5 / the progress bar reports a converged fit as a success"):
            # every run after the first starts from the previous result, so it converges on a
            # tolerance-limited stop rather than an exact "success" - which must still read as done
            self.assertEqual(100, self.plot_view.fit_progress_bar.value())
            self.assertTrue(self._converged(self.plot_view.fit_progress_bar.toolTip()))

    def _check_fit_outputs(self):
        from mantid.api import AnalysisDataService as ADS

        group_name = self.data_presenter.get_log_ws_group_name().split("_log")[0] + "_fits"

        with self.check("Test 5 / the fit results are grouped together"):
            self.assertTrue(ADS.doesExist(group_name), f"{group_name} was not created")

        with self.check("Test 5 / a matrix workspace is produced per fitted parameter"):
            # named for the function the parameter belongs to, so a model with two peaks of the same
            # type stays unambiguous
            members = list(ADS.retrieve(group_name).getNames())
            for parameter in ("Gaussian_PeakCentre", "Gaussian_Height", "Gaussian_Sigma", "LinearBackground_A0"):
                self.assertIn(parameter, members, f"{parameter} is missing from {members}")

        with self.check("Test 5 / the peak width is reported as an FWHM as well"):
            self.assertIn("Gaussian_fwhm", list(ADS.retrieve(group_name).getNames()))

        with self.check("Test 5 / the peak centre is also reported in d-spacing"):
            members = list(ADS.retrieve(group_name).getNames())
            d_parameters = [name for name in members if name.endswith("_dSpacing")]
            self.assertTrue(d_parameters, f"no d-spacing conversion in {members}")

        with self.check("Test 5 / the d-spacing conversion is of the peak that was fitted"):
            import numpy as np

            members = list(ADS.retrieve(group_name).getNames())
            d_name = next(name for name in members if name.endswith("_dSpacing"))
            values = ADS.retrieve(d_name).extractY()
            finite = values[np.isfinite(values)]
            self.assertTrue(finite.size, f"{d_name} holds no finite values")
            self.assertTrue(np.allclose(FIT_PEAK_D, finite, rtol=0.02), f"expected d = {FIT_PEAK_D}, got {finite}")

        with self.check("Test 5 / the model summary table has one row per fitted run"):
            table = ADS.retrieve("model")
            self.assertEqual(["Workspace", "chisq/DOF", "status", "Model"], list(table.getColumnNames()))
            self.assertEqual(2, table.rowCount())
            for row in range(table.rowCount()):
                self.assertTrue(self._converged(table.cell("status", row)), f"row {row} reports {table.cell('status', row)}")

        with self.check("Test 5 / a fit parameter table is saved for each run"):
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

        with self.check("Test 5 / sorting by a primary log keeps every run"):
            self.assertEqual(sorted(self.data_presenter.get_active_ws_list()), sorted(ascending))

        with self.check("Test 5 / unticking Ascending reverses the order the runs are fitted in"):
            # asserted as a reversal rather than against specific log values, because two runs can
            # legitimately share a value for a given log and then no absolute order is defined
            self.set_engineering_setting("sort_ascending", False)
            self.assertEqual(ascending[::-1], self.data_presenter.get_sorted_active_ws_list())

        with self.check("Test 5 / with no primary log the loaded order is kept"):
            self.set_engineering_setting("primary_log", "")
            self.set_engineering_setting("sort_ascending", True)
            self.assertEqual(self.data_presenter.get_active_ws_list(), self.data_presenter.get_sorted_active_ws_list())

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
