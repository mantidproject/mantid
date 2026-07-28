# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Functional tests for the Texture Planner interface.

These aim to do minimal mocking to test the implementation and replace a manual test

The ``SetSampleMaterial`` dialog (``InterfaceManager``) is patched with some default entries.
The settings sub-dialog is also patched out.

Everything else should run the actual logic.

File-load tests use small, self-contained temporary fixtures (an ASCII STL, a CSG xml, an
orientation text file)
"""

import os
import shutil
import sys
import tempfile
import unittest
from unittest import mock

import numpy as np
from scipy.spatial.transform import Rotation

from matplotlib.collections import PathCollection
from matplotlib.colors import to_rgba
from mpl_toolkits.mplot3d.art3d import Line3DCollection, Path3DCollection, Poly3DCollection, Text3D

from qtpy.QtCore import Qt, QPoint
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QApplication, QStyle

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import (
    CreateGroupingWorkspace,
    CreateSimulationWorkspace,
    LoadNexus,
    SaveDetectorsGrouping,
    SetSampleMaterial,
)
from Engineering.common.instrument_config import SUPPORTED_INSTRUMENTS
from Engineering.common.xml_shapes import get_cube_xml
from Engineering.texture.texture_helper import convert_to_sscanss_frame

from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel
from mantidqtinterfaces.TexturePlanner.settings.settings_model import DEFAULT_SETTINGS
from mantidqtinterfaces.TexturePlanner.settings.settings_presenter import TexturePlannerSettingsPresenter as RealSettingsPresenter
from mantidqtinterfaces.TexturePlanner.view import (
    TexturePlannerView,
    CUSTOM_INSTRUMENT,
    EXPORT_SSCANSS,
    EXPORT_EULER,
    EXPORT_MATRIX,
    EXPORT_REFERENCE_WS,
    EXPORT_TRANSMISSION_WEIGHTING,
    GAUGE_VOL_CUSTOM_SHAPE,
)
from mantidqtinterfaces.TexturePlanner.presenter import TexturePlannerPresenter

# render off-screen so the test doesn't create windows
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

app = QApplication.instance() or QApplication(sys.argv)

PRESENTER = "mantidqtinterfaces.TexturePlanner.presenter"

# a minimal closed ASCII-STL tetrahedron (cm); enough for LoadSampleShape to build a mesh
_TETRAHEDRON_STL = """solid tet
facet normal 0 0 -1
 outer loop
  vertex 0 0 0
  vertex 0 1 0
  vertex 1 0 0
 endloop
endfacet
facet normal 0 -1 0
 outer loop
  vertex 0 0 0
  vertex 1 0 0
  vertex 0 0 1
 endloop
endfacet
facet normal -1 0 0
 outer loop
  vertex 0 0 0
  vertex 0 0 1
  vertex 0 1 0
 endloop
endfacet
facet normal 0.577 0.577 0.577
 outer loop
  vertex 1 0 0
  vertex 0 1 0
  vertex 0 0 1
 endloop
endfacet
endsolid tet
"""


class _FunctionalTestBase(unittest.TestCase):
    def setUp(self):
        # the settings dialog collaborators touch the filesystem on construction; the settings
        # interface is tested elsewhere, so patch them out and build only the main view here.
        for name in ("TexturePlannerSettingsView", "TexturePlannerSettingsPresenter"):
            patcher = mock.patch(f"{PRESENTER}.{name}")
            setattr(self, f"mock_{name}", patcher.start())
            self.addCleanup(patcher.stop)

        self._tmpdir = tempfile.mkdtemp(prefix="texplan_test_")
        self.addCleanup(self._teardown_state)

        self.model = TexturePlannerModel()
        self.view = TexturePlannerView()
        self.presenter = TexturePlannerPresenter(self.model, self.view)
        self.view.presenter = self.presenter

        self.view.show()
        QApplication.processEvents()
        self.addCleanup(self.view.close)

    def _teardown_state(self):
        # registered before view.close in addCleanup
        ADS.clear()
        shutil.rmtree(self._tmpdir, ignore_errors=True)

    # interaction helpers ------------------------------------------------
    def _click(self, widget):
        QTest.mouseClick(widget, Qt.LeftButton)

    def _click_checkbox(self, checkbox):
        # click the indicator at the left edge: I am led to believe
        # this is a reliable hot-spot for the table checkboxes too.
        indicator_w = checkbox.style().pixelMetric(QStyle.PM_IndicatorWidth)
        QTest.mouseClick(checkbox, Qt.LeftButton, pos=QPoint(max(indicator_w // 2, 4), checkbox.height() // 2))

    def _show_experiment_tab(self):
        # the instrument / goniometer / gauge-volume controls live on the second tab; it must be
        # the current tab for its widgets to report isVisible() == True.
        self.view.tabSetup.setCurrentWidget(self.view.tabExperiment)
        QApplication.processEvents()

    def _write(self, name, content):
        path = os.path.join(self._tmpdir, name)
        with open(path, "w") as f:
            f.write(content)
        return path

    def _checkbox(self, row, col):
        from qtpy.QtWidgets import QCheckBox

        return self.view.tableWidget.cellWidget(row, col).findChild(QCheckBox)

    @staticmethod
    def _aabb_extent(ws):
        # axis-aligned bounding-box size of a workspace's sample mesh. The default cube is cubic
        # (equal extents) until an initial rotation is baked in, which is exactly what the direction
        # labels' extents - and the drawn sample - read off the mesh, so it is a faithful proxy for
        # "is the initial rotation still applied to this workspace's shape".
        verts = ws.sample().getShape().getMesh().reshape(-1, 3)
        return verts.max(axis=0) - verts.min(axis=0)

    @staticmethod
    def _aabb_centre(ws):
        verts = ws.sample().getShape().getMesh().reshape(-1, 3)
        return (verts.max(axis=0) + verts.min(axis=0)) / 2

    # heavier workflow helpers ------------------------------------------
    def _load_stl(self, content=None, name="sample.stl"):
        path = self._write(name, content if content is not None else _TETRAHEDRON_STL)
        self.view.get_stl_string = lambda: path
        self.view.set_load_stl_enabled(True)
        self._click(self.view.btnSTL)
        QApplication.processEvents()

    def _load_csg_cube(self, side):
        path = self._write("shape.xml", get_cube_xml("test_cube", side))
        self.view.get_xml_string = lambda: path
        self.view.set_load_xml_enabled(True)
        self._click(self.view.btnXML)
        QApplication.processEvents()

    def _set_material_via_dialog(self, **material_kwargs):
        # emulate accepting the modal SetSampleMaterial dialog: opening it runs a real
        # SetSampleMaterial against the preset raw mesh workspace, then notifies the algorithm
        # observer - exactly the path the real dialog drives when the user accepts it.
        self.view.grpSetMaterial.setChecked(True)
        QApplication.processEvents()
        with mock.patch(f"{PRESENTER}.InterfaceManager") as mock_mgr:
            dialog = mock_mgr.return_value.createDialogFromName.return_value

            def run_dialog():
                SetSampleMaterial(InputWorkspace=self.model.workspaces.WS_MESH_RAW, **material_kwargs)
                dialog.addAlgorithmObserver.call_args.args[0].finishHandle()

            dialog.show.side_effect = run_dialog
            self._click(self.view.btnSetMaterial)
        QApplication.processEvents()

    # which getter of the (mocked) settings dialog supplies which settings-dict entry
    _SETTINGS_GETTER_KEYS = {
        "get_show_directions": "directions",
        "get_show_goniometers": "goniometers",
        "get_show_incident_beam": "incident",
        "get_show_ks": "ks",
        "get_show_scattered_beam": "scattered",
        "get_stl_scale": "stl_scale",
        "get_stl_x_deg": "stl_x_degrees",
        "get_stl_y_deg": "stl_y_degrees",
        "get_stl_z_deg": "stl_z_degrees",
        "get_stl_translation": "stl_translation_vector",
        "get_orient_axes": "orientation_axes",
        "get_orient_senses": "orientation_senses",
        "get_mc_events": "mc_events_per_point",
        "get_mc_max_scatter": "mc_max_scatter_attempts",
        "get_mc_simulate_in": "mc_simulate_in",
        "get_mc_resimulate": "mc_resimulate",
        "get_att_point": "att_point",
        "get_att_unit": "att_unit",
        "get_att_use_data_range": "att_use_data_range",
    }

    def _apply_settings_via_real_presenter(self, **overrides):
        """Drive the REAL settings presenter with a mock settings dialog returning the default
        settings plus ``overrides``, then Apply. The QSettings-backed settings model is stubbed so
        the user's real stored settings are never written."""
        settings = dict(DEFAULT_SETTINGS)
        settings.update(overrides)
        settings_view = mock.MagicMock()
        for getter, key in self._SETTINGS_GETTER_KEYS.items():
            getattr(settings_view, getter).return_value = settings[key]
        settings_presenter = RealSettingsPresenter(self.model, settings_view)
        settings_presenter.settings_model = mock.MagicMock()
        settings_presenter.set_on_settings_applied(self.presenter.on_settings_applied)
        settings_presenter.save_settings()
        QApplication.processEvents()

    def _apply_instrument_group(self, group, instrument=None):
        self._show_experiment_tab()
        if instrument is not None:
            self.view.cmbInstr.setCurrentText(instrument)
            QApplication.processEvents()
        self.view.cmbGroup.setCurrentText(group)
        QApplication.processEvents()
        self._click(self.view.btnUpdateInstr)
        QApplication.processEvents()

    def _set_gauge_volume(self, method="4mmCube", custom_file=None):
        self._show_experiment_tab()
        self.view.grpGaugeVol.setChecked(True)
        if custom_file is not None:
            self.view.get_custom_shape = lambda: custom_file
        self.view.combo_shapeMethod.setCurrentText(method)
        QApplication.processEvents()
        self._click(self.view.setGV)
        QApplication.processEvents()

    # matplotlib artist introspection -----------------------------------
    def _pf_point_scatters(self):
        # scatter PathCollections on the pole-figure axis (goniometer poles have 1 point,
        # orientation coverage has one point per detector group); quivers are PolyCollections
        # and are naturally excluded
        return [c for c in self.view.get_pf_ax().collections if isinstance(c, PathCollection)]

    def _pf_scatters_matching(self, pf_points):
        # the plotter draws (pf_xy[:, 1], pf_xy[:, 0]), i.e. columns swapped
        swapped = np.asarray(pf_points)[:, ::-1]
        return [
            c
            for c in self._pf_point_scatters()
            if len(c.get_offsets()) == len(swapped) and np.allclose(np.asarray(c.get_offsets()), swapped, atol=1e-12)
        ]

    def _drawn_sample_vertices(self):
        # the sample is drawn grey; a gauge volume (if any) adds a second, cyan Poly3DCollection
        polys = [
            c
            for c in self.view.get_lab_ax().collections
            if isinstance(c, Poly3DCollection) and np.allclose(c.get_facecolor()[0][:3], to_rgba("grey")[:3])
        ]
        self.assertEqual(len(polys), 1)
        return np.asarray(polys[0]._vec)[:3].T

    def _lab_q_tip_positions(self):
        # scatter tips of the diffraction-vector (Q) quiver bundle; the only Path3DCollection with
        # the default vis settings (ks on, scattered off)
        tips = [c for c in self.view.get_lab_ax().collections if isinstance(c, Path3DCollection)]
        self.assertEqual(len(tips), 1)
        return np.asarray(tips[0]._offsets3d).T

    def _expected_q_tip_positions(self):
        # replicate the plotter's maths: tips sit at scattering_centre + Q * 1.25 * extent
        wsm = self.model.workspaces
        mesh = wsm.updated_mesh_ws.sample().getShape().getMesh()
        extent = (np.linalg.norm(mesh, axis=(1, 2)).max() / 2) * 1.2
        return wsm.scattering_centre + self.model.geometry.detQs_lab * (1.25 * extent)

    def _lab_axis_quiver_direction(self, label):
        quivers = [c for c in self.view.get_lab_ax().collections if isinstance(c, Line3DCollection) and c.get_label() == label]
        self.assertEqual(len(quivers), 1)
        # the quiver is rooted at the origin, so its tip - the segment point farthest from the
        # origin (segment ordering within the artist is an implementation detail) - gives the axis
        points = np.asarray(quivers[0]._segments3d).reshape(-1, 3)
        tip = points[np.argmax(np.linalg.norm(points, axis=1))]
        return tip / np.linalg.norm(tip)

    def _lab_direction_labels(self):
        return {t.get_text(): np.asarray(t.get_position_3d(), dtype=float) for t in self.view.get_lab_ax().texts if isinstance(t, Text3D)}


