# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Automated UI tests for the Absorption Correction tab.

This tab is the heaviest user of the generated-algorithm-dialog seam: the sample shape, the sample
material and the orientation are all set by opening a real ``InterfaceManager`` dialog and reacting
to the algorithm finishing. A real dialog would block an unattended run forever, so
``algorithm_dialog_runs`` stands in for the user pressing Run - it executes the *real* algorithm and
then drives the same observer callback the real dialog does, leaving everything downstream of the
dialog unmodified. That seam is exercised here for all four algorithms the tab opens.

Everything else is real: real workspaces, a real ``MonteCarloAbsorption``, real files on disk.

The sample runs are the fabricated ENGIN-X pair (see ``create_synthetic_ceria_and_vanadium``). The
tab only cares that a run loads and carries an instrument, a run number and enough spectra to
correct, so fabricated data exercises it exactly as real data would while keeping the Monte Carlo
calculation quick.
"""

import os

from eng_diff_gui_test_base import (
    CORRECTION_PRESENTER,
    ENGINX_SYNTHETIC_CERIA_RUN,
    ENGINX_SYNTHETIC_VANADIUM_RUN,
    EngDiffGuiTestBase,
    TAB_CORRECTION,
    create_enginx_ceria_and_vanadium,
)
from qt_interaction_helpers import (
    cell_button,
    click,
    combo_items,
    figure_numbers,
    process_events,
    select_combo,
    set_checkbox,
    set_finder_text,
    table_checkbox,
    table_column,
)

INSTRUMENT = "ENGINX"
CERIA = str(ENGINX_SYNTHETIC_CERIA_RUN)
VANADIUM = str(ENGINX_SYNTHETIC_VANADIUM_RUN)

# workspaces are named after the file, and the fixture writes 8-digit zero padded run numbers
CERIA_WS = f"{INSTRUMENT}{ENGINX_SYNTHETIC_CERIA_RUN:08d}"
VANADIUM_WS = f"{INSTRUMENT}{ENGINX_SYNTHETIC_VANADIUM_RUN:08d}"

# table columns, as set up in TextureCorrectionView.table_column_headers
COL_RUN, COL_SHAPE, COL_MATERIAL, COL_ORIENTATION, COL_SELECT = range(5)

RB_NUMBER = "5551234"

# the orientation and gauge volume fixtures live under Testing/Data/SystemTest/Texture
TEXTURE_DATA = "Texture"

# the CSG cuboid the manual guide asks for, as typed into the SetSampleShape dialog
CUBOID_XML = (
    "<cuboid id='sample'>"
    "<height val='0.01'/>"
    "<width val='0.01'/>"
    "<depth val='0.01'/>"
    "<centre x='0.0' y='0.0' z='0.0'/>"
    "</cuboid>"
    "<algebra val='sample'/>"
)
CUBOID_VOLUME = 1.0e-6  # a 1 cm cube, in m^3

# MonteCarloAbsorption defaults to 1000 events per point, which dominates the runtime of every
# correction applied below. None of the checks here need converged factors - they ask whether the
# settings reached the algorithm and whether changing the inputs changed the result - and the
# algorithm is seeded, so a cheap sampling is still reproducible run to run.
MC_EVENTS = 50


def monte_carlo_params(rows=5, columns=10):
    """The Monte Carlo settings string the interface takes, with a cheap event count.

    ``rows`` and ``columns`` default to the algorithm's own sparse grid, so only the event count
    differs from what a user would get.
    """
    return f"SparseInstrument:True,EventsPerPoint:{MC_EVENTS},NumberOfDetectorRows:{rows},NumberOfDetectorColumns:{columns}"


class _CorrectionTestBase(EngDiffGuiTestBase):
    """Loads the fabricated ENGIN-X runs into the correction table."""

    def seeded_settings(self):
        settings = super(_CorrectionTestBase, self).seeded_settings()
        # keep the corrected and intermediate workspaces so they can be asserted on; the default is
        # to drop them once saved, which is checked explicitly in EngDiffGuiCorrectionApplyTest
        settings["clear_absorption_ws_after_processing"] = False
        # keep every Monte Carlo calculation cheap; the parameters themselves are checked explicitly
        # in EngDiffGuiCorrectionApplyTest._check_monte_carlo_parameters
        settings["monte_carlo_params"] = monte_carlo_params()
        return settings

    def pre_gui_setup(self):
        self.data_dir = os.path.join(self.tmp_root, "enginx_data")
        os.makedirs(self.data_dir, exist_ok=True)
        create_enginx_ceria_and_vanadium(self.data_dir)
        self.add_data_search_dir(self.data_dir)

    # ------------------------------------------------------------------ helpers

    def load_runs(self, runs=f"{CERIA}, {VANADIUM}"):
        """Type run numbers into the tab's finder and press Load, as a user would."""
        self.show_tab(TAB_CORRECTION)
        set_finder_text(self.correction_view.finder_corr, runs)
        click(self.correction_view.btn_loadFiles)
        process_events(2)
        return self.table_runs()

    def table(self):
        return self.correction_view.table_loaded_data

    def table_runs(self):
        return table_column(self.table(), COL_RUN)

    def row_of(self, ws_name):
        runs = self.table_runs()
        self.assertIn(ws_name, runs, f"{ws_name} is not in the table; found {runs}")
        return runs.index(ws_name)

    def select_only(self, ws_names):
        """Tick exactly the given rows, so a following action applies to a known set."""
        click(self.correction_view.btn_deselectAll)
        for name in ws_names:
            table_checkbox(self.table(), self.row_of(name), COL_SELECT).setChecked(True)
        process_events()

    def run_alg_dialog(self, button, run_algorithm):
        """Press a button that opens a generated algorithm dialog and accept it.

        The dialog is replaced, but the algorithm it would have run is run for real and the
        presenter's own finish handling is left alone - which is what makes the table redraw and the
        reference information update part of what is being tested.
        """
        with self.algorithm_dialog_runs(CORRECTION_PRESENTER, run_algorithm):
            click(button)
        process_events(2)

    # ------------------------------------------------------------------ fixture paths

    @staticmethod
    def cube_stl():
        from mantid.api import FileFinder

        return FileFinder.Instance().getFullPath("cube.stl")

    @staticmethod
    def texture_data_file(name):
        """Resolve one of the shipped Texture fixtures.

        The data search path holds the ``SystemTest`` root rather than each of its subdirectories,
        so the relative path has to be given - and a miss has to fail loudly here rather than
        returning an empty string that only fails much later.
        """
        from mantid.api import FileFinder

        path = FileFinder.Instance().getFullPath(f"{TEXTURE_DATA}/{name}")
        if not path:
            raise RuntimeError(f"could not resolve the test fixture {TEXTURE_DATA}/{name}")
        return path

    def output_dir(self, name, rb_number=None):
        if rb_number:
            return os.path.join(self.save_dir, "User", rb_number, name)
        return os.path.join(self.save_dir, name)


