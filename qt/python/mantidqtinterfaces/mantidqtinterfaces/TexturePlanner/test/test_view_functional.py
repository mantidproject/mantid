# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Functional (widget-level, end-to-end) tests for the Texture Planner interface.

These build the *real* ``TexturePlannerView`` wired to the *real* ``TexturePlannerPresenter``
and the *real* ``TexturePlannerModel``, then drive the genuine Qt widgets and assert the
observable result on the model (workspaces, orientation table, instrument config, ...).
Because nothing in the value path is mocked, the tests catch both broken signal/slot wiring
*and* incorrect behaviour - e.g. selecting IMAT really computes IMAT's grouping presets, so a
test cannot pass while asserting the wrong instrument's groups.

The only genuinely external boundary that is isolated is the modal ``SetSampleMaterial`` dialog
(``InterfaceManager``), patched where exercised. Everything else - including the transmission
path's real absorption calculation - runs for real. The settings sub-dialog is also patched out;
it is a separate interface with its own tests.

File-load tests use small, self-contained temporary fixtures (an ASCII STL, a CSG xml, an
orientation text file); no external test data is required. Each test gets a fresh model and
clears the ADS afterwards so the hidden ``__``-prefixed planner workspaces never leak.

Interaction style:
  * Push-buttons and check-boxes are driven with real ``QTest`` mouse clicks.
  * Spin-boxes / combo-boxes / line-edits are driven through their setters (which emit the
    same ``valueChanged`` / ``currentTextChanged`` / ``textEdited`` signals a user edit would).
"""

import os
import shutil
import sys
import tempfile
import unittest
from unittest import mock

import numpy as np

from qtpy.QtCore import Qt, QPoint
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QApplication, QStyle

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import SetSampleMaterial
from Engineering.common.instrument_config import SUPPORTED_INSTRUMENTS
from Engineering.common.xml_shapes import get_cube_xml

from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel
from mantidqtinterfaces.TexturePlanner.view import (
    TexturePlannerView,
    CUSTOM_INSTRUMENT,
    EXPORT_SSCANSS,
    EXPORT_MATRIX,
    EXPORT_REFERENCE_WS,
    EXPORT_TRANSMISSION_WEIGHTING,
)
from mantidqtinterfaces.TexturePlanner.presenter import TexturePlannerPresenter

# render off-screen so the test run never pops up real windows; must be set before the
# QApplication is constructed. setdefault leaves an externally-chosen platform untouched.
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

app = QApplication.instance() or QApplication(sys.argv)

PRESENTER = "mantidqtinterfaces.TexturePlanner.presenter"

# the real grouping presets (mirrors InstrumentHelper, with the trailing custom-file option)
ENGINX_GROUPS = ["Texture20", "Texture30", "banks", "Custom"]
IMAT_GROUPS = ["Module1", "Module4", "Row1", "Row4", "banks", "Custom"]

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
        # registered before view.close in addCleanup, so (LIFO) it runs *after* the window closes
        ADS.clear()
        shutil.rmtree(self._tmpdir, ignore_errors=True)

    # interaction helpers ------------------------------------------------
    def _click(self, widget):
        QTest.mouseClick(widget, Qt.LeftButton)

    def _click_checkbox(self, checkbox):
        # click the indicator at the left edge: a text-less checkbox stretched in its layout
        # only toggles there, and it is the reliable hot-spot for the table checkboxes too.
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
        self.assertEqual(items, ENGINX_GROUPS)

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

    def test_loading_a_csg_xml_sets_a_valid_shape(self):
        xml_path = self._write("shape.xml", get_cube_xml("test_cube", 0.02))
        self.view.get_xml_string = lambda: xml_path
        self.view.set_load_xml_enabled(True)

        self._click(self.view.btnXML)

        # the loaded cube has side 0.02 m; volume() is signed, so compare on magnitude
        volume = self.model.workspaces.ws.sample().getShape().volume()
        self.assertAlmostEqual(abs(volume), 0.02**3, places=9)

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
        self.assertEqual(items, IMAT_GROUPS)
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
    def test_set_material_button_opens_preset_dialog(self):
        self.view.grpSetMaterial.setChecked(True)  # reveal the (collapsed) material controls
        QApplication.processEvents()

        with mock.patch(f"{PRESENTER}.InterfaceManager") as mock_mgr:
            dialog = mock_mgr.return_value.createDialogFromName.return_value
            self._click(self.view.btnSetMaterial)

        mock_mgr.return_value.createDialogFromName.assert_called_once_with(
            "SetSampleMaterial", -1, self.view, False, {"InputWorkspace": self.model.workspaces.WS_MESH_RAW}, "", (), ("InputWorkspace",)
        )
        dialog.show.assert_called_once_with()

    def test_material_set_signal_propagates_new_material(self):
        # emulate the dialog: it writes the chosen material onto the raw mesh workspace only
        SetSampleMaterial(InputWorkspace=self.model.workspaces.mesh_ws, ChemicalFormula="Cu")

        self.view.signal_material_set()  # emitted on the GUI thread when the dialog finishes

        self.assertEqual(self.model.workspaces.get_material_name(), "Cu")
        self.assertEqual(self.view.lblCurrentMaterialValue.text(), "Cu")

    def test_settings_button_shows_settings_presenter(self):
        self._click(self.view.btn_settings)
        self.mock_TexturePlannerSettingsPresenter.return_value.show.assert_called_once_with()


class TestWindowClose(_FunctionalTestBase):
    def test_closing_window_removes_this_instances_workspaces(self):
        wsm = self.model.workspaces
        owned = [getattr(wsm, attr) for attr in wsm._OWNED_WS_NAME_ATTRS]
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


if __name__ == "__main__":
    unittest.main()