class TestInitialState(_FunctionalTestBase):
    def test_instrument_combo_lists_supported_instruments_plus_custom(self):
        items = [self.view.cmbInstr.itemText(i) for i in range(self.view.cmbInstr.count())]
        self.assertEqual(items, list(SUPPORTED_INSTRUMENTS) + [CUSTOM_INSTRUMENT])

    def test_default_texture_directions_are_populated(self):
        self.assertEqual(self.view.get_rd_name(), "RD")
        self.assertEqual(self.view.get_nd_name(), "ND")
        self.assertEqual(self.view.get_td_name(), "TD")
        self.assertEqual(self.view.get_rd_dir(), "1,0,0")
        self.assertEqual(self.view.get_nd_dir(), "0,1,0")
        self.assertEqual(self.view.get_td_dir(), "0,0,1")

    def test_group_combo_shows_enginx_presets(self):
        items = [self.view.cmbGroup.itemText(i) for i in range(self.view.cmbGroup.count())]
        self.assertEqual(items, list(self.model.instrument.groups_for_instrument("ENGINX")))

    def test_default_two_gonio_axes_enabled_rest_disabled(self):
        self.assertEqual(self.view.get_num_gonios(), 2)
        enabled = [ax.isEnabled() for ax in self.view.gonio_axes]
        self.assertEqual(enabled, [True, True, False, False, False, False])

    def test_only_first_n_table_columns_visible(self):
        hidden = [self.view.tableWidget.isColumnHidden(i) for i in range(6)]
        self.assertEqual(hidden, [False, False, True, True, True, True])

    def test_load_and_output_buttons_initially_disabled(self):
        self.assertFalse(self.view.btnSTL.isEnabled())
        self.assertFalse(self.view.btnXML.isEnabled())
        self.assertFalse(self.view.btnOrient.isEnabled())
        self.assertFalse(self.view.btnExport.isEnabled())

    def test_export_combo_lists_base_formats_without_transmission_weighting(self):
        items = [self.view.cmbExportFormat.itemText(i) for i in range(self.view.cmbExportFormat.count())]
        self.assertEqual(items, [EXPORT_SSCANSS, "Euler Orientation File", EXPORT_MATRIX, EXPORT_REFERENCE_WS])

    def test_starts_with_one_orientation(self):
        self.assertEqual(self.model.orientations.get_num_orientations(), 1)
        self.assertEqual(self.view.tableWidget.rowCount(), 1)

    def test_default_material_is_shown(self):
        # the planner seeds the sample with the default material on construction
        self.assertEqual(self.view.lblCurrentMaterialValue.text(), "Fe")


class TestGoniometerControls(_FunctionalTestBase):
    def test_changing_num_gonios_updates_axis_enablement_and_model(self):
        self.view.spnNumAxes.setValue(4)

        enabled = [ax.isEnabled() for ax in self.view.gonio_axes]
        self.assertEqual(enabled, [True, True, True, True, False, False])
        self.assertEqual(self.model.orientations.n_gonio, 4)

    def test_changing_num_gonios_reveals_axis_columns(self):
        self.view.spnNumAxes.setValue(5)

        hidden = [self.view.tableWidget.isColumnHidden(i) for i in range(6)]
        self.assertEqual(hidden, [False, False, False, False, False, True])

    def test_editing_an_angle_rotates_the_current_orientation(self):
        self.assertTrue(np.allclose(self.model.orientations[0].R.as_matrix(), np.eye(3)))

        self.view.spnAngle0.setValue(37.5)

        # a non-zero angle about axis 0 must leave the orientation no longer at identity
        self.assertFalse(np.allclose(self.model.orientations[0].R.as_matrix(), np.eye(3)))

    def test_changing_a_sense_updates_the_stored_goniometer_string(self):
        self.view.cmbSense0.setCurrentText("Counterclockwise")

        # Counterclockwise maps to the +1 sense in the stored "angle,vx,vy,vz,sense" string
        self.assertTrue(self.model.orientations[0].gonio_strings[0].endswith(",1"))

    def test_committing_a_vector_edit_updates_the_stored_goniometer_string(self):
        self.view.edtVec1.setText("0,0,1")
        self.view.edtVec1.editingFinished.emit()

        self.assertIn("0.0,0.0,1.0", self.model.orientations[0].gonio_strings[1])

    def test_changing_step_size_updates_angle_single_step(self):
        self.view.spnStepSize.setValue(5.0)
        self.assertEqual(self.view.spnAngle0.singleStep(), 5.0)


