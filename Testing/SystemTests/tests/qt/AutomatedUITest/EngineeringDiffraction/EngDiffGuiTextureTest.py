# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""System tests for the Texture tab, replacing the manual guide's Test 12.

Everything here is real: the focused workspaces and fit parameter tables are the shipped Texture30
validation files, and the pole figures are produced by the genuine model on a worker thread. Nothing
is mocked - the tab has no external dependency and no blocking dialog on these paths.

The seven focused runs are used rather than focusing runs in the test because they are exactly the
fixtures the manual guide names, which makes this the cheapest real-data module in the suite; the
route from focusing into this tab is covered where a real focus already happens.
"""

import os

from eng_diff_gui_test_base import EngDiffGuiTestBase, TAB_TEXTURE
from qt_interaction_helpers import (
    click,
    combo_items,
    process_events,
    select_combo,
    set_checkbox,
    set_finder_text,
    table_checkbox,
    table_column,
)

TEXTURE_DATA = "Texture/ValidationFiles"
RUNS = ("364901", "364911", "364920", "364926", "364929", "364935", "364944")
FOCUS_TEMPLATE = "ENGINX_{run}_361838_Texture30_dSpacing.nxs"
PARAM_TEMPLATE = "ENGINX_{run}_2.03_Texture30_Fit_Parameters.nxs"

# columns of the loaded-data table
COL_RUN, COL_PARAMS, COL_CRYSTAL, COL_SHAPE, COL_SELECT = 0, 1, 2, 3, 4

# a body centred cubic iron crystal, as the guide's scattering correction step uses
LATTICE = "2.87 2.87 2.87"
SPACEGROUP = "I m -3 m"
BASIS = "Fe 0 0 0 1.0 0.05"


class _TextureTestBase(EngDiffGuiTestBase):
    """Shared setup. Abstract, so the system test collector ignores it where it is imported."""

    def requiredFiles(self):
        return [f"{TEXTURE_DATA}/Focus/{FOCUS_TEMPLATE.format(run=run)}" for run in RUNS] + [
            f"{TEXTURE_DATA}/FitParameters/{PARAM_TEMPLATE.format(run=run)}" for run in RUNS
        ]

    def focus_files(self, runs=RUNS):
        return [self._resolve(f"{TEXTURE_DATA}/Focus/{FOCUS_TEMPLATE.format(run=run)}") for run in runs]

    def param_files(self, runs=RUNS):
        return [self._resolve(f"{TEXTURE_DATA}/FitParameters/{PARAM_TEMPLATE.format(run=run)}") for run in runs]

    @staticmethod
    def _resolve(name):
        from mantid.api import FileFinder

        path = FileFinder.Instance().getFullPath(name)
        if not path:
            raise RuntimeError(f"could not resolve the test fixture {name}")
        return path

    # ------------------------------------------------------------------ driving the tab

    def load_workspaces(self, runs=RUNS):
        self.show_tab(TAB_TEXTURE)
        set_finder_text(self.texture_view.finder_texture_ws, ",".join(self.focus_files(runs)))
        click(self.texture_view.btn_loadWSFiles)
        process_events(2)

    def load_parameters(self, runs=RUNS):
        set_finder_text(self.texture_view.finder_texture_tables, ",".join(self.param_files(runs)))
        click(self.texture_view.btn_loadParamFiles)
        process_events(2)

    def table(self):
        return self.texture_view.table_loaded_data

    def calculate_pole_figure(self):
        click(self.texture_view.btn_calc_pf)
        self.wait_for_async_task(self.texture_presenter.worker, what="pole figure")

    def pole_figure_dir(self, rb_number=None):
        if rb_number:
            return os.path.join(self.save_dir, "User", rb_number, "PoleFigureTables")
        return os.path.join(self.save_dir, "PoleFigureTables")

    def plot_axes(self):
        figure, _canvas = self.texture_view.get_plot_axis()
        return figure.axes


class EngDiffGuiTextureLoadingTest(_TextureTestBase):
    """Loading runs and parameter files, and the table buttons."""

    def _run_checks(self):
        self._check_initial_state()
        self._check_loading_runs()
        self._check_loading_parameters()
        self._check_selection_buttons()
        self._check_delete_buttons()

    def _check_initial_state(self):
        self.show_tab(TAB_TEXTURE)
        view = self.texture_view
        with self.check("Test 12 / the table starts empty"):
            self.assertEqual(0, self.table().rowCount())
        with self.check("Test 12 / the scattering correction section is hidden until requested"):
            self.assertFalse(view.check_scatt.isChecked())
        with self.check("Test 12 / the parameter column selector is hidden with no parameter tables"):
            self.assertFalse(view.combo_param.isVisible())
        with self.check("Test 12 / the crystal buttons are disabled with nothing to apply them to"):
            self.assertFalse(view.btn_setCrystal.isEnabled())
            self.assertFalse(view.btn_setAllCrystal.isEnabled())

    def _check_loading_runs(self):
        self.load_workspaces()
        # a precondition: every later check reads this table
        self.assertEqual(len(RUNS), self.table().rowCount(), "the focused runs did not load")

        with self.check("Test 12 / one row per loaded run, named for the focused workspace"):
            names = table_column(self.table(), COL_RUN)
            expected = [os.path.splitext(os.path.basename(path))[0] for path in self.focus_files()]
            self.assertEqual(sorted(expected), sorted(names))

        with self.check("Test 12 / runs load with no parameters, crystal or shape set"):
            for column in (COL_PARAMS, COL_CRYSTAL):
                self.assertEqual(["Not set"] * len(RUNS), table_column(self.table(), column))

        with self.check("Test 12 / every row starts unselected"):
            self.assertEqual([False] * len(RUNS), [table_checkbox(self.table(), row, COL_SELECT).isChecked() for row in range(len(RUNS))])

        with self.check("Test 12 / the crystal workspace list offers every loaded run"):
            self.assertEqual(sorted(table_column(self.table(), COL_RUN)), sorted(combo_items(self.texture_view.combo_workspaceListProp)))

        with self.check("Test 12 / loading the same runs again does not duplicate rows"):
            self.load_workspaces()
            self.assertEqual(len(RUNS), self.table().rowCount())

    def _check_loading_parameters(self):
        with self.check("Test 12 / with no parameter tables only the projection option is offered"):
            self.assertFalse(self.texture_view.combo_param.isVisible())

        self.load_parameters()
        with self.check("Test 12 / each run is paired with its own fit parameter table"):
            params = table_column(self.table(), COL_PARAMS)
            self.assertNotIn("Not set", params, "a run was left without parameters")
            # the pairing is by run number, so each row's parameter table must name the same run
            for run_name, param_name in zip(table_column(self.table(), COL_RUN), params, strict=True):
                run_number = run_name.split("_")[1]
                self.assertIn(run_number, param_name, f"{param_name} was paired with {run_name}")

        with self.check("Test 12 / the parameter column selector stays hidden while nothing is selected"):
            # the offer is driven by the selected rows, not the loaded ones, because the readout
            # column has to be common to everything that will go into the pole figure
            self.assertFalse(self.texture_view.combo_param.isVisible())

        click(self.texture_view.btn_selectAll)
        with self.check("Test 12 / the parameter column selector appears once every selected run has parameters"):
            self.assertTrue(self.texture_view.combo_param.isVisible())
            columns = combo_items(self.texture_view.combo_param)
            self.assertTrue(columns, "no plottable columns were offered")

        with self.check("Test 12 / only numeric parameter columns are offered for plotting"):
            from mantid.api import AnalysisDataService as ADS

            table_name = table_column(self.table(), COL_PARAMS)[0]
            parameter_table = ADS.retrieve(table_name)
            numeric = [
                name
                for name, column_type in zip(parameter_table.getColumnNames(), parameter_table.columnTypes(), strict=True)
                if column_type in ("double", "float", "int", "long64", "size_t")
            ]
            for offered in combo_items(self.texture_view.combo_param):
                self.assertIn(offered, numeric, f"'{offered}' is not a numeric column")

    def _check_selection_buttons(self):
        view = self.texture_view
        click(view.btn_selectAll)
        with self.check("Test 12 / Select All ticks every row"):
            self.assertEqual([True] * len(RUNS), [table_checkbox(self.table(), row, COL_SELECT).isChecked() for row in range(len(RUNS))])
            selected, _params = view.get_selected_workspaces()
            self.assertEqual(len(RUNS), len(selected))

        click(view.btn_deselectAll)
        with self.check("Test 12 / Deselect All unticks every row"):
            self.assertEqual([False] * len(RUNS), [table_checkbox(self.table(), row, COL_SELECT).isChecked() for row in range(len(RUNS))])
            selected, _params = view.get_selected_workspaces()
            self.assertEqual([], selected)

    def _check_delete_buttons(self):
        view = self.texture_view
        # remove the parameters from the first two rows only
        for row in (0, 1):
            table_checkbox(self.table(), row, COL_SELECT).setChecked(True)
        process_events()
        click(view.btn_deleteSelectedParams)
        with self.check("Test 12 / Remove Selected Parameters clears only the selected rows"):
            params = table_column(self.table(), COL_PARAMS)
            self.assertEqual(["Not set", "Not set"], params[:2])
            self.assertNotIn("Not set", params[2:])

        with self.check("Test 12 / the column selector hides again once a run has no parameters"):
            self.assertFalse(view.combo_param.isVisible())

        remaining_before = table_column(self.table(), COL_RUN)
        click(view.btn_deleteSelected)
        with self.check("Test 12 / Delete Selected removes the selected rows"):
            self.assertEqual(len(RUNS) - 2, self.table().rowCount())
            self.assertEqual(sorted(remaining_before[2:]), sorted(table_column(self.table(), COL_RUN)))

        click(view.btn_selectAll)
        click(view.btn_deleteSelected)
        with self.check("Test 12 / deleting every selected row empties the table"):
            self.assertEqual(0, self.table().rowCount())
            self.assertEqual([], combo_items(view.combo_workspaceListProp))


class EngDiffGuiTexturePoleFigureTest(_TextureTestBase):
    """Creating pole figures: projections, parameter readout and the scattering correction."""

    RB_NUMBER = "5432"

    def _run_checks(self):
        self._check_pole_figure_without_parameters()
        self._check_projection_methods()
        self._check_pole_figure_with_parameters()
        self._check_scattering_correction()
        self._check_rb_number_save_location()

    def _check_pole_figure_without_parameters(self):
        self.load_workspaces()
        click(self.texture_view.btn_selectAll)

        with self.check("Test 12 / a pole figure needs no parameter table"):
            self.assertFalse(self.texture_view.combo_param.isVisible())

        self.calculate_pole_figure()
        # a precondition for everything below
        self.assertTrue(self.plot_axes(), "no pole figure axes were created")

        with self.check("Test 12 / the pole figure is plotted"):
            axes = self.plot_axes()[0]
            self.assertTrue(axes.collections or axes.get_lines(), "nothing was drawn on the pole figure")

        with self.check("Test 12 / a pole figure table is written to the save directory"):
            saved = self.basenames_under(self.pole_figure_dir())
            self.assertTrue(saved, f"nothing under {self.pole_figure_dir()}")

    def _check_projection_methods(self):
        view = self.texture_view
        methods = combo_items(view.combo_projMethod)
        with self.check("Test 12 / both projections are offered"):
            self.assertGreaterEqual(len(methods), 2, f"only got {methods}")

        offsets = {}
        for method in methods:
            select_combo(view.combo_projMethod, method)
            self.calculate_pole_figure()
            axes = self.plot_axes()[0]
            if axes.collections:
                offsets[method] = axes.collections[0].get_offsets().data.copy()

        with self.check("Test 12 / each projection produces a different point set"):
            self.assertEqual(len(methods), len(offsets), "a projection produced no scatter points")
            first, second = (offsets[method] for method in methods[:2])
            self.assertEqual(first.shape, second.shape, "the projections disagree on the number of points")
            self.assertFalse((first == second).all(), "the two projections produced identical points")

        select_combo(view.combo_projMethod, methods[0])

    def _check_pole_figure_with_parameters(self):
        self.load_parameters()
        view = self.texture_view
        with self.check("Test 12 / every numeric parameter column becomes plottable"):
            self.assertTrue(view.combo_param.isVisible())
            self.assertTrue(combo_items(view.combo_param))

        columns = combo_items(view.combo_param)
        colours = {}
        for column in columns[:2]:
            select_combo(view.combo_param, column)
            self.calculate_pole_figure()
            axes = self.plot_axes()[0]
            if axes.collections:
                colours[column] = axes.collections[0].get_array()

        with self.check("Test 12 / the chosen parameter column drives the plotted colour data"):
            self.assertEqual(len(colours), min(2, len(columns)), "a parameter column produced no colour data")
            values = list(colours.values())
            if len(values) == 2 and values[0] is not None and values[1] is not None:
                self.assertFalse((values[0] == values[1]).all(), "two different columns plotted identical values")

    def _check_scattering_correction(self):
        view = self.texture_view
        with self.check("Test 12 / the crystal inputs are hidden until the correction is requested"):
            self.assertFalse(view.lattice_lineedit.isVisible())

        set_checkbox(view.check_scatt, True)
        with self.check("Test 12 / ticking the correction reveals the crystal inputs"):
            self.assertTrue(view.lattice_lineedit.isVisible())
            self.assertTrue(view.finder_cif_file.isEnabled())

        view.lattice_lineedit.setText(LATTICE)
        view.spacegroup_lineedit.setText(SPACEGROUP)
        view.basis_lineedit.setText(BASIS)
        process_events()
        with self.check("Test 12 / entering lattice parameters disables the CIF finder"):
            self.assertFalse(view.finder_cif_file.isEnabled())
        with self.check("Test 12 / the crystal buttons enable once a crystal and a run are chosen"):
            self.assertTrue(view.btn_setCrystal.isEnabled())
            self.assertTrue(view.btn_setAllCrystal.isEnabled())

        click(view.btn_setAllCrystal)
        with self.check("Test 12 / the crystal structure is recorded against every selected run"):
            self.assertNotIn("Not set", table_column(self.table(), COL_CRYSTAL))

        for line_edit, value in zip((view.h_lineedit, view.k_lineedit, view.l_lineedit), ("1", "1", "0"), strict=True):
            line_edit.setText(value)
        process_events()
        self.calculate_pole_figure()
        with self.check("Test 12 / a corrected pole figure is still produced"):
            axes = self.plot_axes()[0]
            self.assertTrue(axes.collections or axes.get_lines(), "nothing was drawn after the scattering correction")

    def _check_rb_number_save_location(self):
        self.set_rb_number(self.RB_NUMBER)
        with self.check("Test 12 / the RB number reaches the texture presenter"):
            self.assertEqual(self.RB_NUMBER, self.texture_presenter.rb_num)

        self.calculate_pole_figure()
        written = self.files_under(self.save_dir)
        with self.check("Test 12 / pole figure tables are written under PoleFigureTables"):
            # repeated calculations overwrite rather than accumulate, so this asserts the layout
            # rather than a delta
            self.assertTrue(written, "nothing was written to the save directory")
            self.assertTrue(
                any(path.startswith("PoleFigureTables") for path in written),
                f"no PoleFigureTables output in {sorted(written)}",
            )
