# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""System tests for the GSAS II tab, replacing the manual guide's Tests 13 and 14.

GSAS-II itself is an external program, so the one thing mocked is the subprocess call that runs it;
the canned outputs it would have produced are shipped in ``Testing/Data/UnitTest/EngDiff_gsas2_tab``
and the mock copies them into the temporary directory the real model created. Everything on both
sides of that seam is real: the model builds the GSAS-II command line and serialises its inputs, and
then genuinely parses the outputs, builds the tables, moves the files to the user save location and
plots the result.

Note that mocking ``call_subprocess`` alone is not enough to avoid needing GSAS-II installed:
``call_gsas2`` first resolves the interpreter and ``GSASIIscriptable.py`` out of the configured
installation directory. The tests therefore stage a stub installation tree, which keeps that whole
resolution path - and the JSON the interface would have handed to GSAS-II - under test.
"""

import json
import os
import shutil
from unittest import mock

import numpy as np

from eng_diff_gui_test_base import EngDiffGuiTestBase, TAB_GSAS2
from qt_interaction_helpers import click, combo_items, process_events, select_combo, set_checkbox, set_finder_text

GSAS2_MODEL = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.model"

# the canned GSAS-II outputs were generated for this focused workspace, so the fabricated .gss below
# uses the same name and bank count to stay consistent with them
FOCUSED_BASENAME = "ENGINX_305761_307521_all_banks_TOF"
CANNED_DIR = "EngDiff_gsas2_tab"
PHASE_NAME = "Fe_gamma"
PROJECT_NAME = "SysTestRefinement"
# run_model appends the focused file's basename to the user's project name, once per data file
FULL_PROJECT = f"{PROJECT_NAME}_{FOCUSED_BASENAME}"

# a focused ENGIN-X 'all banks' file has one spectrum per bank
N_BANKS = 2
# diffractometer constants of roughly the right magnitude for ENGIN-X, used for the fabricated .prm
BANK_DIFC = (18400.0, 18500.0)


class _Gsas2TestBase(EngDiffGuiTestBase):
    """Stages the GSAS II tab's inputs and replaces the external GSAS-II call.

    Abstract - it has no ``_run_checks`` - which is also what keeps the system test collector from
    picking it up out of the modules that import it.
    """

    def requiredFiles(self):
        return [
            f"{CANNED_DIR}/gsas2_output.lst",
            f"{CANNED_DIR}/gsas2_output_1.csv",
            f"{CANNED_DIR}/gsas2_output_cell_parameters_{PHASE_NAME}.txt",
            f"{CANNED_DIR}/gsas2_output_inst_parameters_PWDR_{FOCUSED_BASENAME}_Bank_1.txt",
            f"{CANNED_DIR}/gsas2_output_inst_parameters_PWDR_{FOCUSED_BASENAME}_Bank_2.txt",
            f"{CANNED_DIR}/gsas2_output_reflections_1_{PHASE_NAME}.txt",
            f"{CANNED_DIR}/gsas2_output_reflections_2_{PHASE_NAME}.txt",
        ]

    def pre_gui_setup(self):
        self.inputs_dir = os.path.join(self.tmp_root, "focus_output")
        os.makedirs(self.inputs_dir, exist_ok=True)
        self.gss_path = _write_focused_gss(self.inputs_dir)
        self.prm_path = _write_instrument_prm(self.inputs_dir)
        self.gsas2_install = _stage_stub_gsas2_install(os.path.join(self.tmp_root, "gsas2"))

    def seeded_settings(self):
        settings = super(_Gsas2TestBase, self).seeded_settings()
        # the model refuses to call GSAS-II at all without this, and the handler resolves a real
        # interpreter out of it, hence the stub tree
        settings["path_to_gsas2"] = self.gsas2_install
        return settings

    def setUp(self):
        super(_Gsas2TestBase, self).setUp()
        self.show_tab(TAB_GSAS2)
        self.subprocess_calls = []
        patcher = mock.patch.object(
            type(self.gsas2_presenter.model), "call_subprocess", autospec=True, side_effect=self._fake_gsas2_subprocess
        )
        patcher.start()
        self.addCleanup(patcher.stop)

    # ------------------------------------------------------------------ the GSAS-II seam

    def _fake_gsas2_subprocess(self, model, command_string_list, gsas_binary_paths):
        """Stand in for the external GSAS-II process.

        Records the command line the interface built - the third argument is the serialised inputs,
        which is the only place several settings are observable - and drops the canned output files
        into the temporary directory the real model has already created, renamed to whatever project
        name the model chose. Returning them under the model's own naming rather than the canned
        naming is what lets the checks below assert on the real ``move_output_files_to_user_save``
        and table-building code.
        """
        self.subprocess_calls.append(list(command_string_list))
        _copy_canned_outputs(model.save_directories.temporary_save_directory, model.save_directories.project_name)
        return ("GSAS-II ran", "1.23")

    def serialized_inputs(self, call_index=-1):
        return json.loads(self.subprocess_calls[call_index][2])

    # ------------------------------------------------------------------ driving the tab

    def fill_in_refinement(self, project_name=PROJECT_NAME, gss_paths=None, prm_paths=None):
        view = self.gsas2_view
        set_finder_text(view.instrument_group_file_finder, ",".join(prm_paths or [self.prm_path]))
        set_finder_text(view.focused_data_file_finder, ",".join(gss_paths or [self.gss_path]))
        view.project_name_line_edit.setText(project_name)
        process_events()

    def refine(self):
        """Click Refine and let the result settle.

        Deliberately does not pause between refinements: back to back clicks are exactly the case
        that used to collide on the second-resolution name of the model's working directory, so
        leaving them back to back keeps that regression covered.
        """
        click(self.gsas2_view.refine_button)
        process_events(3)

    def gsas2_output_dir(self, project=FULL_PROJECT, rb_number=None):
        if rb_number:
            return os.path.join(self.save_dir, "User", rb_number, "GSAS2", project)
        return os.path.join(self.save_dir, "GSAS2", project)


class EngDiffGuiGsas2SingleTest(_Gsas2TestBase):
    """One focused file with two banks - the manual guide's Test 13."""

    def _run_checks(self):
        self._check_initial_tab_state()
        self._check_phase_selection()
        self._check_invalid_inputs_are_reported()
        self._check_refinement_outputs()
        self._check_saved_files()
        self._check_serialized_inputs()
        self._check_plot()
        self._check_x_limits_round_trip()

    # -------------------------------------------------------------- initial state

    def _check_initial_tab_state(self):
        view = self.gsas2_view
        with self.check("Test 13 / an empty project name is flagged as invalid"):
            self.assertTrue(view.project_name_invalid.isVisible())
            self.assertIn("No Project Name", view.project_name_invalid.toolTip())

        view.project_name_line_edit.setText(PROJECT_NAME)
        process_events()
        with self.check("Test 13 / the invalid marker clears once a project name is given"):
            self.assertFalse(view.project_name_invalid.isVisible())

        with self.check("Test 13 / Rietveld is offered but disabled"):
            self.assertIn("Rietveld", combo_items(view.refinement_method_combobox))
            index = view.refinement_method_combobox.findText("Rietveld")
            self.assertFalse(view.refinement_method_combobox.model().item(index).isEnabled())
            self.assertEqual("Pawley", view.refinement_method_combobox.currentText())

        # the guide calls out that refining microstrain together with both peak-shape parameters is
        # inadvisable, and the interface warns rather than forbids
        with self.check("Test 13 / no advisory marker until all three refinement boxes are ticked"):
            self.assertFalse(view.checkboxes_invalid.isVisible())
        for checkbox in (view.refine_microstrain_checkbox, view.refine_sigma_one_checkbox, view.refine_gamma_y_checkbox):
            set_checkbox(checkbox, True)
        with self.check("Test 13 / advisory marker appears when microstrain, Sigma-1 and Gamma(Y) are all refined"):
            self.assertTrue(view.checkboxes_invalid.isVisible())
            self.assertIn("may not be advisable", view.checkboxes_invalid.toolTip())
        set_checkbox(view.refine_microstrain_checkbox, False)
        with self.check("Test 13 / advisory marker clears when microstrain is unticked"):
            self.assertFalse(view.checkboxes_invalid.isVisible())

    def _check_phase_selection(self):
        view = self.gsas2_view
        options = combo_items(view.cifComboBox)
        with self.check("Test 13 / the phase combo offers the shipped cif files plus a custom entry"):
            self.assertGreater(len(options), 1)
            self.assertIn("Custom", options)
        with self.check("Test 13 / the custom phase file finder is hidden for a shipped phase"):
            self.assertNotEqual("Custom", view.cifComboBox.currentText())
            self.assertFalse(view.phase_file_finder.isVisible())
        select_combo(view.cifComboBox, "Custom")
        with self.check("Test 13 / choosing Custom reveals the phase file finder"):
            self.assertTrue(view.phase_file_finder.isVisible())
        # go back to a shipped phase so the refinement below has one without needing a cif on disk
        select_combo(view.cifComboBox, options[0])
        with self.check("Test 13 / the custom phase file finder is hidden again"):
            self.assertFalse(view.phase_file_finder.isVisible())

    # -------------------------------------------------------------- error handling

    def _check_invalid_inputs_are_reported(self):
        self.subprocess_calls = []
        self.fill_in_refinement(project_name="")
        with self.captured_logs(level="error") as logs:
            self.refine()
        with self.check("Test 13 / refining without a project name is rejected"):
            self.assertIn("valid Project Name", logs.text)
            self.assertEqual([], self.subprocess_calls)

        # more than one instrument file is the guide's documented error case
        self.subprocess_calls = []
        second_prm = os.path.join(self.inputs_dir, "second.prm")
        shutil.copy(self.prm_path, second_prm)
        self.fill_in_refinement(prm_paths=[self.prm_path, second_prm])
        with self.captured_logs(level="error") as logs:
            self.refine()
        with self.check("Test 13 / more than one instrument file is rejected"):
            self.assertIn("exactly one instrument file", logs.text)
            self.assertEqual([], self.subprocess_calls)

        # a focused file whose bank count disagrees with the instrument file
        self.subprocess_calls = []
        single_bank = _write_focused_gss(self.inputs_dir, basename="ENGINX_single_bank_TOF", n_banks=1)
        self.fill_in_refinement(gss_paths=[single_bank])
        with self.captured_logs(level="error") as logs:
            self.refine()
        with self.check("Test 13 / a bank count that disagrees with the instrument file is rejected"):
            self.assertIn("same number of banks", logs.text)
            self.assertEqual([], self.subprocess_calls)

    # -------------------------------------------------------------- a successful refinement

    def _check_refinement_outputs(self):
        from mantid.api import AnalysisDataService as ADS

        self.fill_in_refinement()
        # forget the rejected attempts above so the counts below are about this refinement only
        self.subprocess_calls = []
        self.refine()

        # a precondition, not an observation: nothing below means anything if GSAS-II was not called
        self.assertEqual(1, len(self.subprocess_calls), "GSAS-II was not called exactly once")

        with self.check("Test 13 / the histogram selector lists one entry per bank"):
            self.assertEqual([str(i) for i in range(1, N_BANKS + 1)], combo_items(self.gsas2_view.number_output_histograms_combobox))
            self.assertEqual("1", self.gsas2_view.number_output_histograms_combobox.currentText())

        with self.check("Test 13 / the lattice parameter table is created from the cell parameters file"):
            table = ADS.retrieve(f"{FULL_PROJECT}_GSASII_lattice_parameters")
            self.assertEqual(1, table.rowCount())
            self.assertEqual(PHASE_NAME, table.column("Phase name")[0])
            # gamma iron is cubic, so a, b and c must agree and the angles must all be 90 degrees
            lengths = [table.column(name)[0] for name in ("a", "b", "c")]
            self.assertTrue(np.allclose(lengths, lengths[0]), f"cubic phase has unequal cell lengths {lengths}")
            for angle in ("alpha", "beta", "gamma"):
                self.assertAlmostEqual(90.0, table.column(angle)[0], places=4)
            self.assertAlmostEqual(lengths[0] ** 3, table.column("volume")[0], places=3)

        with self.check("Test 13 / the microstrain column is marked as refined only when it was refined"):
            table = ADS.retrieve(f"{FULL_PROJECT}_GSASII_lattice_parameters")
            self.assertIn("Microstrain", table.getColumnNames())
            self.assertNotIn("Microstrain (Refined)", table.getColumnNames())

        with self.check("Test 13 / the instrument parameter table has a row per bank with the fit range"):
            table = ADS.retrieve(f"{FULL_PROJECT}_GSASII_instrument_parameters")
            self.assertEqual(N_BANKS, table.rowCount())
            names = table.column("Histogram name")
            self.assertEqual([f"PWDR_{FOCUSED_BASENAME}_Bank_{i}" for i in range(1, N_BANKS + 1)], names)
            for row in range(N_BANKS):
                self.assertLess(table.column("Fit X Min")[row], table.column("Fit X Max")[row])
            # both were left ticked by the initial-state checks, so both columns say so
            self.assertIn("Sigma-1 (Refined)", table.getColumnNames())
            self.assertIn("Gamma (Y) (Refined)", table.getColumnNames())

        with self.check("Test 13 / the reflections table has a row per bank and phase"):
            table = ADS.retrieve(f"{FULL_PROJECT}_GSASII_reflections")
            self.assertEqual(N_BANKS, table.rowCount())
            self.assertEqual([PHASE_NAME] * N_BANKS, table.column("Phase name"))
            for reflections in table.column("Reflections"):
                self.assertTrue(reflections.strip(), "a reflections row is empty")

        with self.check("Test 13 / an 'all banks' file is expanded into its per-bank focused workspaces"):
            # the tab loads the .nxs beside the .gss to get at the sample logs, one per bank
            loaded = self.gsas2_presenter.model._data_workspaces.get_loaded_workpace_names()
            expected = [FOCUSED_BASENAME.replace("all_banks", f"bank_{bank}") + "_GSASII" for bank in range(1, N_BANKS + 1)]
            self.assertEqual(sorted(expected), sorted(loaded))

        with self.check("Test 13 / the GSAS-II sample log group is built"):
            self.assertTrue(ADS.doesExist("logs_GSASII"))
            group_names = ADS.retrieve("logs_GSASII").getNames()
            # a run summary table plus one table per log the interface tracks for this instrument
            self.assertIn("run_info_GSASII", group_names)
            self.assertGreater(len(group_names), 1, f"only got {group_names}")
            self.assertEqual(["Instrument", "Run", "Bank", "uAmps", "Title"], ADS.retrieve("run_info_GSASII").getColumnNames())

    def _check_saved_files(self):
        output_dir = self.gsas2_output_dir()
        saved = self.basenames_under(output_dir)
        with self.check("Test 13 / the GSAS-II outputs are moved to the save directory"):
            self.assertTrue(os.path.isdir(output_dir), f"{output_dir} was not created")
            for expected in (
                f"{FULL_PROJECT}.lst",
                f"{FULL_PROJECT}.gpx",
                f"{FULL_PROJECT}_1.csv",
                f"{FULL_PROJECT}_cell_parameters_{PHASE_NAME}.txt",
                f"{FULL_PROJECT}_reflections_1_{PHASE_NAME}.txt",
            ):
                self.assertIn(expected, saved)

        with self.check("Test 13 / no temporary working directory is left behind"):
            # covers the rejected refinements above as well as the successful one: each creates a
            # working directory before validating, and all of them must clean it up again
            leftover = [name for name in os.listdir(os.path.join(self.save_dir, "GSAS2")) if name.startswith("tmp_EngDiff_GSASII")]
            self.assertEqual([], leftover)

    def _check_serialized_inputs(self):
        inputs = self.serialized_inputs()
        with self.check("Test 13 / the inputs handed to GSAS-II describe the requested refinement"):
            self.assertEqual(FULL_PROJECT, inputs["project_name"])
            self.assertEqual("Pawley", inputs["refinement_settings"]["method"])
            self.assertFalse(inputs["refinement_settings"]["microstrain"])
            self.assertTrue(inputs["refinement_settings"]["sigma_one"])
            self.assertTrue(inputs["refinement_settings"]["gamma"])
            self.assertEqual(N_BANKS, inputs["number_of_regions"])

        with self.check("Test 13 / the file paths handed to GSAS-II are the ones chosen in the tab"):
            self.assertEqual([self.prm_path], [os.path.normpath(p) for p in inputs["file_paths"]["instrument_files"]])
            self.assertEqual([self.gss_path], [os.path.normpath(p) for p in inputs["file_paths"]["data_files"]])
            self.assertTrue(inputs["file_paths"]["phase_filepaths"], "no phase file was passed to GSAS-II")

        with self.check("Test 13 / Pawley reflections were generated for the phase"):
            reflections = inputs["mantid_pawley_reflections"]
            self.assertEqual(1, len(reflections), "expected one phase")
            self.assertTrue(reflections[0], "no reflections generated")
            # each entry is [hkl, d, multiplicity], sorted by descending d
            d_values = [entry[1] for entry in reflections[0]]
            self.assertEqual(sorted(d_values, reverse=True), d_values)
            self.assertTrue(all(d >= inputs["d_spacing_min"] for d in d_values))

        with self.check("Test 13 / the command line points at the configured GSAS-II interpreter"):
            command = self.subprocess_calls[-1]
            self.assertTrue(command[0].startswith(self.gsas2_install), f"unexpected interpreter {command[0]}")
            self.assertTrue(command[1].endswith("call_G2sc.py"))

    def _check_plot(self):
        axes = self.gsas2_view.get_axes()[0]
        with self.check("Test 13 / the refinement result is plotted"):
            labels = [line.get_label() for line in axes.get_lines()]
            # observed, calculated, difference and background, plus the two draggable limit markers
            self.assertGreaterEqual(len(labels), 4, f"only got {labels}")
        with self.check("Test 13 / the four refinement curves are plotted"):
            labels = [line.get_label() for line in axes.get_lines()]
            for expected in ("observed", "calculated", "difference", "background"):
                self.assertIn(expected, labels)
        with self.check("Test 13 / reflection markers are plotted for the phase"):
            labels = [line.get_label() for line in axes.get_lines()]
            self.assertIn(f"reflections_{PHASE_NAME}", labels)
        with self.check("Test 13 / the plot is titled for the refined data file"):
            # set_x_limits runs immediately after the title is set and reaches update_figure through
            # the range markers, so this also covers the title surviving that
            self.assertEqual(f"GSAS-II Refinement {os.path.basename(self.gss_path)}", self.gsas2_view.plot_dock.windowTitle())
        with self.check("Test 13 / the plot is labelled in time of flight"):
            self.assertIn("Time-of-flight", axes.get_xlabel())

    def _check_x_limits_round_trip(self):
        view = self.gsas2_view
        original_min = float(view.x_min_line_edit.text())
        original_max = float(view.x_max_line_edit.text())
        with self.check("Test 13 / the x limits are seeded from the data"):
            self.assertLess(original_min, original_max)
            self.assertAlmostEqual(original_min, view.initial_x_limits[0], places=2)
            self.assertAlmostEqual(original_max, view.initial_x_limits[1], places=2)

        # narrowing the range and refining again must reach the serialized inputs, but only because
        # the load parameters are unchanged - that is what get_limits_if_same_load_parameters guards
        narrowed_min = original_min + 0.25 * (original_max - original_min)
        narrowed_max = original_max - 0.25 * (original_max - original_min)
        view.set_x_limits_line_edits(narrowed_min, narrowed_max)
        process_events()
        self.refine()
        with self.check("Test 13 / user x limits are passed through to GSAS-II"):
            self.assertEqual(2, len(self.subprocess_calls), "the second refinement did not run")
            limits = self.serialized_inputs()["limits"]
            self.assertEqual(N_BANKS, len(limits[0]))
            for value in limits[0]:
                self.assertAlmostEqual(narrowed_min, value, places=2)
            for value in limits[1]:
                self.assertAlmostEqual(narrowed_max, value, places=2)

        # choosing different input files must discard the limits rather than apply them to new data
        other_gss = _write_focused_gss(self.inputs_dir, basename="ENGINX_305763_307521_all_banks_TOF", n_banks=N_BANKS)
        self.fill_in_refinement(gss_paths=[other_gss])
        self.refine()
        with self.check("Test 13 / x limits are reset when different input files are chosen"):
            self.assertEqual(3, len(self.subprocess_calls), "the third refinement did not run")
            limits = self.serialized_inputs()["limits"]
            self.assertNotAlmostEqual(narrowed_min, limits[0][0], places=2)