class TestDirections(_FunctionalTestBase):
    def test_update_directions_button_pushes_directions_to_model(self):
        # the Update Directions button sits inside a collapsed, checkable group box
        self.view.grpDirectionWidgets.setChecked(True)
        QApplication.processEvents()
        self.view.set_rd_dir((0, 1, 0))
        self.view.set_nd_dir((0, 0, 1))
        self.view.set_td_dir((1, 0, 0))

        self._click(self.view.updateDirs)

        # ax_transform columns are the (normalised) RD, ND, TD vectors
        expected = np.array([[0, 0, 1], [1, 0, 0], [0, 1, 0]])
        self.assertTrue(np.allclose(self.model.ax_transform, expected))
        self.assertEqual(self.model.dir_names, ["RD", "ND", "TD"])

    def test_updating_directions_preserves_initial_shape_rotation(self):
        # bake a non-trivial initial rotation into the sample
        self.view.spnInitX.setValue(30.0)
        QApplication.processEvents()
        rotated = self._aabb_extent(self.model.workspaces.ws)
        # sanity: rotating the default cube makes its axis-aligned bounding box non-cubic
        self.assertFalse(np.allclose(rotated, rotated[0]))

        # push a new set of texture directions through the real Update Directions button
        self.view.grpDirectionWidgets.setChecked(True)
        QApplication.processEvents()
        self.view.set_rd_dir((0, 1, 0))
        self.view.set_nd_dir((0, 0, 1))
        self.view.set_td_dir((1, 0, 0))
        self._click(self.view.updateDirs)

        # updating directions regroups the data ws; the initial rotation - which sets the extent of
        # the direction-axis labels drawn from this ws - must survive that regroup
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), rotated, atol=1e-9)


class TestOrientationTable(_FunctionalTestBase):
    def test_add_orientation_button_appends_and_selects_new_row(self):
        self._click(self.view.addOrientation)

        self.assertEqual(self.model.orientations.get_num_orientations(), 2)
        self.assertEqual(self.view.tableWidget.rowCount(), 2)
        self.assertEqual(self.view.spnIndex.maximum(), 2)
        self.assertEqual(self.view.spnIndex.value(), 2)  # new orientation (index 1) shown 1-based

    def test_deselect_all_clears_every_selection(self):
        self._click(self.view.deselectAll)
        self.assertTrue(all(not o.select for o in self.model.orientations.values()))

    def test_select_all_selects_every_orientation(self):
        self.model.orientations.deselect_all()

        self._click(self.view.selectAll)

        self.assertTrue(all(o.select for o in self.model.orientations.values()))

    def test_delete_selected_removes_only_selected_rows(self):
        self._click(self.view.addOrientation)  # two orientations
        # keep index 0 (deselected), mark index 1 for deletion
        self.model.orientations.update_selected([1])

        self._click(self.view.deleteSelected)

        self.assertEqual(self.model.orientations.get_num_orientations(), 1)

    def test_changing_index_updates_the_current_orientation(self):
        self._click(self.view.addOrientation)  # now two orientations, index 1 current

        self.view.spnIndex.setValue(1)  # 1-based -> select orientation index 0

        self.assertEqual(self.model.orientations.get_orientation_index(), 0)

    def test_table_reflects_include_flags(self):
        self._click(self.view.addOrientation)  # two rows, both included by default
        self.model.orientations.update_included([1])  # exclude row 0, keep row 1

        self.presenter.update_table()

        self.assertEqual(self.view.tableWidget.item(0, 0).text(), "0.0,1.0,0.0,0.0,-1")
        self.assertFalse(self._checkbox(0, 6).isChecked())
        self.assertTrue(self._checkbox(1, 6).isChecked())

    def test_unticking_include_checkbox_excludes_that_orientation(self):
        self._click(self.view.addOrientation)  # two rows, both included

        self._click_checkbox(self._checkbox(0, 6))  # untick row 0's include box

        self.assertFalse(self.model.orientations[0].include)
        self.assertTrue(self.model.orientations[1].include)

    def test_unticking_select_checkbox_deselects_that_orientation(self):
        self._click(self.view.addOrientation)  # two rows, both selected

        self._click_checkbox(self._checkbox(0, 7))  # untick row 0's select box

        self.assertFalse(self.model.orientations[0].select)
        self.assertTrue(self.model.orientations[1].select)


class TestLoadShapeAndFiles(_FunctionalTestBase):
    def test_stl_button_enables_once_a_path_is_present(self):
        self.assertFalse(self.view.btnSTL.isEnabled())
        self.view.get_stl_string = lambda: "/data/sample.stl"

        self.view.finder_stl.fileFindingFinished.emit()

        self.assertTrue(self.view.btnSTL.isEnabled())

    def test_loading_an_stl_replaces_the_sample_with_a_mesh(self):
        stl_path = self._write("sample.stl", _TETRAHEDRON_STL)
        self.view.get_stl_string = lambda: stl_path
        self.view.set_load_stl_enabled(True)

        self._click(self.view.btnSTL)

        # default planner shape is a CSG cube; loading an STL swaps it for a mesh object
        shape = self.model.workspaces.ws.sample().getShape()
        self.assertEqual(type(shape).__name__, "MeshObject")
        # the fixture tetrahedron spans 1 unit per axis and loads at the default "cm" scale
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), (0.01, 0.01, 0.01), atol=1e-9)

    def test_loading_a_csg_xml_sets_a_valid_shape(self):
        xml_path = self._write("shape.xml", get_cube_xml("test_cube", 0.02))
        self.view.get_xml_string = lambda: xml_path
        self.view.set_load_xml_enabled(True)

        self._click(self.view.btnXML)

        # the loaded cube has side 0.02 m; volume() is signed, so compare on magnitude
        volume = self.model.workspaces.ws.sample().getShape().volume()
        self.assertAlmostEqual(abs(volume), 0.02**3, places=9)
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), (0.02, 0.02, 0.02), atol=1e-9)

    def test_loading_an_orientation_file_adds_orientations_and_sets_gonios(self):
        orient_path = self._write("orient.txt", "10,20,30\n40,50,60\n70,80,90\n")
        self.view.get_orientation_file = lambda: orient_path
        self.view.set_load_orientation_enabled(True)

        self._click(self.view.btnOrient)

        # three euler rows are appended to the initial orientation; YXY -> three gonio axes
        self.assertEqual(self.model.orientations.get_num_orientations(), 4)
        self.assertEqual(self.view.get_num_gonios(), 3)
        self.assertEqual(self.view.spnIndex.maximum(), 4)


class TestInitialShapeAndPosition(_FunctionalTestBase):
    def test_changing_initial_rotation_sets_init_R(self):
        self.view.spnInitX.setValue(15.0)

        applied = self.model.workspaces.init_R.as_euler("xyz", degrees=True)
        self.assertTrue(np.allclose(applied, [15.0, 0.0, 0.0]))

    def test_changing_initial_position_sets_offset(self):
        self.view.spnInitPY.setValue(0.005)

        self.assertTrue(np.allclose(self.model.workspaces.offset, (0.0, 0.005, 0.0)))

    def test_initial_orientation_is_applied_before_initial_translation(self):
        # orient the sample, then shift it: the offset is a lab-frame translation of the *oriented*
        # sample, so the sample centre ends up at the offset itself (the default cube is centred on
        # the origin). If the translation were applied before the orientation, the 90 deg rotation
        # about X would swing the +Y offset round onto the +Z axis and the centre would sit there.
        self.view.spnInitX.setValue(90.0)
        self.view.spnInitPY.setValue(0.02)
        QApplication.processEvents()

        verts = self.model.workspaces.ws.sample().getShape().getMesh().reshape(-1, 3)
        centre = (verts.max(axis=0) + verts.min(axis=0)) / 2
        np.testing.assert_allclose(centre, [0.0, 0.02, 0.0], atol=1e-6)

    def test_combined_rotation_and_translation_give_expected_bounds(self):
        # 45 deg about X stretches the default cube's AABB to sqrt(2) in y and z (rotation is
        # resolved first), and the offset then shifts that box wholesale in the lab frame
        self.view.spnInitX.setValue(45.0)
        self.view.spnInitPY.setValue(0.02)
        QApplication.processEvents()

        ws = self.model.workspaces.ws
        np.testing.assert_allclose(self._aabb_extent(ws), (0.01, 0.01 * np.sqrt(2), 0.01 * np.sqrt(2)), atol=1e-6)
        np.testing.assert_allclose(self._aabb_centre(ws), (0.0, 0.02, 0.0), atol=1e-6)