class EngDiffGuiCorrectionTableTest(_CorrectionTestBase):
    """Loading runs, the table, the reference workspace and every sample-definition dialog.

    No correction is applied here, so this is the cheap half of the tab's coverage.
    """

    def required_files(self):
        return ["cube.stl", f"{TEXTURE_DATA}/rotation_as_euler.txt", f"{TEXTURE_DATA}/rotation_as_matrix.txt"]

    def test_correction_table_and_sample_definition(self):
        self.set_rb_number(RB_NUMBER)
        self._check_loading()
        self._check_selection_buttons()
        self._check_reference_workspace()
        self._check_sample_shape()
        self._check_sample_material()
        self._check_orientations()
        self._check_copy_sample()
        self._check_deleting()

    def _check_loading(self):
        runs = self.load_runs()

        with self.subTest("Correction / loading runs puts one row in the table per run"):
            self.assertEqual([CERIA_WS, VANADIUM_WS], sorted(runs))

        with self.subTest("Correction / a freshly loaded run has no shape, material or orientation"):
            row = self.row_of(CERIA_WS)
            self.assertIsNone(cell_button(self.table(), row, COL_SHAPE), "a run with no shape must not offer a view button")
            self.assertEqual("Not set", self.table().item(row, COL_MATERIAL).text())
            self.assertEqual("default", self.table().item(row, COL_ORIENTATION).text())

        with self.subTest("Correction / newly loaded rows start unselected"):
            # the presenter carries the previous tick state forward when it redraws, and a run that
            # was not in the table before had none, so nothing is acted on until the user chooses
            self.assertEqual([], self.correction_view.get_selected_workspaces())

        with self.subTest("Correction / the loaded runs are offered as a sample to copy from"):
            self.assertIn(CERIA_WS, combo_items(self.correction_view.combo_workspaceList))

        with self.subTest("Correction / loading the same run again does not duplicate its row"):
            self.load_runs(runs=CERIA)
            self.assertEqual([CERIA_WS, VANADIUM_WS], sorted(self.table_runs()))

    def _check_selection_buttons(self):
        view = self.correction_view

        click(view.btn_deselectAll)
        with self.subTest("Correction / Deselect All clears every row"):
            self.assertEqual([], view.get_selected_workspaces())

        click(view.btn_selectAll)
        with self.subTest("Correction / Select All ticks every row"):
            self.assertEqual(sorted([CERIA_WS, VANADIUM_WS]), sorted(view.get_selected_workspaces()))

        with self.subTest("Correction / a single row can be ticked on its own"):
            self.select_only([CERIA_WS])
            self.assertEqual([CERIA_WS], view.get_selected_workspaces())

    def _check_reference_workspace(self):
        from mantid.api import AnalysisDataService as ADS

        view = self.correction_view
        click(view.btn_createRefWS)
        process_events(2)

        with self.subTest("Correction / creating a reference workspace names it for the RB number"):
            expected = f"{RB_NUMBER}_reference_workspace"
            self.assertTrue(ADS.doesExist(expected), f"{expected} was not created")
            self.assertEqual(expected, view.ref_frame_status.text())

        with self.subTest("Correction / a reference workspace with no shape cannot be viewed"):
            self.assertFalse(view.btn_viewRefShape.isEnabled())
            self.assertEqual("Not set", view.ref_material_status.text())

        # give the reference a shape and a material through the real dialogs, which is also what
        # makes the rest of the reference checks meaningful
        reference = f"{RB_NUMBER}_reference_workspace"
        self._set_shape_from_stl([reference])
        self._set_material(["Fe"], [reference])

        with self.subTest("Correction / the reference section updates once it has a shape and material"):
            self.assertTrue(view.btn_viewRefShape.isEnabled())
            self.assertEqual("Fe", view.ref_material_status.text())

        with self.subTest("Correction / the reference shape can be viewed"):
            before = figure_numbers()
            click(view.btn_viewRefShape)
            process_events(3)
            self.assertTrue(figure_numbers() - before, "viewing the reference shape opened no figure")

        click(view.btn_saveRefWS)
        process_events(2)
        with self.subTest("Correction / saving the reference writes it under ReferenceWorkspaces"):
            saved = self.basenames_under(self.output_dir("ReferenceWorkspaces", RB_NUMBER))
            self.assertIn(f"{RB_NUMBER}_reference_workspace.nxs", saved)

        with self.subTest("Correction / a saved reference can be loaded back"):
            path = os.path.join(self.output_dir("ReferenceWorkspaces", RB_NUMBER), f"{RB_NUMBER}_reference_workspace.nxs")
            set_finder_text(view.finder_reference, path)
            click(view.btn_loadRef)
            process_events(2)
            self.assertEqual(f"{RB_NUMBER}_reference_workspace", view.ref_frame_status.text())
            self.assertEqual("Fe", view.ref_material_status.text())

    def _check_sample_shape(self):
        from mantid.api import AnalysisDataService as ADS
        from mantid.geometry import CSGObject, MeshObject

        self.select_only([CERIA_WS])
        self._set_shape_from_stl([CERIA_WS])

        with self.subTest("Correction / an STL shape is loaded as a mesh"):
            shape = ADS.retrieve(CERIA_WS).sample().getShape()
            self.assertIsInstance(shape, MeshObject)

        with self.subTest("Correction / the table offers a view button once a shape is set"):
            row = self.row_of(CERIA_WS)
            self.assertIsNotNone(cell_button(self.table(), row, COL_SHAPE), "no view button appeared for a shape")

        with self.subTest("Correction / the per-row shape button opens a figure"):
            before = figure_numbers()
            click(cell_button(self.table(), self.row_of(CERIA_WS), COL_SHAPE))
            process_events(3)
            self.assertTrue(figure_numbers() - before, "the row's shape button opened no figure")

        # now the CSG route, on the other run, so both shape dialogs are covered
        self.select_only([VANADIUM_WS])
        self._set_shape_from_csg([VANADIUM_WS])

        with self.subTest("Correction / a CSG shape is loaded as a constructive solid"):
            shape = ADS.retrieve(VANADIUM_WS).sample().getShape()
            self.assertIsInstance(shape, CSGObject)

        with self.subTest("Correction / the CSG shape has the volume that was asked for"):
            # the magnitude, because Mantid reports a signed volume for a centre-and-dimensions
            # cuboid - the same form Engineering's own get_cube_xml produces - and it comes back
            # negative. That is a geometry quirk, not something this tab controls.
            volume = abs(ADS.retrieve(VANADIUM_WS).sample().getShape().volume())
            self.assertAlmostEqual(CUBOID_VOLUME, volume, delta=0.01 * CUBOID_VOLUME)

    def _check_sample_material(self):
        from mantid.api import AnalysisDataService as ADS

        self.select_only([CERIA_WS])
        self._set_material(["Fe"], [CERIA_WS])

        with self.subTest("Correction / the material set through the dialog is on the workspace"):
            self.assertEqual("Fe", ADS.retrieve(CERIA_WS).sample().getMaterial().name())

        with self.subTest("Correction / and is shown in the table"):
            self.assertEqual("Fe", self.table().item(self.row_of(CERIA_WS), COL_MATERIAL).text())

        with self.subTest("Correction / a run with no material still reads 'Not set'"):
            self.assertEqual("Not set", self.table().item(self.row_of(VANADIUM_WS), COL_MATERIAL).text())

    def _check_orientations(self):
        import numpy as np
        from mantid.api import AnalysisDataService as ADS
        from mantid.simpleapi import SetGoniometer

        view = self.correction_view
        self.select_only([CERIA_WS])

        with self.subTest("Correction / a single orientation set through the dialog reaches the goniometer"):
            self.run_alg_dialog(
                view.btn_setOrientation,
                lambda: SetGoniometer(Workspace=CERIA_WS, Axis0="90,1,0,0,1", Axis1="135,0,0,1,-1"),
            )
            rotation = ADS.retrieve(CERIA_WS).run().getGoniometer().getR()
            self.assertFalse(np.allclose(rotation, np.identity(3)), "the goniometer is still the identity")

        with self.subTest("Correction / the table reports that an orientation has been set"):
            self.assertEqual("set", self.table().item(self.row_of(CERIA_WS), COL_ORIENTATION).text())

        # from a file of flattened rotation matrices
        self.select_only([CERIA_WS])
        matrix_file = self.texture_data_file("rotation_as_matrix.txt")
        self.set_engineering_setting("use_euler_angles", False)
        set_finder_text(view.finder_orientation_file, matrix_file)
        click(view.btn_loadOrientation)
        process_events(2)

        with self.subTest("Correction / an orientation file of matrices is applied verbatim"):
            with open(matrix_file) as handle:
                values = [float(value) for value in handle.readline().split(",")]
            expected = np.array(values[:9]).reshape(3, 3)
            self.assertTrue(
                np.allclose(expected, ADS.retrieve(CERIA_WS).run().getGoniometer().getR(), atol=1e-6),
                "the rotation matrix from the file was not the one applied",
            )

        # ... and of Euler angles, which are interpreted using the scheme in the settings
        self.select_only([CERIA_WS])
        self.set_engineering_setting("use_euler_angles", True)
        self.set_engineering_setting("euler_angles_scheme", "XYZ")
        self.set_engineering_setting("euler_angles_sense", "1,1,1")
        set_finder_text(view.finder_orientation_file, self.texture_data_file("rotation_as_euler.txt"))
        click(view.btn_loadOrientation)
        process_events(2)
        from_xyz = ADS.retrieve(CERIA_WS).run().getGoniometer().getR().copy()

        with self.subTest("Correction / an orientation file of Euler angles gives a real rotation"):
            self.assertFalse(np.allclose(from_xyz, np.identity(3)), "the Euler angles produced no rotation")
            # a rotation matrix is orthonormal with determinant +1; a mis-parsed file would not be
            self.assertTrue(np.allclose(from_xyz @ from_xyz.T, np.identity(3), atol=1e-6))
            self.assertAlmostEqual(1.0, float(np.linalg.det(from_xyz)), places=6)

        self.select_only([CERIA_WS])
        self.set_engineering_setting("euler_angles_scheme", "ZXZ")
        click(view.btn_loadOrientation)
        process_events(2)
        # copied so the next load cannot change it under us, as getR() returns a view of the
        # goniometer's own matrix
        from_zxz = ADS.retrieve(CERIA_WS).run().getGoniometer().getR().copy()

        with self.subTest("Correction / changing the Euler scheme changes how the same file is read"):
            self.assertFalse(np.allclose(from_xyz, from_zxz), "the Euler scheme setting had no effect")

        with self.subTest("Correction / reversing the sense of rotation also changes the result"):
            self.select_only([CERIA_WS])
            self.set_engineering_setting("euler_angles_sense", "-1,-1,-1")
            click(view.btn_loadOrientation)
            process_events(2)
            reversed_sense = ADS.retrieve(CERIA_WS).run().getGoniometer().getR()
            self.assertFalse(np.allclose(from_zxz, reversed_sense), "the sense of rotation setting had no effect")

    def _check_copy_sample(self):
        from mantid.api import AnalysisDataService as ADS
        from mantid.geometry import MeshObject

        view = self.correction_view

        with self.subTest("Correction / the reference sample can be copied onto the selected runs"):
            self.select_only([VANADIUM_WS])
            click(view.btn_copyRefSample)
            process_events(2)
            # the reference carries the STL mesh, so the CSG cuboid must have been replaced
            self.assertIsInstance(ADS.retrieve(VANADIUM_WS).sample().getShape(), MeshObject)
            self.assertEqual("Fe", ADS.retrieve(VANADIUM_WS).sample().getMaterial().name())

        with self.subTest("Correction / any loaded workspace can be used as the sample to copy from"):
            self._set_shape_from_csg([VANADIUM_WS])
            select_combo(view.combo_workspaceList, VANADIUM_WS)
            self.select_only([CERIA_WS])
            click(view.btn_copySampleToAll)
            process_events(2)
            from mantid.geometry import CSGObject

            self.assertIsInstance(ADS.retrieve(CERIA_WS).sample().getShape(), CSGObject)

    def _check_deleting(self):
        view = self.correction_view

        self.select_only([VANADIUM_WS])
        click(view.btn_deleteSelected)
        process_events(2)

        with self.subTest("Correction / Delete Selected removes only the ticked rows"):
            self.assertEqual([CERIA_WS], self.table_runs())

        with self.subTest("Correction / deleting a row does not delete the workspace"):
            from mantid.api import AnalysisDataService as ADS

            # the table is a working set, not the ADS - the guide expects the workspace to survive
            self.assertTrue(ADS.doesExist(VANADIUM_WS))

    # ------------------------------------------------------------------ dialog helpers

    def _set_shape_from_stl(self, workspaces):
        from mantid.simpleapi import LoadSampleShape

        def run():
            for ws in workspaces:
                LoadSampleShape(InputWorkspace=ws, OutputWorkspace=ws, Filename=self.cube_stl(), Scale="mm")

        self.run_alg_dialog(self.correction_view.btn_loadSampleShape, run)

    def _set_shape_from_csg(self, workspaces):
        from mantid.simpleapi import SetSample

        def run():
            for ws in workspaces:
                SetSample(InputWorkspace=ws, Geometry={"Shape": "CSG", "Value": CUBOID_XML})

        self.run_alg_dialog(self.correction_view.btn_setSampleShape, run)

    def _set_material(self, materials, workspaces):
        from mantid.simpleapi import SetSampleMaterial

        def run():
            for ws in workspaces:
                SetSampleMaterial(InputWorkspace=ws, ChemicalFormula=materials[0])

        self.run_alg_dialog(self.correction_view.btn_setSampleMaterial, run)