class EngDiffGuiGsas2MultipleTest(_Gsas2TestBase):
    """Several focused files, and the RB-number save location - the manual guide's Test 14."""

    RB_NUMBER = "9876"

    def _run_checks(self):
        self._check_multiple_files_refine_separately()
        self._check_rb_number_save_location()

    def _check_multiple_files_refine_separately(self):
        from mantid.api import AnalysisDataService as ADS

        second_gss = _write_focused_gss(self.inputs_dir, basename="ENGINX_305762_307521_all_banks_TOF", n_banks=N_BANKS)
        self.fill_in_refinement(gss_paths=[self.gss_path, second_gss])
        self.refine()

        with self.check("Test 14 / each focused file is refined in its own GSAS-II call"):
            self.assertEqual(2, len(self.subprocess_calls))
            projects = [self.serialized_inputs(i)["project_name"] for i in range(2)]
            self.assertEqual([FULL_PROJECT, f"{PROJECT_NAME}_ENGINX_305762_307521_all_banks_TOF"], projects)

        with self.check("Test 14 / each refinement produces its own set of tables"):
            for project in (FULL_PROJECT, f"{PROJECT_NAME}_ENGINX_305762_307521_all_banks_TOF"):
                for suffix in ("reflections", "instrument_parameters", "lattice_parameters"):
                    self.assertTrue(ADS.doesExist(f"{project}_GSASII_{suffix}"), f"{project}_GSASII_{suffix} missing")

        with self.check("Test 14 / each refinement writes its own save directory"):
            for project in (FULL_PROJECT, f"{PROJECT_NAME}_ENGINX_305762_307521_all_banks_TOF"):
                self.assertTrue(os.path.isdir(self.gsas2_output_dir(project)), f"no output directory for {project}")

        with self.check("Test 14 / the sample logs cover every bank of both focused files"):
            self.assertTrue(ADS.doesExist("logs_GSASII"))
            loaded = self.gsas2_presenter.model._data_workspaces.get_loaded_workpace_names()
            self.assertEqual(2 * N_BANKS, len(loaded), f"expected both files' banks, got {loaded}")

    def _check_rb_number_save_location(self):
        self.set_rb_number(self.RB_NUMBER)
        self.fill_in_refinement()
        self.refine()

        rb_dir = self.gsas2_output_dir(rb_number=self.RB_NUMBER)
        with self.check("Test 14 / an RB number adds a copy under User/<RB>/GSAS2"):
            self.assertTrue(os.path.isdir(rb_dir), f"{rb_dir} was not created")
            self.assertIn(f"{FULL_PROJECT}.lst", self.basenames_under(rb_dir))
        with self.check("Test 14 / the non-RB copy is still written"):
            self.assertIn(f"{FULL_PROJECT}.lst", self.basenames_under(self.gsas2_output_dir()))