class TestTransmission(_FunctionalTestBase):
    def test_toggling_transmission_computes_transmission_for_the_orientation(self):
        self.assertFalse(self.view.chkTransmission.isChecked())
        self.assertIsNone(self.model.orientations[0].transmission)

        self._click_checkbox(self.view.chkTransmission)

        self.assertTrue(self.view.chkTransmission.isChecked())
        self.assertTrue(self.model.plot_transmission)
        # turning transmission on runs the real absorption calc and caches factors on the orientation
        self.assertIsNotNone(self.model.orientations[0].transmission)


class TestGaugeVolume(_FunctionalTestBase):
    def setUp(self):
        super().setUp()
        self._show_experiment_tab()
        # gauge-volume controls live in a collapsed, checkable group box
        self.view.grpGaugeVol.setChecked(True)
        QApplication.processEvents()

    def test_selecting_custom_shape_reveals_the_file_finder(self):
        self.view.combo_shapeMethod.setCurrentText("Custom Shape")
        QApplication.processEvents()
        self.assertTrue(self.view.finder_gauge_vol.isVisible())

        self.view.combo_shapeMethod.setCurrentText("4mmCube")
        QApplication.processEvents()
        self.assertFalse(self.view.finder_gauge_vol.isVisible())

    def test_set_then_clear_gauge_volume_round_trips(self):
        self.view.combo_shapeMethod.setCurrentText("4mmCube")

        self._click(self.view.setGV)
        self.assertIsNotNone(self.model.workspaces.gauge_volume_str)

        self._click(self.view.clearGV)
        self.assertFalse(self.model.workspaces.gauge_volume_str)


class TestInstrumentSelection(_FunctionalTestBase):
    def test_selecting_a_preset_instrument_repopulates_groups_without_applying(self):
        self.view.cmbInstr.setCurrentText("IMAT")

        items = [self.view.cmbGroup.itemText(i) for i in range(self.view.cmbGroup.count())]
        self.assertEqual(items, list(self.model.instrument.groups_for_instrument("IMAT")))
        self.assertTrue(self.view.cmbGroup.isEnabled())
        # nothing is applied to the model until Update Instrument is pressed
        self.assertEqual(self.model.instrument.get_instrument(), "ENGINX")

    def test_selecting_custom_instrument_reveals_name_field_and_locks_group(self):
        self._show_experiment_tab()

        self.view.cmbInstr.setCurrentText(CUSTOM_INSTRUMENT)
        QApplication.processEvents()

        self.assertTrue(self.view.edt_custom_instr.isVisible())
        self.assertFalse(self.view.cmbGroup.isEnabled())
        groups = [self.view.cmbGroup.itemText(i) for i in range(self.view.cmbGroup.count())]
        self.assertEqual(groups, ["Custom"])
        self.assertTrue(self.view.finder_grouping.isVisible())

    def test_invalid_custom_name_flags_field_and_disables_update(self):
        self._show_experiment_tab()
        self.view.cmbInstr.setCurrentText(CUSTOM_INSTRUMENT)

        QTest.keyClicks(self.view.edt_custom_instr, "NOTREAL")

        self.assertIn("red", self.view.edt_custom_instr.styleSheet())
        self.assertFalse(self.view.btnUpdateInstr.isEnabled())

    def test_update_instrument_button_applies_group_to_model(self):
        self._show_experiment_tab()
        self.view.cmbGroup.setCurrentText("banks")
        self.assertTrue(self.view.btnUpdateInstr.isEnabled())

        self._click(self.view.btnUpdateInstr)

        # a preset group is stored as the instrument config's enum, whose value is the group name
        self.assertEqual(self.model.instrument.group.value, "banks")

    def test_applying_instrument_settings_preserves_initial_shape_rotation(self):
        # Update Instrument rebuilds every sample workspace on the (re)selected instrument before
        # regrouping - the same path a genuine instrument change takes. Both steps used to drop the
        # baked-in initial rotation for the default CSG cube. ENGINX + banks is used so the grouping
        # file is guaranteed present in the test environment (see the test above).
        self._show_experiment_tab()
        # bake a non-trivial initial rotation into the sample
        self.view.spnInitX.setValue(30.0)
        QApplication.processEvents()
        rotated_data = self._aabb_extent(self.model.workspaces.ws)
        rotated_neutral = self._aabb_extent(self.model.workspaces.updated_mesh_ws)
        # sanity: rotating the default cube makes its axis-aligned bounding box non-cubic
        self.assertFalse(np.allclose(rotated_data, rotated_data[0]))

        self.view.cmbGroup.setCurrentText("banks")
        QApplication.processEvents()
        self._click(self.view.btnUpdateInstr)

        # the initial rotation must survive the rebuild + regroup on both the data ws (used for the
        # direction arrows) and the neutral mesh ws (used to draw the sample itself)
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), rotated_data, atol=1e-9)
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.updated_mesh_ws), rotated_neutral, atol=1e-9)


class TestExports(_FunctionalTestBase):
    def _enable_outputs(self):
        self.view.get_save_dir = lambda: self._tmpdir
        QTest.keyClicks(self.view.saveFileLine, "run")

    def _export_as(self, fmt):
        self.view.cmbExportFormat.setCurrentText(fmt)
        self._click(self.view.btnExport)

    def test_export_button_enables_once_dir_and_filename_present(self):
        self.assertFalse(self.view.btnExport.isEnabled())
        self._enable_outputs()
        self.assertTrue(self.view.btnExport.isEnabled())

    def test_selected_format_writes_matching_file(self):
        self._enable_outputs()

        self._export_as(EXPORT_SSCANSS)
        self.assertTrue(os.path.exists(os.path.join(self._tmpdir, "run.angles")))

        self._export_as(EXPORT_MATRIX)
        self.assertTrue(os.path.exists(os.path.join(self._tmpdir, "run.txt")))

        self._export_as(EXPORT_REFERENCE_WS)
        self.assertTrue(os.path.exists(os.path.join(self._tmpdir, "run.nxs")))

    def test_transmission_weighting_option_tracks_estimate_toggle(self):
        def combo_items():
            return [self.view.cmbExportFormat.itemText(i) for i in range(self.view.cmbExportFormat.count())]

        self.assertNotIn(EXPORT_TRANSMISSION_WEIGHTING, combo_items())

        self._click_checkbox(self.view.chkTransmission)
        self.assertIn(EXPORT_TRANSMISSION_WEIGHTING, combo_items())

        self._click_checkbox(self.view.chkTransmission)
        self.assertNotIn(EXPORT_TRANSMISSION_WEIGHTING, combo_items())

    def test_transmission_weighting_export_writes_file(self):
        self._enable_outputs()
        self._click_checkbox(self.view.chkTransmission)
        # seed deterministic, positive factors so the write does not depend on the absorption calc
        # landing the (unloaded) sample inside a gauge volume; the real exporter still does the work
        self.model.orientations[0].transmission = np.array([0.4, 0.9])

        self._export_as(EXPORT_TRANSMISSION_WEIGHTING)

        out_file = os.path.join(self._tmpdir, "run_transmission_weighting.txt")
        self.assertTrue(os.path.exists(out_file))
        with open(out_file) as f:
            # one orientation, normalised against itself -> weight of 1.0
            self.assertEqual(f.read().splitlines(), ["1.0"])