class EngDiffGuiCorrectionApplyTest(_CorrectionTestBase):
    """Applying the corrections: gauge volume, Monte Carlo parameters, divergence and the
    attenuation table, plus where all of it is saved."""

    def required_files(self):
        return ["cube.stl", f"{TEXTURE_DATA}/custom_gauge_volume.xml"]

    def test_applying_corrections(self):
        self.load_runs(runs=CERIA)
        self._prepare_sample()
        self._check_visibility_toggles()
        self._check_absorption_only()
        self._check_gauge_volume_choices()
        self._check_divergence()
        self._check_attenuation_table()
        self._check_monte_carlo_parameters()
        self._check_workspace_cleanup()

    def _prepare_sample(self):
        """A shape and a material are preconditions for any correction, so these are hard asserts."""
        from mantid.simpleapi import LoadSampleShape, SetSampleMaterial

        self.select_only([CERIA_WS])

        def load_shape():
            LoadSampleShape(InputWorkspace=CERIA_WS, OutputWorkspace=CERIA_WS, Filename=self.cube_stl(), Scale="mm")

        def set_material():
            SetSampleMaterial(InputWorkspace=CERIA_WS, ChemicalFormula="Fe")

        self.run_alg_dialog(self.correction_view.btn_loadSampleShape, load_shape)
        self.run_alg_dialog(self.correction_view.btn_setSampleMaterial, set_material)

        from mantid.api import AnalysisDataService as ADS

        self.assertEqual("Fe", ADS.retrieve(CERIA_WS).sample().getMaterial().name(), "the sample material was not set")

    def _check_visibility_toggles(self):
        view = self.correction_view

        with self.subTest("Correction / the gauge volume inputs follow the absorption checkbox"):
            set_checkbox(view.check_absorption, False)
            self.assertFalse(view.combo_shapeMethod.isVisible())
            set_checkbox(view.check_absorption, True)
            self.assertTrue(view.combo_shapeMethod.isVisible())

        with self.subTest("Correction / the divergence inputs follow the divergence checkbox"):
            set_checkbox(view.check_divergence, True)
            self.assertTrue(view.line_divHorz.isVisible())
            set_checkbox(view.check_divergence, False)
            self.assertFalse(view.line_divHorz.isVisible())

        with self.subTest("Correction / the attenuation table inputs follow its checkbox"):
            set_checkbox(view.check_attenTab, True)
            self.assertTrue(view.widget_attenuationTableContainer.isVisible())
            set_checkbox(view.check_attenTab, False)
            self.assertFalse(view.widget_attenuationTableContainer.isVisible())

        with self.subTest("Correction / the custom gauge volume finder appears only for a custom shape"):
            select_combo(view.combo_shapeMethod, "4mmCube")
            self.assertFalse(view.finder_gauge_vol.isVisible())
            select_combo(view.combo_shapeMethod, "Custom Shape")
            self.assertTrue(view.finder_gauge_vol.isVisible())
            select_combo(view.combo_shapeMethod, "4mmCube")

    def _check_absorption_only(self):
        from mantid.api import AnalysisDataService as ADS

        self.apply_corrections(absorption=True, divergence=False, attenuation=False)

        with self.subTest("Correction / applying absorption produces a corrected workspace"):
            self.assertTrue(ADS.doesExist(f"Corrected_{CERIA_WS}"), "no corrected workspace was produced")

        with self.subTest("Correction / the corrected workspace is in d-spacing"):
            corrected = ADS.retrieve(f"Corrected_{CERIA_WS}")
            self.assertEqual("dSpacing", corrected.getAxis(0).getUnit().unitID())
            self.assertEqual(ADS.retrieve(CERIA_WS).getNumberHistograms(), corrected.getNumberHistograms())

        with self.subTest("Correction / the correction actually changed the data"):
            import numpy as np
            from mantid.simpleapi import ConvertUnits

            original = ConvertUnits(InputWorkspace=CERIA_WS, Target="dSpacing", StoreInADS=False)
            corrected = ADS.retrieve(f"Corrected_{CERIA_WS}")
            self.assertFalse(np.allclose(original.readY(0), corrected.readY(0)), "the corrected data is identical to the input")

        with self.subTest("Correction / the corrected workspace is saved under AbsorptionCorrection"):
            self.assertIn(f"Corrected_{CERIA_WS}.nxs", self.basenames_under(self.output_dir("AbsorptionCorrection")))

        with self.subTest("Correction / the absorption calculation used the shape that was set"):
            # MonteCarloAbsorption is never stubbed - the correction workspace it produced is left
            # in the ADS so the factors themselves can be checked
            import numpy as np

            factors = ADS.retrieve("_abs_corr")
            self.assertTrue(np.all(factors.readY(0) > 0.0), "absorption factors must be positive")
            self.assertTrue(np.all(factors.readY(0) <= 1.0), "absorption factors must not exceed 1")

    def _check_gauge_volume_choices(self):
        import numpy as np
        from mantid.api import AnalysisDataService as ADS
        from Engineering.common.xml_shapes import get_cube_xml

        def logged_gauge_volume():
            # the algorithm strips the shape string before storing it as a run log
            return ADS.retrieve(CERIA_WS).run().getLogData("GaugeVolume").value.strip()

        # chosen here rather than left to whatever an earlier check happened to leave selected, so
        # the log and the factors below are known to belong to the preset
        select_combo(self.correction_view.combo_shapeMethod, "4mmCube")
        self.apply_corrections(absorption=True, divergence=False, attenuation=False)

        with self.subTest("Correction / the 4mm cube preset is written to the run as a gauge volume"):
            self.assertEqual(get_cube_xml("some-gv", 0.004).strip(), logged_gauge_volume())

        preset_factors = ADS.retrieve("_abs_corr").readY(0).copy()

        with self.subTest("Correction / a custom gauge volume file is used instead when chosen"):
            custom_file = self.texture_data_file("custom_gauge_volume.xml")
            select_combo(self.correction_view.combo_shapeMethod, "Custom Shape")
            set_finder_text(self.correction_view.finder_gauge_vol, custom_file)
            self.apply_corrections(absorption=True, divergence=False, attenuation=False)

            with open(custom_file) as handle:
                expected = handle.read()
            self.assertEqual(expected.strip(), logged_gauge_volume())

        with self.subTest("Correction / the shipped custom file describes the same volume as the preset"):
            # custom_gauge_volume.xml is itself a 4 mm cube, so this is a round trip: reading the
            # shape from a file must give the same correction as asking for the preset directly
            self.assertTrue(
                np.allclose(preset_factors, ADS.retrieve("_abs_corr").readY(0)),
                "the same gauge volume gave different absorption factors depending on how it was specified",
            )

        with self.subTest("Correction / a genuinely different gauge volume gives a different correction"):
            smaller = os.path.join(self.tmp_root, "small_gauge_volume.xml")
            with open(smaller, "w") as handle:
                handle.write(get_cube_xml("some-gv", 0.001))
            set_finder_text(self.correction_view.finder_gauge_vol, smaller)
            self.apply_corrections(absorption=True, divergence=False, attenuation=False)
            self.assertFalse(
                np.allclose(preset_factors, ADS.retrieve("_abs_corr").readY(0)),
                "shrinking the gauge volume made no difference to the absorption factors",
            )

        with self.subTest("Correction / 'No Gauge Volume' clears the gauge volume off the run"):
            from Engineering.texture.texture_helper import GAUGE_VOLUME_LOG

            select_combo(self.correction_view.combo_shapeMethod, "No Gauge Volume")
            self.apply_corrections(absorption=True, divergence=False, attenuation=False)
            # the option has to actively remove an earlier definition, otherwise the stale shape is
            # silently reused and choosing it does nothing at all
            self.assertFalse(
                ADS.retrieve(CERIA_WS).run().hasProperty(GAUGE_VOLUME_LOG),
                "the previous gauge volume was left on the workspace",
            )

        with self.subTest("Correction / and the correction really is the uncollimated one"):
            uncollimated = ADS.retrieve("_abs_corr").readY(0).copy()
            self.assertFalse(
                np.allclose(preset_factors, uncollimated),
                "correcting with no gauge volume gave the same factors as the 4mm cube",
            )

        select_combo(self.correction_view.combo_shapeMethod, "4mmCube")

    def _check_divergence(self):
        """The divergence factors are consumed and removed inside ``apply_corrections``, so unlike
        the absorption factors they can only be observed through the corrected data itself."""
        import numpy as np
        from mantid.api import AnalysisDataService as ADS
        from mantid.simpleapi import ConvertUnits

        uncorrected = ConvertUnits(InputWorkspace=CERIA_WS, Target="dSpacing", StoreInADS=False).readY(0).copy()

        self.apply_corrections(absorption=False, divergence=True, attenuation=False)
        corrected = ADS.retrieve(f"Corrected_{CERIA_WS}").readY(0).copy()

        with self.subTest("Correction / a divergence correction changes the data"):
            self.assertFalse(np.allclose(uncorrected, corrected), "the divergence correction had no effect")
            self.assertTrue(np.all(np.isfinite(corrected)), "the corrected data contains non-finite values")

        with self.subTest("Correction / the divergence parameters typed into the tab are the ones used"):
            # the correction is data / (scale * sin^2(theta)) with scale = vert * sqrt(horz^2 +
            # det_horz^2), so dividing the two spectra back out must recover that factor exactly
            view = self.correction_view
            horz, vert, det_horz = (float(view.get_div_horz()), float(view.get_div_vert()), float(view.get_div_det_horz()))
            scale = vert * np.sqrt(horz**2 + det_horz**2)
            two_theta = ADS.retrieve(CERIA_WS).spectrumInfo().twoTheta(0)
            expected = scale * np.sin(two_theta) ** 2
            usable = (uncorrected != 0.0) & (corrected != 0.0)
            ratio = uncorrected[usable] / corrected[usable]
            self.assertTrue(np.allclose(expected, ratio, rtol=1e-6), f"expected a divergence factor of {expected}, got {ratio[:3]}")

        with self.subTest("Correction / changing the divergence changes the correction"):
            self.correction_view.line_divVert.setText("0.05")
            process_events()
            self.apply_corrections(absorption=False, divergence=True, attenuation=False)
            self.assertFalse(
                np.allclose(corrected, ADS.retrieve(f"Corrected_{CERIA_WS}").readY(0)),
                "the divergence parameters made no difference to the correction",
            )
            self.correction_view.line_divVert.setText("0.02")
            process_events()

    def _check_attenuation_table(self):
        from mantid.api import AnalysisDataService as ADS

        view = self.correction_view
        set_checkbox(view.check_attenTab, True)
        view.line_evalVal.setText("2.0")
        select_combo(view.combo_Units, "dSpacing")
        process_events()

        self.apply_corrections(absorption=True, divergence=False, attenuation=True)

        # the table is named from the workspace's own instrument name, which is the full name in the
        # IDF ("ENGIN-X") rather than the short name the interface's combo box uses ("ENGINX")
        expected_name = f"ENGIN-X_{CERIA}_attenuation_coefficient_2.0_dSpacing"
        with self.subTest("Correction / the attenuation table is named for the run and evaluation point"):
            self.assertTrue(ADS.doesExist(expected_name), f"{expected_name} was not produced; found {ADS.getObjectNames()}")

        with self.subTest("Correction / the attenuation table has one mu per spectrum"):
            table = ADS.retrieve(expected_name)
            self.assertEqual(["mu"], list(table.getColumnNames()))
            self.assertEqual(ADS.retrieve(CERIA_WS).getNumberHistograms(), table.rowCount())

        with self.subTest("Correction / the attenuation table is saved under AttenuationTables"):
            self.assertIn(f"{expected_name}.nxs", self.basenames_under(self.output_dir("AttenuationTables")))

        set_checkbox(view.check_attenTab, False)

    def _check_monte_carlo_parameters(self):
        from mantid.api import AnalysisDataService as ADS

        # the parameters are only observable through the algorithm history of the workspace
        # MonteCarloAbsorption produced, which is why they are never stubbed
        self.set_engineering_setting("monte_carlo_params", monte_carlo_params(rows=5, columns=5))
        self.apply_corrections(absorption=True, divergence=False, attenuation=False)

        with self.subTest("Correction / the Monte Carlo settings reach MonteCarloAbsorption"):
            history = ADS.retrieve("_abs_corr").getHistory().getAlgorithmHistories()
            monte_carlo = [alg for alg in history if alg.name() == "MonteCarloAbsorption"]
            self.assertTrue(monte_carlo, "MonteCarloAbsorption is not in the history of the correction workspace")
            properties = {prop.name(): prop.value() for prop in monte_carlo[-1].getProperties()}
            self.assertEqual("5", properties["NumberOfDetectorRows"])
            self.assertEqual("5", properties["NumberOfDetectorColumns"])

        with self.subTest("Correction / different Monte Carlo settings give different factors"):
            import numpy as np

            coarse = ADS.retrieve("_abs_corr").readY(0).copy()
            self.set_engineering_setting("monte_carlo_params", monte_carlo_params(rows=9, columns=9))
            self.apply_corrections(absorption=True, divergence=False, attenuation=False)
            self.assertFalse(
                np.allclose(coarse, ADS.retrieve("_abs_corr").readY(0)),
                "the Monte Carlo parameters made no difference to the absorption factors",
            )

    def _check_workspace_cleanup(self):
        from mantid.api import AnalysisDataService as ADS

        self.set_engineering_setting("clear_absorption_ws_after_processing", True)
        self.apply_corrections(absorption=True, divergence=True, attenuation=False)

        with self.subTest("Correction / the intermediate workspaces are dropped when the setting asks"):
            self.assertFalse(ADS.doesExist("_abs_corr"), "the absorption factors were left in the ADS")
            self.assertFalse(ADS.doesExist("_div_corr"), "the divergence factors were left in the ADS")
            self.assertFalse(ADS.doesExist(f"Corrected_{CERIA_WS}"), "the corrected workspace was left in the ADS")

        with self.subTest("Correction / but the file is still written"):
            self.assertIn(f"Corrected_{CERIA_WS}.nxs", self.basenames_under(self.output_dir("AbsorptionCorrection")))

    # ------------------------------------------------------------------ helper

    def apply_corrections(self, absorption, divergence, attenuation):
        """Tick the wanted corrections and press Apply, waiting for the worker."""
        view = self.correction_view
        self.show_tab(TAB_CORRECTION)
        self.select_only([CERIA_WS])
        set_checkbox(view.check_absorption, absorption)
        set_checkbox(view.check_divergence, divergence)
        set_checkbox(view.check_attenTab, attenuation)
        click(view.btn_applyCorrections)
        self.wait_for_async_task(self.correction_presenter.worker, what="corrections")
        process_events(2)