# ---------------------------------------------------------------------- fixtures


def _write_focused_gss(out_dir, basename=FOCUSED_BASENAME, n_banks=N_BANKS):
    """Write a focused GSAS file of the shape the focus tab produces.

    Fabricated rather than focused for real because the GSAS-II tab only ever reads the bin
    boundaries out of it - the counts are handed straight to GSAS-II, which is mocked here. Running
    a real calibration and focus just to obtain it would make this the slowest module in the suite
    instead of the fastest, and that path is already covered by the run processing tests.

    A matching ``_bank_<n>.nxs`` is written alongside each bank because the tab loads those for the
    sample log group after a successful refinement, exactly as the focus tab leaves them on disk.
    """
    from mantid.simpleapi import AddSampleLog, CreateWorkspace, DeleteWorkspace, SaveGSS, SaveNexusProcessed

    tof = np.arange(15000.0, 45000.0, 10.0)
    counts = 500.0 + 200.0 * np.exp(-(((tof - 25000.0) / 4000.0) ** 2))

    ws = CreateWorkspace(
        DataX=np.tile(tof, n_banks),
        DataY=np.tile(counts, n_banks),
        DataE=np.tile(np.sqrt(counts), n_banks),
        NSpec=n_banks,
        UnitX="TOF",
        OutputWorkspace="__gsas2_focused",
        EnableLogging=False,
    )
    AddSampleLog(Workspace=ws, LogName="run_number", LogType="String", LogText=basename.split("_")[1], EnableLogging=False)

    gss_path = os.path.join(out_dir, f"{basename}.gss")
    SaveGSS(InputWorkspace=ws, Filename=gss_path, SplitFiles=False, EnableLogging=False)

    # The tab derives the sample log file names from the .gss name: an 'all_banks' file is expanded
    # into one per bank, anything else is taken as a single spectrum. Mirror that exactly, or the
    # refinement succeeds but the log loading reports missing files.
    if "all_banks" in basename:
        nxs_names = [f"{basename.replace('all_banks', f'bank_{bank}')}.nxs" for bank in range(1, n_banks + 1)]
    else:
        nxs_names = [f"{basename}.nxs"]
    for nxs_name in nxs_names:
        single = CreateWorkspace(
            DataX=tof,
            DataY=counts,
            DataE=np.sqrt(counts),
            NSpec=1,
            UnitX="TOF",
            OutputWorkspace="__gsas2_focused_bank",
            EnableLogging=False,
        )
        AddSampleLog(Workspace=single, LogName="run_number", LogType="String", LogText=basename.split("_")[1], EnableLogging=False)
        SaveNexusProcessed(InputWorkspace=single, Filename=os.path.join(out_dir, nxs_name), EnableLogging=False)
        DeleteWorkspace(single, EnableLogging=False)

    DeleteWorkspace(ws, EnableLogging=False)
    return os.path.normpath(gss_path)