class TestMaterialAndSettings(_FunctionalTestBase):
    def test_set_material_button_opens_preset_dialog_and_sets_material(self):
        self.view.grpSetMaterial.setChecked(True)  # reveal the (collapsed) material controls
        QApplication.processEvents()

        with mock.patch(f"{PRESENTER}.InterfaceManager") as mock_mgr:
            dialog = mock_mgr.return_value.createDialogFromName.return_value

            # emulate the modal SetSampleMaterial dialog: opening it runs a real SetSampleMaterial
            # against the preset raw mesh workspace, then notifies the algorithm observer the dialog
            # was handed - exactly the path the real dialog drives when the user accepts it.
            def run_dialog():
                SetSampleMaterial(InputWorkspace=self.model.workspaces.WS_MESH_RAW, ChemicalFormula="Cu")
                observer = dialog.addAlgorithmObserver.call_args.args[0]
                observer.finishHandle()

            dialog.show.side_effect = run_dialog

            self._click(self.view.btnSetMaterial)

        # the dialog is opened against the hidden raw mesh ws, with InputWorkspace locked
        mock_mgr.return_value.createDialogFromName.assert_called_once_with(
            "SetSampleMaterial", -1, self.view, False, {"InputWorkspace": self.model.workspaces.WS_MESH_RAW}, "", (), ("InputWorkspace",)
        )
        # the real material set on the raw ws is propagated to the other workspaces and shown
        self.assertEqual(self.model.workspaces.get_material_name(), "Cu")
        self.assertEqual(self.view.lblCurrentMaterialValue.text(), "Cu")

    def test_settings_button_shows_settings_presenter(self):
        self._click(self.view.btn_settings)
        self.mock_TexturePlannerSettingsPresenter.return_value.show.assert_called_once_with()


class TestWindowClose(_FunctionalTestBase):
    def test_closing_window_removes_this_instances_workspaces(self):
        wsm = self.model.workspaces
        owned = [wsname for wsname in wsm._owned_ws_names]
        # the model bootstraps the persistent planner workspaces on construction
        self.assertTrue(ADS.doesExist(wsm.wsname))

        self.view.close()
        QApplication.processEvents()

        for name in owned:
            self.assertFalse(ADS.doesExist(name), f"{name} should have been removed on close")

    def test_two_windows_use_distinct_workspaces(self):
        # a second planner can be open at the same time without clobbering the first's workspaces
        other_model = TexturePlannerModel()
        self.addCleanup(other_model.workspaces.cleanup)

        self.assertNotEqual(self.model.workspaces.wsname, other_model.workspaces.wsname)
        self.assertTrue(ADS.doesExist(self.model.workspaces.wsname))
        self.assertTrue(ADS.doesExist(other_model.workspaces.wsname))


class TestStlLoadSettings(_FunctionalTestBase):
    """Changing the STL-loading settings (scale / initial rotation / translation) through the real
    settings presenter must change how a subsequently loaded mesh is realised."""

    def test_mm_scale_applied_through_settings_shrinks_loaded_mesh(self):
        self._apply_settings_via_real_presenter(stl_scale="mm")
        self.assertEqual(self.model.workspaces.stl_kwargs["Scale"], "mm")

        self._load_stl()

        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), (0.001, 0.001, 0.001), atol=1e-9)

    def test_rotation_and_translation_settings_transform_loaded_mesh(self):
        # LoadSampleShape rotates X,Y,Z first and then translates, with the TranslationVector
        # expressed in the same units as the mesh (the Scale setting). Rotating the unit
        # tetrahedron 90 deg about Z swings its +x vertex onto +y and its +y vertex onto -x
        # (AABB x-range becomes [-1, 0] cm), and the 1 cm x-translation shifts that back to [0, 1].
        self._apply_settings_via_real_presenter(stl_z_degrees=90.0, stl_translation_vector="1,0,0")

        self._load_stl()

        verts = self.model.workspaces.ws.sample().getShape().getMesh().reshape(-1, 3)
        np.testing.assert_allclose(verts.min(axis=0), (0.0, 0.0, 0.0), atol=1e-9)
        np.testing.assert_allclose(verts.max(axis=0), (0.01, 0.01, 0.01), atol=1e-9)


class TestMaterialOnShapes(_FunctionalTestBase):
    """Set and update the sample material on both shape types."""

    def _assert_material_shown(self, name):
        self.assertEqual(self.model.workspaces.get_material_name(), name)
        self.assertEqual(self.view.lblCurrentMaterialValue.text(), name)

    def test_set_then_update_material_on_stl_mesh(self):
        self._load_stl()

        self._set_material_via_dialog(ChemicalFormula="Cu")
        self._assert_material_shown("Cu")
        # the mesh shape must survive the material round-trip
        self.assertEqual(type(self.model.workspaces.ws.sample().getShape()).__name__, "MeshObject")

        self._set_material_via_dialog(ChemicalFormula="Al")
        self._assert_material_shown("Al")

    def test_set_then_update_material_on_csg_shape(self):
        self._load_csg_cube(0.02)

        self._set_material_via_dialog(ChemicalFormula="Cu")
        self._assert_material_shown("Cu")
        self.assertAlmostEqual(abs(self.model.workspaces.ws.sample().getShape().volume()), 0.02**3, places=9)

        self._set_material_via_dialog(ChemicalFormula="Al")
        self._assert_material_shown("Al")

    def test_material_update_recomputes_active_transmission_estimates(self):
        # 2 detector groups keep the (real) MonteCarloAbsorption runs quick
        self._apply_instrument_group("banks")
        self._click_checkbox(self.view.chkTransmission)
        t_fe = np.array(self.model.orientations[0].transmission)
        self.assertEqual(len(t_fe), 2)

        self._set_material_via_dialog(ChemicalFormula="V", SampleNumberDensity=0.0722)

        # the estimate is redone for the new material: still cached, valid, and (V attenuating
        # very differently from Fe) with clearly different factors
        t_v = np.array(self.model.orientations[0].transmission)
        self.assertEqual(len(t_v), 2)
        self.assertTrue(np.all((t_v > 0) & (t_v <= 1)))
        self.assertFalse(np.allclose(t_fe, t_v, rtol=1e-3))


class TestInitialShapeOnLoadedShapes(_FunctionalTestBase):
    """Initial orientation / translation applied to both shape types."""

    def test_initial_orientation_rotates_csg_bounds(self):
        self.view.spnInitX.setValue(45.0)
        QApplication.processEvents()

        expected = (0.01, 0.01 * np.sqrt(2), 0.01 * np.sqrt(2))
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), expected, atol=1e-6)
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.updated_mesh_ws), expected, atol=1e-6)

    def test_initial_orientation_rotates_stl_bounds(self):
        self._load_stl()

        self.view.spnInitZ.setValue(45.0)
        QApplication.processEvents()

        # unit tetrahedron rotated 45 deg about Z (about the origin): x extent grows to sqrt(2) cm,
        # y extent shrinks to sqrt(2)/2 cm, z extent is unchanged
        expected = (0.01 * np.sqrt(2), 0.01 * np.sqrt(2) / 2, 0.01)
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), expected, atol=1e-6)

    def test_translation_moves_bounds_and_scattering_elements(self):
        detqs_at_origin = self.model.geometry.detQs_lab.copy()

        self.view.spnInitPX.setValue(0.002)
        self.view.spnInitPY.setValue(0.003)
        QApplication.processEvents()

        # shape: same cube, shifted wholesale
        ws = self.model.workspaces.ws
        np.testing.assert_allclose(self._aabb_extent(ws), (0.01, 0.01, 0.01), atol=1e-9)
        np.testing.assert_allclose(self._aabb_centre(ws), (0.002, 0.003, 0.0), atol=1e-6)
        # the scattering centre follows the (whole-shape) centre of mass
        np.testing.assert_allclose(self.model.workspaces.scattering_centre, (0.002, 0.003, 0.0), atol=1e-4)
        # and the Q vectors are recomputed against it (unit length, measurably different from the
        # origin-centred set for a mm-scale shift against m-scale detector distances)
        detqs = self.model.geometry.detQs_lab
        np.testing.assert_allclose(np.linalg.norm(detqs, axis=1), 1.0, atol=1e-12)
        self.assertFalse(np.allclose(detqs_at_origin, detqs))
        # the lab-view Q-vector bundle is rooted on the new scattering centre
        np.testing.assert_allclose(self._lab_q_tip_positions(), self._expected_q_tip_positions(), atol=1e-9)

    def test_translation_after_gauge_volume_scatters_from_illuminated_volume(self):
        self._set_gauge_volume("4mmCube")
        gauge_str = self.model.workspaces.gauge_volume_str

        # small enough that the 4 mm gauge cube still intersects the 1 cm sample
        self.view.spnInitPX.setValue(0.004)
        QApplication.processEvents()

        # shape moved...
        ws = self.model.workspaces.ws
        np.testing.assert_allclose(self._aabb_extent(ws), (0.01, 0.01, 0.01), atol=1e-9)
        np.testing.assert_allclose(self._aabb_centre(ws), (0.004, 0.0, 0.0), atol=1e-6)
        # ...but the gauge volume stays put at the origin (the log normalises whitespace)
        self.assertEqual(ws.run().getProperty("GaugeVolume").value.strip(), gauge_str.strip())
        self.assertIn("x='0.0' y='0.0' z='0.0'", gauge_str)
        # the scattering centre is the c.o.m. of the illuminated (gauge AND sample) region: well
        # short of the sample centre (0.004), just above the gauge centre (0)
        centre = self.model.workspaces.scattering_centre
        self.assertGreaterEqual(centre[0], 0.0)
        self.assertLess(centre[0], 0.002)
        np.testing.assert_allclose(centre[1:], (0.0, 0.0), atol=5e-4)
        # and the displayed scattering elements are rooted there
        np.testing.assert_allclose(self._lab_q_tip_positions(), self._expected_q_tip_positions(), atol=1e-9)