def _write_instrument_prm(out_dir, basename=FOCUSED_BASENAME):
    """Write the .prm the focus tab writes, from the shipped ENGIN-X header template.

    The tab reads the bank count out of the ``INS   BANK`` line and cross-checks it against the
    focused data, so the template's own header is used verbatim rather than being hand written.
    """
    from Engineering.EnggUtils import CALIB_DIR

    with open(os.path.join(CALIB_DIR, "template_ENGINX_prm_header.prm")) as template:
        header = template.read()

    lines = [header.rstrip("\n")]
    for bank, difc in enumerate(BANK_DIFC, start=1):
        lines.append(f"INS  {bank} ICONS  {difc:.2f}    0.00    0.00")
    path = os.path.join(out_dir, f"{basename}.prm")
    with open(path, "w") as prm:
        prm.write("\n".join(lines) + "\n")
    return os.path.normpath(path)


def _stage_stub_gsas2_install(root):
    """Create the minimum tree that ``GSAS2Handler`` will accept as a GSAS-II installation.

    The handler searches for the interpreter within two directory levels and for
    ``GSASIIscriptable.py`` within three, and only ever records their paths - nothing here is
    executed, because the subprocess call itself is mocked. Staging this instead of mocking the
    handler keeps the path resolution, the binary search and the JSON serialisation under test.
    """
    scriptable_dir = os.path.join(root, "GSASII")
    os.makedirs(scriptable_dir, exist_ok=True)
    interpreter = "python.exe" if os.name == "nt" else "python"
    for path in (os.path.join(root, interpreter), os.path.join(scriptable_dir, "GSASIIscriptable.py")):
        with open(path, "w") as stub:
            stub.write("")
    return os.path.normpath(root)