class TestSampleDirectionsDisplay(_FunctionalTestBase):
    """Sample-direction arrows and labels in the lab view and pole figure."""

    def test_direction_labels_scale_with_shape_and_appear_in_both_views(self):
        self._load_csg_cube(0.02)

        self.view.grpDirectionWidgets.setChecked(True)
        QApplication.processEvents()
        self.view.set_rd_name("AD")
        self.view.set_nd_name("BD")
        self.view.set_td_name("CD")
        self._click(self.view.updateDirs)
        QApplication.processEvents()

        # lab view: one label per direction, placed at the arrow tip 1.2 x the half-extent of the
        # shape from its centre (the 0.02 cube => 0.012 along each axis)
        labels = self._lab_direction_labels()
        self.assertEqual(set(labels), {"AD", "BD", "CD"})
        np.testing.assert_allclose(labels["AD"], (0.012, 0.0, 0.0), atol=1e-6)
        np.testing.assert_allclose(labels["BD"], (0.0, 0.012, 0.0), atol=1e-6)
        np.testing.assert_allclose(labels["CD"], (0.0, 0.0, 0.012), atol=1e-6)

        # pole figure: rim circle, the in-plane direction labels, and the coverage points
        pf_ax = self.view.get_pf_ax()
        self.assertEqual([type(p).__name__ for p in pf_ax.patches], ["Circle"])
        self.assertEqual([t.get_text() for t in pf_ax.texts], ["AD", "CD"])
        pf_points = self.model.orientations[0].pf_points
        self.assertEqual(len(pf_points), 20)  # ENGINX Texture20 default
        self.assertEqual(len(self._pf_scatters_matching(pf_points)), 1)


class TestInstrumentGeometryCounts(_FunctionalTestBase):
    """Detector / Q / pole-figure point counts per instrument grouping."""

    def _assert_group_count(self, n):
        self.assertEqual(self.model.geometry.det_k.shape, (n, 3))
        self.assertEqual(self.model.geometry.detQs_lab.shape, (n, 3))
        self.assertEqual(self.model.orientations[0].pf_points.shape, (n, 2))
        self.assertEqual(len(self._pf_scatters_matching(self.model.orientations[0].pf_points)), 1)

    def test_enginx_group_counts(self):
        self._assert_group_count(20)  # Texture20 is the startup default

        self._apply_instrument_group("Texture30")
        self._assert_group_count(30)

        self._apply_instrument_group("banks")
        self._assert_group_count(2)

    def test_imat_group_counts(self):
        self._apply_instrument_group("banks", instrument="IMAT")
        self.assertEqual(self.model.instrument.get_instrument(), "IMAT")
        self._assert_group_count(2)

        self._apply_instrument_group("Module1")
        self._assert_group_count(14)

    def test_custom_instrument_with_generated_grouping_file(self):
        # build a two-group grouping file for POLDI at runtime (group IDs must start at 1: the
        # planner treats group 0 as the null group)
        sim = CreateSimulationWorkspace(Instrument="POLDI", BinParams="0,1,2", OutputWorkspace="__texplan_test_poldi_sim")
        grouping_ws = CreateGroupingWorkspace(InputWorkspace=sim, FixedGroupCount=2, OutputWorkspace="__texplan_test_poldi_grp")[0]
        n_hist = grouping_ws.getNumberHistograms()
        for i in range(n_hist):
            grouping_ws.dataY(i)[0] = 1.0 if i < n_hist // 2 else 2.0
        grouping_path = os.path.join(self._tmpdir, "poldi_grouping.xml")
        SaveDetectorsGrouping(InputWorkspace=grouping_ws, OutputFile=grouping_path)

        # drive the real custom-instrument UI path
        self._show_experiment_tab()
        self.view.cmbInstr.setCurrentText(CUSTOM_INSTRUMENT)
        QApplication.processEvents()
        QTest.keyClicks(self.view.edt_custom_instr, "POLDI")
        self.view.edt_custom_instr.editingFinished.emit()
        QApplication.processEvents()
        self.assertNotIn("red", self.view.edt_custom_instr.styleSheet())
        # a valid name alone is not enough: the grouping file must be supplied and applicable
        self.assertFalse(self.view.btnUpdateInstr.isEnabled())

        self.view.get_grouping_file = lambda: grouping_path
        self.view.finder_grouping.fileFindingFinished.emit()
        QApplication.processEvents()
        self.assertTrue(self.view.btnUpdateInstr.isEnabled())

        self._click(self.view.btnUpdateInstr)
        QApplication.processEvents()

        self.assertEqual(self.model.instrument.get_instrument(), "POLDI")
        self._assert_group_count(2)


class TestGaugeVolumeScattering(_FunctionalTestBase):
    """Gauge volumes move the scattering centre; custom gauge shapes load from file."""

    def test_setting_gauge_volume_writes_log_and_moves_scattering_centre(self):
        # shift the sample so the whole-shape c.o.m. is clearly away from the gauge centre
        self.view.spnInitPX.setValue(0.004)
        QApplication.processEvents()
        np.testing.assert_allclose(self.model.workspaces.scattering_centre, (0.004, 0.0, 0.0), atol=1e-4)
        detqs_without_gauge = self.model.geometry.detQs_lab.copy()

        self._set_gauge_volume("4mmCube")

        self.assertTrue(self.model.workspaces.ws.run().hasProperty("GaugeVolume"))
        # the centre snaps from the sample centre towards the (origin-centred) gauge region
        centre = self.model.workspaces.scattering_centre
        self.assertLess(centre[0], 0.002)
        self.assertGreaterEqual(centre[0], 0.0)
        # and the detector Q vectors follow the new centre
        self.assertFalse(np.allclose(detqs_without_gauge, self.model.geometry.detQs_lab))

    def test_custom_gauge_volume_loaded_from_xml_file(self):
        gauge_xml = get_cube_xml("custom_gv", 0.002)
        gauge_path = self._write("gauge.xml", gauge_xml)

        self._set_gauge_volume(GAUGE_VOL_CUSTOM_SHAPE, custom_file=gauge_path)

        self.assertEqual(self.model.workspaces.gauge_volume_str, gauge_xml)
        # the log normalises whitespace, so compare stripped
        self.assertEqual(self.model.workspaces.ws.run().getProperty("GaugeVolume").value.strip(), gauge_xml.strip())