def _copy_canned_outputs(temporary_dir, project_name):
    """Drop the canned GSAS-II outputs into the model's temporary directory under its project name.

    The files shipped in ``Testing/Data/UnitTest`` were produced by a real GSAS-II run against the
    ENGIN-X focused data, and the model finds its outputs purely by name, so renaming them to the
    project the model chose is all that is needed for the whole output-handling path to run for
    real. The ``.gpx`` project file is not shipped (it is a large binary and only its existence is
    checked), so an empty placeholder stands in for it.
    """
    from mantid.api import FileFinder

    def canned(name):
        return FileFinder.Instance().getFullPath(f"{CANNED_DIR}/{name}")

    shutil.copy(canned("gsas2_output.lst"), os.path.join(temporary_dir, f"{project_name}.lst"))
    shutil.copy(
        canned(f"gsas2_output_cell_parameters_{PHASE_NAME}.txt"),
        os.path.join(temporary_dir, f"{project_name}_cell_parameters_{PHASE_NAME}.txt"),
    )
    for bank in range(1, N_BANKS + 1):
        # one csv and one set of reflections per histogram; only histogram 1 is shipped, and the
        # banks differ only in their counts, which nothing here asserts on
        shutil.copy(canned("gsas2_output_1.csv"), os.path.join(temporary_dir, f"{project_name}_{bank}.csv"))
        shutil.copy(
            canned(f"gsas2_output_reflections_{bank}_{PHASE_NAME}.txt"),
            os.path.join(temporary_dir, f"{project_name}_reflections_{bank}_{PHASE_NAME}.txt"),
        )
        shutil.copy(
            canned(f"gsas2_output_inst_parameters_PWDR_{FOCUSED_BASENAME}_Bank_{bank}.txt"),
            os.path.join(temporary_dir, f"{project_name}_inst_parameters_PWDR_{FOCUSED_BASENAME}_Bank_{bank}.txt"),
        )
    with open(os.path.join(temporary_dir, f"{project_name}.gpx"), "wb") as placeholder:
        placeholder.write(b"")