class TestOrientationFileContents(_FunctionalTestBase):
    """Orientation-file loading: euler, matrix, and custom euler conventions."""

    def _load_orientation_file(self, content):
        path = self._write("orient.txt", content)
        self.view.get_orientation_file = lambda: path
        self.view.set_load_orientation_enabled(True)
        self._click(self.view.btnOrient)
        QApplication.processEvents()

    def test_euler_file_populates_table_and_rotations(self):
        self._load_orientation_file("30,45,60\n")

        # one row appended after the initial orientation; YXY -> three goniometer axes
        self.assertEqual(self.model.orientations.get_num_orientations(), 2)
        self.assertEqual(self.view.get_num_gonios(), 3)
        # default convention: YXY axes, all senses -1
        expected_R = (
            Rotation.from_euler("y", -30, degrees=True)
            * Rotation.from_euler("x", -45, degrees=True)
            * Rotation.from_euler("y", -60, degrees=True)
        )
        self.assertLess((self.model.orientations[1].R * expected_R.inv()).magnitude(), 1e-10)
        # table strings carry the file angles on the convention's axes, exactly as loaded
        self.assertEqual(self.view.tableWidget.item(1, 0).text(), "30.0,0,1,0,-1")
        self.assertEqual(self.view.tableWidget.item(1, 1).text(), "45.0,1,0,0,-1")
        self.assertEqual(self.view.tableWidget.item(1, 2).text(), "60.0,0,1,0,-1")
        # the shape is untouched and the sample directions are still displayed in both views
        np.testing.assert_allclose(self._aabb_extent(self.model.workspaces.ws), (0.01, 0.01, 0.01), atol=1e-9)
        self.assertEqual(set(self._lab_direction_labels()), {"RD", "ND", "TD"})
        self.assertEqual([t.get_text() for t in self.view.get_pf_ax().texts], ["RD", "TD"])

    def test_matrix_file_preserves_exact_rotation(self):
        exact_Rs = [Rotation.from_euler("XYZ", angles, degrees=True) for angles in ((10, 20, 30), (25, 15, 5))]
        rows = "".join(",".join(str(x) for x in R.as_matrix().reshape(-1)) + "\n" for R in exact_Rs)
        self._load_orientation_file(rows)

        self.assertEqual(self.model.orientations.get_num_orientations(), 3)
        self.assertEqual(self.view.get_num_gonios(), 3)
        # every matrix row must keep the exact R (the YXY euler decomposition is display-only),
        # including the last row, which becomes the displayed orientation on load
        np.testing.assert_allclose(self.model.orientations[1].R.as_matrix(), exact_Rs[0].as_matrix(), atol=1e-10)
        np.testing.assert_allclose(self.model.orientations[2].R.as_matrix(), exact_Rs[1].as_matrix(), atol=1e-10)

    def test_euler_file_with_custom_axes_and_senses_from_settings(self):
        self._apply_settings_via_real_presenter(orientation_axes="XYZ", orientation_senses="1,1,1")
        self.assertEqual(self.model.orientations.orientation_kwargs, {"Axes": "XYZ", "Senses": "1,1,1"})

        self._load_orientation_file("90,0,0\n")

        self.assertEqual(self.model.orientations.get_num_orientations(), 2)
        expected_R = Rotation.from_euler("x", 90, degrees=True)
        self.assertLess((self.model.orientations[1].R * expected_R.inv()).magnitude(), 1e-10)


class TestGoniometerDisplay(_FunctionalTestBase):
    """Goniometer rings, axis amendments, and pole-figure axis rendering."""

    def _pf_lines_of_color(self, color):
        return [ln for ln in self.view.get_pf_ax().lines if ln.get_color() == color]

    def test_ring_count_tracks_number_of_goniometers(self):
        # each goniometer ring is two Line3D artists (swept arc + remainder), and rings are the
        # only Line3D artists on the lab axis
        self.assertEqual(len(self.view.get_lab_ax().lines), 4)

        self.view.spnNumAxes.setValue(4)
        QApplication.processEvents()
        self.assertEqual(len(self.view.get_lab_ax().lines), 8)

        self.view.spnNumAxes.setValue(2)
        QApplication.processEvents()
        self.assertEqual(len(self.view.get_lab_ax().lines), 4)

    def test_amending_axis_updates_rotation_table_and_both_views(self):
        self.view.edtVec0.setText("0,0,1")
        self.view.spnAngle0.setValue(90.0)
        QApplication.processEvents()

        # model: 90 deg clockwise (sense -1) about z
        expected_R = Rotation.from_euler("z", -90, degrees=True)
        self.assertLess((self.model.orientations[0].R * expected_R.inv()).magnitude(), 1e-10)
        # table row shows the amended axis
        self.assertEqual(self.view.tableWidget.item(0, 0).text(), "90.0,0.0,0.0,1.0,-1")
        # lab view: the axis-0 quiver now points along +z
        np.testing.assert_allclose(self._lab_axis_quiver_direction("Axis 0"), (0.0, 0.0, 1.0), atol=1e-8)
        # pole figure: the (equatorial) axis pole is the line through the centre at the new azimuth
        (line,) = self._pf_lines_of_color("hotpink")
        np.testing.assert_allclose(line.get_xdata(), (1.0, -1.0), atol=1e-10)
        np.testing.assert_allclose(line.get_ydata(), (0.0, 0.0), atol=1e-10)

    def test_equatorial_axis_is_line_and_polar_axis_is_point(self):
        # the default axis 0 (1,0,0) lies on the pole-figure equator -> drawn as a diameter line
        (line,) = self._pf_lines_of_color("hotpink")
        np.testing.assert_allclose(line.get_xdata(), (0.0, 0.0), atol=1e-10)
        np.testing.assert_allclose(line.get_ydata(), (1.0, -1.0), atol=1e-10)

        # an axis along the pole-figure pole (0,1,0) collapses to a point at the centre
        self.view.edtVec0.setText("0,1,0")
        self.view.edtVec0.editingFinished.emit()
        QApplication.processEvents()

        self.assertEqual(self._pf_lines_of_color("hotpink"), [])
        hotpink = to_rgba("hotpink")
        centre_points = [
            c
            for c in self._pf_point_scatters()
            if len(c.get_offsets()) == 1
            and np.allclose(np.asarray(c.get_offsets())[0], (0.0, 0.0), atol=1e-10)
            and np.allclose(c.get_edgecolor()[0], hotpink)
        ]
        self.assertEqual(len(centre_points), 1)
        # axis 0 is the selected goniometer, so its pole is drawn filled
        np.testing.assert_allclose(centre_points[0].get_facecolor()[0], hotpink)


class TestOrientationCycling(_FunctionalTestBase):
    """Cycling / clamping the current orientation and its pole-figure highlighting."""

    def _add_rotated_orientation(self, angle):
        self._click(self.view.addOrientation)
        self.view.spnAngle0.setValue(angle)
        QApplication.processEvents()

    def _filled_state(self, pf_points):
        (scatter,) = self._pf_scatters_matching(pf_points)
        return len(scatter.get_facecolor()) > 0

    def test_cycling_index_applies_orientation_rotation_to_display(self):
        self._add_rotated_orientation(45.0)  # orientation 1: 45 deg CW about x
        neutral_verts = self.model.workspaces.updated_mesh_ws.sample().getShape().getMesh().reshape(-1, 3)

        for spn_value, orientation_index in ((1, 0), (2, 1)):
            self.view.spnIndex.setValue(spn_value)
            QApplication.processEvents()

            expected_R = self.model.orientations[orientation_index].R
            # the run goniometer of the data ws carries the displayed orientation's R
            np.testing.assert_allclose(self.model.workspaces.ws.run().getGoniometer().getR(), expected_R.as_matrix(), atol=1e-10)
            # and the drawn sample mesh is the neutral mesh rotated by that R
            expected_verts = expected_R.apply(neutral_verts)
            drawn = self._drawn_sample_vertices()
            np.testing.assert_allclose(drawn.min(axis=0), expected_verts.min(axis=0), atol=1e-9)
            np.testing.assert_allclose(drawn.max(axis=0), expected_verts.max(axis=0), atol=1e-9)

    def test_index_spinbox_clamps_to_orientation_count(self):
        self._add_rotated_orientation(45.0)
        self.assertEqual(self.view.spnIndex.maximum(), 2)

        self.view.spnIndex.setValue(0)
        self.assertEqual(self.view.spnIndex.value(), 1)

        self.view.spnIndex.setValue(99)
        self.assertEqual(self.view.spnIndex.value(), 2)

    def test_changing_index_swaps_which_scatter_is_filled(self):
        self._add_rotated_orientation(45.0)
        pf0 = self.model.orientations[0].pf_points
        pf1 = self.model.orientations[1].pf_points
        self.assertFalse(np.allclose(pf0, pf1))

        # orientation 1 is current: drawn filled, orientation 0 as edges only
        self.assertTrue(self._filled_state(pf1))
        self.assertFalse(self._filled_state(pf0))

        self.view.spnIndex.setValue(1)
        QApplication.processEvents()

        self.assertTrue(self._filled_state(pf0))
        self.assertFalse(self._filled_state(pf1))

    def test_include_toggles_update_pole_figure(self):
        self._add_rotated_orientation(45.0)
        pf0 = self.model.orientations[0].pf_points
        pf1 = self.model.orientations[1].pf_points

        # exclude the non-current row 0: its points disappear entirely
        self._click_checkbox(self._checkbox(0, 6))
        QApplication.processEvents()
        self.assertEqual(self._pf_scatters_matching(pf0), [])
        self.assertTrue(self._filled_state(pf1))

        # exclude the current row 1 as well: it is kept visible but greyed out
        self._click_checkbox(self._checkbox(1, 6))
        QApplication.processEvents()
        (scatter,) = self._pf_scatters_matching(pf1)
        self.assertEqual(len(scatter.get_facecolor()), 0)
        np.testing.assert_allclose(scatter.get_edgecolor()[0][:3], to_rgba("grey")[:3])
        self.assertEqual(scatter.get_alpha(), 0.5)


class TestTransmissionValues(_FunctionalTestBase):
    """Transmission estimates are physical and drive the pole-figure colour scale."""

    def setUp(self):
        super().setUp()
        # 2 detector groups keep the (real) MonteCarloAbsorption runs quick
        self._apply_instrument_group("banks")

    def _transmission_scatter(self):
        scatters = [c for c in self._pf_point_scatters() if c.get_array() is not None]
        self.assertEqual(len(scatters), 1)
        return scatters[0]

    def test_transmission_estimates_are_sensible(self):
        self._click_checkbox(self.view.chkTransmission)

        transmission = np.array(self.model.orientations[0].transmission)
        self.assertEqual(len(transmission), len(self.model.geometry.spec_inds))
        self.assertTrue(np.all((transmission > 0) & (transmission <= 1)))
        # the default cube is centred on the beam, so the two (mirrored) ENGINX banks should see
        # roughly the same attenuation (loose bound: MonteCarlo with 50 events per point)
        self.assertLess(abs(transmission[0] - transmission[1]), 0.15 * transmission.mean())

    def test_colourbar_limit_setting_switches_scale_to_data_range(self):
        self._click_checkbox(self.view.chkTransmission)

        scatter = self._transmission_scatter()
        self.assertEqual(scatter.get_cmap().name, "jet")
        self.assertEqual(scatter.get_clim(), (0.0, 1.0))

        self._apply_settings_via_real_presenter(att_use_data_range=True)

        transmission = np.array(self.model.orientations[0].transmission)
        scatter = self._transmission_scatter()
        self.assertNotEqual(scatter.get_clim(), (0.0, 1.0))
        np.testing.assert_allclose(scatter.get_clim(), (transmission.min(), transmission.max()), atol=1e-12)


class TestExportContents(_FunctionalTestBase):
    """Exports write only the included rows, with faithful values."""

    def setUp(self):
        super().setUp()
        # three orientations: row 0 identity, row 1 Rx(-30), row 2 Rx(-60); row 1 excluded
        self._click(self.view.addOrientation)
        self.view.spnAngle0.setValue(30.0)
        self._click(self.view.addOrientation)
        self.view.spnAngle0.setValue(60.0)
        QApplication.processEvents()
        self._click_checkbox(self._checkbox(1, 6))
        QApplication.processEvents()
        self.included_Rs = [self.model.orientations[0].R, self.model.orientations[2].R]

        self.view.get_save_dir = lambda: self._tmpdir
        QTest.keyClicks(self.view.saveFileLine, "run")

    def _export_and_read(self, fmt, filename):
        self.view.cmbExportFormat.setCurrentText(fmt)
        self._click(self.view.btnExport)
        with open(os.path.join(self._tmpdir, filename)) as f:
            return f.read().splitlines()

    def test_sscanss_export_contains_only_included_rows(self):
        lines = self._export_and_read(EXPORT_SSCANSS, "run.angles")

        self.assertEqual(lines[0], "xyz")
        self.assertEqual(len(lines), 3)  # header + the two included rows
        for line, expected_R in zip(lines[1:], self.included_Rs):
            angles = [float(x) for x in line.split("\t")]
            np.testing.assert_allclose(angles, convert_to_sscanss_frame(expected_R.as_matrix()), atol=0.011)

    def test_euler_export_round_trips_included_rotations(self):
        lines = self._export_and_read(EXPORT_EULER, "run.txt")

        self.assertEqual(len(lines), 2)
        senses = [float(s) for s in self.model.orientations.orientation_kwargs["Senses"].split(",")]
        for line, expected_R in zip(lines, self.included_Rs):
            file_angles = [float(x) for x in line.split("\t")]
            # undo the sense factors and rebuild the rotation on the convention's axes
            raw = [sense * angle for sense, angle in zip(senses, file_angles)]
            rebuilt_R = Rotation.from_euler(self.model.orientations.orientation_kwargs["Axes"], raw, degrees=True)
            self.assertLess((rebuilt_R * expected_R.inv()).magnitude(), 1e-3)

    def test_matrix_export_contains_exact_flattened_matrices(self):
        lines = self._export_and_read(EXPORT_MATRIX, "run.txt")

        self.assertEqual(len(lines), 2)
        for line, expected_R in zip(lines, self.included_Rs):
            values = [float(x) for x in line.split("\t")]
            np.testing.assert_allclose(values, expected_R.as_matrix().reshape(-1), atol=1e-12)

    def test_reference_workspace_round_trips_shape_and_material(self):
        # bake in an initial rotation so the round-trip proves the oriented shape survives
        self.view.spnInitX.setValue(30.0)
        QApplication.processEvents()
        expected_extent = self._aabb_extent(self.model.workspaces.updated_mesh_ws)

        self.view.cmbExportFormat.setCurrentText(EXPORT_REFERENCE_WS)
        self._click(self.view.btnExport)

        loaded = LoadNexus(Filename=os.path.join(self._tmpdir, "run.nxs"), OutputWorkspace="__texplan_test_ref_roundtrip")
        self.assertAlmostEqual(abs(loaded.sample().getShape().volume()), 0.01**3, places=9)
        self.assertEqual(loaded.sample().getMaterial().name(), "Fe")
        np.testing.assert_allclose(self._aabb_extent(loaded), expected_extent, atol=1e-9)


class TestPoleFigureReference(_FunctionalTestBase):
    """Hard-coded pole-figure regression references.

    The reference arrays below were captured from the implementation on 28/7/2026 (ENGINX,
    "banks" grouping). They are primarily to function as REGRESSION CHECKS,
    and failures should be considered in the context of their changes: a legitimate change
    to the projection / geometry algorithms may move them, in which case they should be
    re-captured and updated deliberately.
    """

    # ENGINX + banks, identity orientation, default directions, azimuthal projection
    _REF_BANKS_IDENTITY = np.array(
        [
            [-0.7074793697410617, 0.7067339962042226],
            [0.7074793697410614, 0.7067339962042228],
        ]
    )
    # as above with the single edit of goniometer axis 0 (1,0,0) set to 30 deg, sense Clockwise
    _REF_BANKS_X30_CLOCKWISE = np.array(
        [
            [0.5823836510064151, -0.5038276628930591],
            [-0.5823836510064151, -0.5038276628930591],
        ]
    )

    def setUp(self):
        super().setUp()
        self._apply_instrument_group("banks")

    def test_identity_orientation_matches_captured_reference(self):
        pf = self.model.orientations[0].pf_points

        # analytic anchor (loose): the beam is +z and the ENGINX banks sit near +/-90 deg 2theta,
        # so the identity-orientation azimuthal points are on the rim at ~(+/-0.707, +/-0.707),
        # mirrored in x
        np.testing.assert_allclose(np.linalg.norm(pf, axis=1), 1.0, atol=1e-3)
        np.testing.assert_allclose(np.abs(pf), [[0.7075, 0.7067]] * 2, atol=0.01)
        self.assertAlmostEqual(pf[0, 0], -pf[1, 0], places=6)

        # captured regression reference (see class docstring)
        np.testing.assert_allclose(pf, self._REF_BANKS_IDENTITY, atol=1e-10)

    def test_rotated_orientation_matches_captured_reference(self):
        self.view.spnAngle0.setValue(30.0)
        QApplication.processEvents()

        pf = self.model.orientations[0].pf_points
        # analytic anchor (loose): rotating about x keeps the two banks mirrored in x with a
        # common y, and pulls the points off the rim
        self.assertAlmostEqual(pf[0, 1], pf[1, 1], places=6)
        self.assertAlmostEqual(pf[0, 0], -pf[1, 0], places=6)
        self.assertTrue(np.all(np.linalg.norm(pf, axis=1) < 1.0))

        # captured regression reference (see class docstring)
        np.testing.assert_allclose(pf, self._REF_BANKS_X30_CLOCKWISE, atol=1e-10)


if __name__ == "__main__":
    unittest.main()
