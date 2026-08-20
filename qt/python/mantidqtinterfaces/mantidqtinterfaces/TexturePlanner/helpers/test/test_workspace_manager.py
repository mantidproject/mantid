# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
import numpy as np

from unittest.mock import patch, MagicMock, call
from scipy.spatial.transform import Rotation

from mantidqtinterfaces.TexturePlanner.helpers.workspace_manager import WorkspaceManager

file_path = "mantidqtinterfaces.TexturePlanner.helpers.workspace_manager"

COPY_KWARGS = dict(CopyName=False, CopyEnvironment=False, CopyLattice=False)
COPY_MATERIAL_KWARGS = dict(CopyName=False, CopyMaterial=True, CopyShape=False, CopyEnvironment=False, CopyLattice=False)


def _make_manager(instr="ENGINX"):
    model = MagicMock()
    model.instrument.get_instrument.return_value = instr
    return WorkspaceManager(model)


class TestWorkspaceManager_Init(unittest.TestCase):
    def test_default_attributes(self):
        wm = _make_manager("ENGINX")

        self.assertEqual(wm.wsname, wm.WS_DATA)
        self.assertEqual(wm.ungrouped_wsname, wm.WS_UNGROUPED)
        self.assertIsNone(wm.ws)
        self.assertIsNone(wm.ungrouped_ws)
        self.assertIsNone(wm.mesh_ws)
        self.assertIsNone(wm.updated_mesh_ws)
        self.assertIsNone(wm.material_ws)
        self.assertIsNone(wm.gauge_volume_str)
        self.assertEqual(wm.offset, (0, 0, 0))
        np.testing.assert_array_equal(wm.init_R.as_matrix(), np.eye(3))
        self.assertEqual(wm.attenuation_kwargs, {"point": 1.5, "unit": "dSpacing"})
        self.assertEqual(
            wm.stl_kwargs,
            {"Scale": "cm", "XDegrees": 0, "YDegrees": 0, "ZDegrees": 0, "TranslationVector": "0,0,0"},
        )

    def test_instr_property_reads_from_model(self):
        wm = _make_manager("IMAT")
        self.assertEqual(wm.instr, "IMAT")

    def test_workspace_names_carry_an_instance_unique_suffix(self):
        # every owned name gets an instance-unique suffix so several planner windows can be open at
        # once without colliding on the ADS
        wm = _make_manager()
        for name in wm._owned_ws_names:
            self.assertRegex(name, r"^__.+_[0-9a-f]{8}$")

    def test_two_managers_get_distinct_workspace_names(self):
        wm1 = _make_manager()
        wm2 = _make_manager()
        for name1, name2 in zip(wm1._owned_ws_names, wm2._owned_ws_names):
            self.assertNotEqual(name1, name2)

    @patch(file_path + ".get_scattering_centre")
    def test_scattering_centre_property_delegates_to_helper(self, mock_gsc):
        wm = _make_manager()
        wm.ws = MagicMock()
        mock_gsc.return_value = (1.0, 2.0, 3.0)

        self.assertEqual(wm.scattering_centre, (1.0, 2.0, 3.0))
        mock_gsc.assert_called_once_with(wm.ws)


@patch(file_path + ".ADS")
class TestWorkspaceManager_Cleanup(unittest.TestCase):
    def test_removes_every_owned_ws_that_exists(self, mock_ads):
        wm = _make_manager()
        mock_ads.doesExist.return_value = True

        wm.cleanup()

        removed = [c.args[0] for c in mock_ads.remove.call_args_list]
        self.assertEqual(removed, list(wm._owned_ws_names))

    def test_skips_owned_ws_that_do_not_exist(self, mock_ads):
        wm = _make_manager()
        mock_ads.doesExist.return_value = False

        wm.cleanup()

        mock_ads.remove.assert_not_called()


@patch(file_path + ".define_gauge_volume")
@patch(file_path + ".CloneWorkspace")
class TestWorkspaceManager_UpdateWorkspace(unittest.TestCase):
    def _make_manager_with_mock_model(self):
        wm = _make_manager("ENGINX")
        wm._init_wss = MagicMock()
        wm._update_existing_wss = MagicMock()
        wm.set_material = MagicMock()
        return wm

    def test_delegates_to_init_and_seeds_material_when_no_existing_ws(self, mock_clone, mock_define_gv):
        wm = self._make_manager_with_mock_model()
        wm.ws = None

        wm.update_ws()

        wm._init_wss.assert_called_once_with()
        wm._update_existing_wss.assert_not_called()
        # brand-new workspaces have no material, so the default is seeded
        wm.set_material.assert_called_once_with()

    def test_delegates_to_update_and_preserves_material_when_ws_exists(self, mock_clone, mock_define_gv):
        wm = self._make_manager_with_mock_model()
        wm.ws = MagicMock()

        wm.update_ws()

        wm._update_existing_wss.assert_called_once_with()
        wm._init_wss.assert_not_called()
        # _update_existing_wss copies the (possibly user-set) material onto the new workspaces, so
        # update_ws must not reset it
        wm.set_material.assert_not_called()

    def test_clones_ungrouped_from_ws(self, mock_clone, mock_define_gv):
        wm = self._make_manager_with_mock_model()
        wm.ws = MagicMock()
        mock_clone.return_value = "ungrouped"

        wm.update_ws()

        mock_clone.assert_called_once_with(InputWorkspace=wm.ws, OutputWorkspace=wm.WS_UNGROUPED)
        self.assertEqual(wm.ungrouped_ws, "ungrouped")

    def test_does_not_define_gauge_volume_when_unset(self, mock_clone, mock_define_gv):
        wm = self._make_manager_with_mock_model()
        wm.gauge_volume_str = None

        wm.update_ws()

        mock_define_gv.assert_not_called()

    def test_defines_gauge_volume_when_set(self, mock_clone, mock_define_gv):
        wm = self._make_manager_with_mock_model()
        wm.ws = MagicMock()
        wm.gauge_volume_str = "<xml/>"

        wm.update_ws()

        mock_define_gv.assert_called_once_with(wm.ws, "<xml/>")


@patch(file_path + ".SetSampleMaterial")
@patch(file_path + ".SetSampleShape")
@patch(file_path + ".CloneWorkspace")
@patch(file_path + ".CreateSimulationWorkspace")
@patch(file_path + ".CreateSampleWorkspace")
class TestWorkspaceManager_InitWss(unittest.TestCase):
    def test_creates_sim_workspace_with_expected_args(self, mock_create_shape, mock_create_sim, mock_clone, mock_set_shape, mock_set_mat):
        wm = _make_manager("ENGINX")
        sim_ws = MagicMock()
        mock_create_sim.return_value = sim_ws

        wm._init_wss()

        # only WS_DATA has its detectors read, so it is the only ws built on the real instrument
        mock_create_sim.assert_called_once_with(
            Instrument="ENGINX",
            BinParams=WorkspaceManager.DEFAULT_BIN_PARAMS,
            OutputWorkspace=wm.WS_DATA,
            UnitX="dSpacing",
        )
        self.assertIs(wm.ws, sim_ws)

    def test_builds_mesh_wss_as_stub_instrument_shape_wss(
        self, mock_create_shape, mock_create_sim, mock_clone, mock_set_shape, mock_set_mat
    ):
        # the mesh workspaces are pure sample holders; building them on the real instrument would
        # cost a full IDF load each for geometry that is never queried
        wm = _make_manager("ENGINX")
        mock_create_shape.side_effect = ["mesh", "neutral"]

        wm._init_wss()

        built = [c.kwargs["OutputWorkspace"] for c in mock_create_shape.call_args_list]
        self.assertEqual(built, [wm.WS_MESH_RAW, wm.WS_MESH_NEUTRAL])
        self.assertEqual(wm.mesh_ws, "mesh")
        self.assertEqual(wm.updated_mesh_ws, "neutral")

    def test_clones_material_holder_from_mesh_ws(self, mock_create_shape, mock_create_sim, mock_clone, mock_set_shape, mock_set_mat):
        wm = _make_manager("ENGINX")
        mock_create_shape.side_effect = ["mesh", "neutral"]
        mock_clone.return_value = "material"

        wm._init_wss()

        # material holder is cloned from the (cube-shaped) mesh ws so it owns a shape
        mock_clone.assert_called_once_with(InputWorkspace="mesh", OutputWorkspace=wm.WS_MATERIAL)
        self.assertEqual(wm.material_ws, "material")

    def test_seeds_material_holder_with_default(self, mock_create_shape, mock_create_sim, mock_clone, mock_set_shape, mock_set_mat):
        wm = _make_manager("ENGINX")
        mock_create_shape.side_effect = ["mesh", "neutral"]
        mock_clone.return_value = "material"

        wm._init_wss()

        mock_set_mat.assert_called_once_with("material", WorkspaceManager.DEFAULT_MATERIAL)

    def test_sets_default_cube_shape_on_all_three_wss(self, mock_create_shape, mock_create_sim, mock_clone, mock_set_shape, mock_set_mat):
        wm = _make_manager("ENGINX")
        sim_ws = MagicMock()
        mock_create_sim.return_value = sim_ws
        mock_create_shape.side_effect = ["mesh", "neutral"]

        wm._init_wss()

        targets = [c.args[0] for c in mock_set_shape.call_args_list]
        self.assertEqual(targets, [sim_ws, "mesh", "neutral"])
        # All three receive the same XML cube string
        xml_args = {c.args[1] for c in mock_set_shape.call_args_list}
        self.assertEqual(len(xml_args), 1)


class TestWorkspaceManager_UpdateExistingWss(unittest.TestCase):
    def test_rebuilds_only_the_data_ws(self):
        wm = _make_manager("ENGINX")
        wm.ws = "old_ws"
        wm.mesh_ws = "old_mesh"
        wm.updated_mesh_ws = "old_neutral"
        wm.material_ws = "old_material"
        wm._create_new_ws_with_copied_sample = MagicMock(return_value="new_ws")

        wm._update_existing_wss()

        # ws holds the initial shape rotation and must preserve it across the instrument switch
        wm._create_new_ws_with_copied_sample.assert_called_once_with(wm.WS_DATA, "old_ws", preserve_initial_rotation=True)
        self.assertEqual(wm.ws, "new_ws")

    def test_leaves_the_instrument_free_sample_holders_untouched(self):
        # the mesh and material workspaces carry a stub instrument, so an instrument switch has
        # nothing to rebuild on them - rebuilding would cost IDF loads for no behavioural gain
        wm = _make_manager("ENGINX")
        wm.ws = "old_ws"
        wm.mesh_ws = "old_mesh"
        wm.updated_mesh_ws = "old_neutral"
        wm.material_ws = "old_material"
        wm._create_new_ws_with_copied_sample = MagicMock(return_value="new_ws")

        wm._update_existing_wss()

        self.assertEqual(wm.mesh_ws, "old_mesh")
        self.assertEqual(wm.updated_mesh_ws, "old_neutral")
        self.assertEqual(wm.material_ws, "old_material")


@patch(file_path + ".ADS")
@patch(file_path + ".CopySample")
@patch(file_path + ".CreateSimulationWorkspace")
@patch(file_path + ".CreateSampleWorkspace")
class TestWorkspaceManager_CreateNewWsWithCopiedSample(unittest.TestCase):
    def test_stashes_sample_on_a_stub_shape_ws(self, mock_create_shape, mock_create_sim, mock_copy, mock_ads):
        # create_simulation_workspace is about to overwrite new_wsname in the ADS, so the sample is
        # parked on a throwaway ws first. That stash only holds a sample, so it needs no real
        # instrument - cloning the (potentially huge) source workspace would defeat the point.
        wm = _make_manager("ENGINX")
        sample = MagicMock(name="sample")
        mock_create_shape.return_value = "shape_tmp"
        mock_create_sim.return_value = "new_ws"
        mock_ads.doesExist.return_value = True

        wm._create_new_ws_with_copied_sample("dest_ws", sample)

        mock_create_shape.assert_called_once()
        self.assertEqual(mock_create_shape.call_args.kwargs["OutputWorkspace"], wm._SHAPE_TMP)
        self.assertEqual(mock_copy.call_args_list[0], call(InputWorkspace=sample, OutputWorkspace="shape_tmp", **COPY_KWARGS))

    def test_copies_from_shape_tmp_into_new_ws(self, mock_create_shape, mock_create_sim, mock_copy, mock_ads):
        wm = _make_manager("ENGINX")
        sample = MagicMock(name="sample")
        mock_create_shape.return_value = "shape_tmp"
        mock_create_sim.return_value = "new_ws"
        mock_ads.doesExist.return_value = True

        result = wm._create_new_ws_with_copied_sample("dest_ws", sample)

        mock_create_sim.assert_called_once_with(
            Instrument="ENGINX", BinParams=WorkspaceManager.DEFAULT_BIN_PARAMS, OutputWorkspace="dest_ws", UnitX="dSpacing"
        )
        self.assertEqual(
            mock_copy.call_args_list,
            [
                call(InputWorkspace=sample, OutputWorkspace="shape_tmp", **COPY_KWARGS),
                call(InputWorkspace="shape_tmp", OutputWorkspace="new_ws", **COPY_KWARGS),
            ],
        )
        self.assertEqual(result, "new_ws")

    def test_removes_shape_tmp_when_it_exists(self, mock_create_shape, mock_create_sim, mock_copy, mock_ads):
        wm = _make_manager("ENGINX")
        mock_create_shape.return_value = "shape_tmp"
        mock_ads.doesExist.return_value = True

        wm._create_new_ws_with_copied_sample("dest_ws", MagicMock())

        mock_ads.doesExist.assert_called_once_with(wm._SHAPE_TMP)
        mock_ads.remove.assert_called_once_with(wm._SHAPE_TMP)

    def test_skips_remove_if_shape_tmp_missing(self, mock_create_shape, mock_create_sim, mock_copy, mock_ads):
        wm = _make_manager("ENGINX")
        mock_ads.doesExist.return_value = False

        wm._create_new_ws_with_copied_sample("dest_ws", MagicMock())

        mock_ads.remove.assert_not_called()

    def test_removes_shape_tmp_when_the_copy_fails(self, mock_create_shape, mock_create_sim, mock_copy, mock_ads):
        # the stash is built inside the try, so a failure part-way through cannot leave it behind
        wm = _make_manager("ENGINX")
        mock_create_shape.return_value = "shape_tmp"
        mock_ads.doesExist.return_value = True
        mock_copy.side_effect = RuntimeError("CopySample failed")

        with self.assertRaises(RuntimeError):
            wm._create_new_ws_with_copied_sample("dest_ws", MagicMock())

        mock_ads.remove.assert_called_once_with(wm._SHAPE_TMP)

    def test_preserve_initial_rotation_used_for_both_hops(self, mock_create_shape, mock_create_sim, mock_copy, mock_ads):
        # both hops must use the same copy semantics: the preserving helper is idempotent, but
        # mixing it with a bare CopySample would re-bake init_R on one hop and strip it on the other
        wm = _make_manager("ENGINX")
        sample = MagicMock(name="sample")
        mock_create_shape.return_value = "shape_tmp"
        mock_create_sim.return_value = "new_ws"
        wm.copy_sample_preserving_initial_rotation = MagicMock()

        wm._create_new_ws_with_copied_sample("dest_ws", sample, preserve_initial_rotation=True)

        self.assertEqual(
            wm.copy_sample_preserving_initial_rotation.call_args_list,
            [call(sample, "shape_tmp"), call("shape_tmp", "new_ws")],
        )
        mock_copy.assert_not_called()


@patch(file_path + ".CopySample")
class TestWorkspaceManager_CopySamplePreservingInitialRotation(unittest.TestCase):
    @staticmethod
    def _ws_with_shape(shape_type_name):
        ws = MagicMock()
        ws.sample.return_value.getShape.return_value = type(shape_type_name, (), {})()
        return ws

    def test_csg_source_bakes_init_R_onto_destination_then_restores_identity(self, mock_copy):
        wm = _make_manager()
        wm.init_R = Rotation.from_euler("x", 30, degrees=True)
        source = self._ws_with_shape("CSGObject")
        dest = MagicMock()
        gonio = dest.run.return_value.getGoniometer.return_value

        wm.copy_sample_preserving_initial_rotation(source, dest)

        # CopySample overwrites a CSG shape's <goniometer> tag with the destination's run goniometer,
        # so the destination must carry init_R while the sample is copied...
        set_matrices = [c.args[0] for c in gonio.setR.call_args_list]
        self.assertEqual(len(set_matrices), 2)
        np.testing.assert_allclose(set_matrices[0], wm.init_R.as_matrix())
        # ...then be restored to identity so init_R lives in the shape, not the run goniometer
        np.testing.assert_array_equal(set_matrices[1], np.eye(3))
        mock_copy.assert_called_once_with(InputWorkspace=source, OutputWorkspace=dest, **COPY_KWARGS)

    def test_mesh_source_keeps_destination_at_identity(self, mock_copy):
        # a MeshObject already holds init_R in its vertices, which CopySample re-rotates by the
        # destination goniometer; leaving it at identity avoids applying init_R a second time
        wm = _make_manager()
        wm.init_R = Rotation.from_euler("x", 30, degrees=True)
        source = self._ws_with_shape("MeshObject")
        dest = MagicMock()
        gonio = dest.run.return_value.getGoniometer.return_value

        wm.copy_sample_preserving_initial_rotation(source, dest)

        for c in gonio.setR.call_args_list:
            np.testing.assert_array_equal(c.args[0], np.eye(3))


@patch(file_path + ".CopySample")
class TestWorkspaceManager_SetMaterial(unittest.TestCase):
    def test_copies_material_holder_onto_ws_mesh_and_updated_mesh(self, mock_copy):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.material_ws = "material"
        wm.ungrouped_ws = None

        wm.set_material()

        self.assertEqual(
            mock_copy.call_args_list,
            [
                call(InputWorkspace="material", OutputWorkspace="ws", **COPY_MATERIAL_KWARGS),
                call(InputWorkspace="material", OutputWorkspace="mesh", **COPY_MATERIAL_KWARGS),
                call(InputWorkspace="material", OutputWorkspace="neutral", **COPY_MATERIAL_KWARGS),
            ],
        )

    def test_also_applies_to_ungrouped_when_present(self, mock_copy):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.material_ws = "material"
        wm.ungrouped_ws = "ungrouped"

        wm.set_material()

        targets = [c.kwargs["OutputWorkspace"] for c in mock_copy.call_args_list]
        self.assertEqual(targets, ["ws", "mesh", "neutral", "ungrouped"])


@patch(file_path + ".CopySample")
class TestWorkspaceManager_PropagateMaterial(unittest.TestCase):
    def test_captures_ground_truth_then_copies_to_other_wss(self, mock_copy):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.material_ws = "material"
        wm.ungrouped_ws = "ungrouped"

        wm.propagate_material()

        self.assertEqual(
            mock_copy.call_args_list,
            [
                # the just-set material on the raw mesh ws becomes the new ground truth
                call(InputWorkspace="mesh", OutputWorkspace="material", **COPY_MATERIAL_KWARGS),
                call(InputWorkspace="mesh", OutputWorkspace="ws", **COPY_MATERIAL_KWARGS),
                call(InputWorkspace="mesh", OutputWorkspace="neutral", **COPY_MATERIAL_KWARGS),
                call(InputWorkspace="mesh", OutputWorkspace="ungrouped", **COPY_MATERIAL_KWARGS),
            ],
        )

    def test_skips_ungrouped_when_absent(self, mock_copy):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.material_ws = "material"
        wm.ungrouped_ws = None

        wm.propagate_material()

        targets = [c.kwargs["OutputWorkspace"] for c in mock_copy.call_args_list]
        self.assertEqual(targets, ["material", "ws", "neutral"])


class TestWorkspaceManager_GetMaterialName(unittest.TestCase):
    def test_returns_name_from_material_ws_sample_material(self):
        wm = _make_manager()
        wm.material_ws = MagicMock()
        wm.material_ws.sample.return_value.getMaterial.return_value.name.return_value = "Al2O3"

        self.assertEqual(wm.get_material_name(), "Al2O3")

    def test_returns_empty_string_when_unavailable(self):
        wm = _make_manager()
        wm.material_ws = None

        self.assertEqual(wm.get_material_name(), "")


@patch(file_path + ".SetSampleMaterial")
@patch(file_path + ".LoadSampleShape")
class TestWorkspaceManager_LoadStl(unittest.TestCase):
    def test_loads_stl_into_all_three_wss_with_stl_kwargs(self, mock_load_shape, mock_set_mat):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.set_material = MagicMock()

        wm.load_stl("file.stl")

        expected = [
            call(InputWorkspace="ws", Filename="file.stl", OutputWorkspace="ws", **wm.stl_kwargs),
            call(InputWorkspace="mesh", Filename="file.stl", OutputWorkspace="mesh", **wm.stl_kwargs),
            call(InputWorkspace="neutral", Filename="file.stl", OutputWorkspace="neutral", **wm.stl_kwargs),
        ]
        self.assertEqual(mock_load_shape.call_args_list, expected)

    def test_reapplies_material_after_loading_stl(self, mock_load_shape, mock_set_mat):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.ungrouped_ws = None
        wm.set_material = MagicMock()

        wm.load_stl("file.stl")

        # loading a shape wipes the material off the shape object; set_material re-applies the
        # ground-truth material (preserving its full definition, e.g. number/mass density)
        wm.set_material.assert_called_once_with()


@patch(file_path + ".SetSampleMaterial")
@patch(file_path + ".SetSampleShape")
class TestWorkspaceManager_LoadXml(unittest.TestCase):
    def _write_tmp_xml(self, content):
        with tempfile.NamedTemporaryFile("w", suffix=".xml", delete=False) as f:
            f.write(content)
            return f.name

    def test_sets_xml_shape_on_all_three_wss(self, mock_set_shape, mock_set_mat):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.set_material = MagicMock()
        fname = self._write_tmp_xml("<cube/>")
        try:
            wm.load_xml(fname)
        finally:
            os.unlink(fname)

        self.assertEqual(
            mock_set_shape.call_args_list,
            [
                call("ws", "<cube/>"),
                call("mesh", "<cube/>"),
                call("neutral", "<cube/>"),
            ],
        )

    def test_reapplies_material_after_loading_xml(self, mock_set_shape, mock_set_mat):
        wm = _make_manager()
        wm.ws = "ws"
        wm.mesh_ws = "mesh"
        wm.updated_mesh_ws = "neutral"
        wm.ungrouped_ws = None
        wm.set_material = MagicMock()
        fname = self._write_tmp_xml("<cube/>")
        try:
            wm.load_xml(fname)
        finally:
            os.unlink(fname)

        # loading a shape wipes the material off the shape object; set_material re-applies the
        # ground-truth material (preserving its full definition, e.g. number/mass density)
        wm.set_material.assert_called_once_with()


@patch(file_path + ".TranslateSampleShape")
class TestWorkspaceManager_TranslateShape(unittest.TestCase):
    def test_skips_when_all_zero(self, mock_translate):
        WorkspaceManager.translate_shape("ws", 0.0, 0.0, 0.0)
        mock_translate.assert_not_called()

    def test_translates_when_x_nonzero(self, mock_translate):
        WorkspaceManager.translate_shape("ws", 1.0, 0.0, 0.0)
        mock_translate.assert_called_once_with(InputWorkspace="ws", TranslationVector="1.0,0.0,0.0")

    def test_translates_when_all_nonzero(self, mock_translate):
        WorkspaceManager.translate_shape("ws", 1.0, 2.0, 3.0)
        mock_translate.assert_called_once_with(InputWorkspace="ws", TranslationVector="1.0,2.0,3.0")


class TestWorkspaceManager_InitialTranslationVector(unittest.TestCase):
    def test_returns_raw_offset_when_no_rotation(self):
        wm = _make_manager()
        wm.offset = (1.0, 2.0, 3.0)
        wm.init_R = Rotation.identity()

        np.testing.assert_allclose(wm.initial_translation_vector(), (1.0, 2.0, 3.0))

    def test_back_rotates_offset_by_inverse_init_R(self):
        # the offset is a lab-frame shift applied after the sample is oriented; expressing it in the
        # sample's pre-rotation frame (init_R^-1 @ offset) makes init_R @ that vector == offset again
        wm = _make_manager()
        wm.offset = (0.0, 1.0, 0.0)
        wm.init_R = Rotation.from_euler("x", 90, degrees=True)

        vec = wm.initial_translation_vector()

        np.testing.assert_allclose(vec, wm.init_R.inv().apply(wm.offset), atol=1e-12)
        np.testing.assert_allclose(wm.init_R.apply(vec), wm.offset, atol=1e-12)


class TestWorkspaceManager_AllRotsZero(unittest.TestCase):
    def test_true_for_zero(self):
        self.assertTrue(WorkspaceManager._all_rots_zero(0, 0, 0))

    def test_true_for_near_zero(self):
        self.assertTrue(WorkspaceManager._all_rots_zero(1e-12, -1e-12, 0))

    def test_false_when_x_nonzero(self):
        self.assertFalse(WorkspaceManager._all_rots_zero(1.0, 0, 0))

    def test_false_when_y_nonzero(self):
        self.assertFalse(WorkspaceManager._all_rots_zero(0, 1.0, 0))

    def test_false_when_z_nonzero(self):
        self.assertFalse(WorkspaceManager._all_rots_zero(0, 0, 1.0))


@patch(file_path + ".RotateSampleShape")
class TestWorkspaceManager_RotateSamplesByInitialGoniometer(unittest.TestCase):
    def test_returns_none_and_no_rotate_when_identity(self, mock_rotate):
        wm = _make_manager()
        wm.init_R = Rotation.identity()

        self.assertIsNone(wm.rotate_samples_by_initial_goniometer())
        mock_rotate.assert_not_called()

    def test_rotates_ws_and_updated_mesh_with_axis_angle_string(self, mock_rotate):
        wm = _make_manager()
        wm.updated_mesh_ws = "neutral"
        wm.init_R = Rotation.from_euler("x", 90, degrees=True)

        wm.rotate_samples_by_initial_goniometer()

        targets = [c.args[0] for c in mock_rotate.call_args_list]
        self.assertEqual(targets, [wm.WS_DATA, "neutral"])
        for c in mock_rotate.call_args_list:
            parts = c.args[1].split(",")
            self.assertAlmostEqual(float(parts[0]), 90.0)
            np.testing.assert_allclose([float(parts[1]), float(parts[2]), float(parts[3])], [1.0, 0.0, 0.0], atol=1e-12)
            self.assertEqual(parts[4], "1")


@patch(file_path + ".ADS")
@patch(file_path + ".CopySample")
class TestWorkspaceManager_UpdateInitialShape(unittest.TestCase):
    def _make_wm_with_ws(self):
        wm = _make_manager()
        wm.ws = MagicMock()
        gonio = MagicMock()
        wm.ws.run.return_value.getGoniometer.return_value = gonio
        wm.mesh_ws = MagicMock()
        wm.updated_mesh_ws = "neutral"
        wm.create_shape_workspace = MagicMock(return_value="tmp_ws")
        wm._copy_sample = MagicMock()
        wm.translate_shape = MagicMock()
        wm.rotate_samples_by_initial_goniometer = MagicMock()
        return wm, gonio

    def test_creates_tmp_ws_from_mesh(self, mock_copy, mock_ads):
        # this runs on every tick of the six initial-shape spin boxes, so the working copy is a
        # stub-instrument shape ws - it only ever holds a sample
        wm, _ = self._make_wm_with_ws()

        wm.update_initial_shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

        wm.create_shape_workspace.assert_called_once_with(wm.WS_TMP)
        wm._copy_sample.assert_called_once_with(wm.mesh_ws, "tmp_ws", preserve_initial_rotation=False)

    def test_stores_offset_and_translates_tmp_ws(self, mock_copy, mock_ads):
        wm, _ = self._make_wm_with_ws()

        wm.update_initial_shape(0.0, 0.0, 0.0, 1.0, 2.0, 3.0)

        self.assertEqual(wm.offset, (1.0, 2.0, 3.0))
        # with no initial rotation the shape is translated by the raw lab-frame offset
        wm.translate_shape.assert_called_once()
        args = wm.translate_shape.call_args.args
        self.assertEqual(args[0], "tmp_ws")
        np.testing.assert_allclose(args[1:], (1.0, 2.0, 3.0))

    def test_translates_tmp_ws_in_pre_rotation_frame(self, mock_copy, mock_ads):
        # the initial orientation is applied before the initial translation, so the offset is
        # pre-rotated by init_R^-1: once init_R is baked into the shape the net shift is the raw offset
        wm, _ = self._make_wm_with_ws()

        wm.update_initial_shape(90.0, 0.0, 0.0, 0.0, 1.0, 0.0)

        self.assertEqual(wm.offset, (0.0, 1.0, 0.0))
        args = wm.translate_shape.call_args.args
        self.assertEqual(args[0], "tmp_ws")
        expected = Rotation.from_euler("xyz", (90.0, 0.0, 0.0), degrees=True).inv().apply((0.0, 1.0, 0.0))
        np.testing.assert_allclose(args[1:], expected, atol=1e-12)

    def test_resets_goniometer_to_identity_before_copy(self, mock_copy, mock_ads):
        wm, gonio = self._make_wm_with_ws()

        wm.update_initial_shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

        gonio.setR.assert_called_once()
        np.testing.assert_array_equal(gonio.setR.call_args.args[0], np.eye(3))

    def test_copies_tmp_into_ws_and_updated_mesh(self, mock_copy, mock_ads):
        wm, _ = self._make_wm_with_ws()

        wm.update_initial_shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

        self.assertEqual(
            mock_copy.call_args_list,
            [
                call(InputWorkspace="tmp_ws", OutputWorkspace=wm.WS_DATA, **COPY_KWARGS),
                call(InputWorkspace="tmp_ws", OutputWorkspace="neutral", **COPY_KWARGS),
            ],
        )

    def test_zero_rotation_sets_init_R_identity_and_skips_rotate(self, mock_copy, mock_ads):
        wm, _ = self._make_wm_with_ws()

        wm.update_initial_shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

        np.testing.assert_array_equal(wm.init_R.as_matrix(), np.eye(3))
        wm.rotate_samples_by_initial_goniometer.assert_not_called()

    def test_nonzero_rotation_sets_init_R_from_euler_and_rotates(self, mock_copy, mock_ads):
        wm, _ = self._make_wm_with_ws()

        wm.update_initial_shape(90.0, 45.0, 30.0, 0.0, 0.0, 0.0)

        expected = Rotation.from_euler("xyz", (90.0, 45.0, 30.0), degrees=True)
        np.testing.assert_allclose(wm.init_R.as_matrix(), expected.as_matrix())
        wm.rotate_samples_by_initial_goniometer.assert_called_once_with()

    def test_tmp_workspace_always_removed(self, mock_copy, mock_ads):
        wm, _ = self._make_wm_with_ws()

        wm.update_initial_shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

        mock_ads.remove.assert_called_once_with(wm.WS_TMP)

    def test_tmp_workspace_removed_even_when_copy_raises(self, mock_copy, mock_ads):
        wm, _ = self._make_wm_with_ws()
        mock_copy.side_effect = RuntimeError("boom")

        with self.assertRaises(RuntimeError):
            wm.update_initial_shape(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

        mock_ads.remove.assert_called_once_with(wm.WS_TMP)


@patch(file_path + ".DeleteLog")
@patch(file_path + ".define_gauge_volume")
@patch(file_path + ".get_gauge_vol_str")
class TestWorkspaceManager_SetGaugeVolumeStr(unittest.TestCase):
    def test_stores_str_from_helper(self, mock_get_str, mock_define_gv, mock_delete_log):
        wm = _make_manager()
        wm.ws = MagicMock()
        mock_get_str.return_value = "<xml/>"

        wm.set_gauge_volume_str("preset", "custom")

        mock_get_str.assert_called_once_with("preset", "custom")
        self.assertEqual(wm.gauge_volume_str, "<xml/>")

    def test_nonempty_str_defines_gauge_volume(self, mock_get_str, mock_define_gv, mock_delete_log):
        wm = _make_manager()
        wm.ws = MagicMock()
        mock_get_str.return_value = "<xml/>"

        wm.set_gauge_volume_str("preset", "custom")

        mock_define_gv.assert_called_once_with(wm.ws, "<xml/>")
        mock_delete_log.assert_not_called()

    def test_empty_str_with_existing_property_deletes_log(self, mock_get_str, mock_define_gv, mock_delete_log):
        wm = _make_manager()
        wm.ws = MagicMock()
        wm.ws.run.return_value.hasProperty.return_value = True
        mock_get_str.return_value = None

        wm.set_gauge_volume_str("preset", "custom")

        self.assertIsNone(wm.gauge_volume_str)
        mock_define_gv.assert_not_called()
        mock_delete_log.assert_called_once_with(Workspace=wm.ws, Name="GaugeVolume")

    def test_empty_str_without_property_does_nothing(self, mock_get_str, mock_define_gv, mock_delete_log):
        wm = _make_manager()
        wm.ws = MagicMock()
        wm.ws.run.return_value.hasProperty.return_value = False
        mock_get_str.return_value = ""

        wm.set_gauge_volume_str("preset", "custom")

        mock_define_gv.assert_not_called()
        mock_delete_log.assert_not_called()


@patch(file_path + ".CreateSimulationWorkspace")
class TestWorkspaceManager_CreateSimulationWorkspace(unittest.TestCase):
    def test_defaults_to_the_generic_dspacing_grid(self, mock_create_sim):
        wm = _make_manager("ENGINX")

        result = wm.create_simulation_workspace("some_ws")

        mock_create_sim.assert_called_once_with(
            Instrument="ENGINX", BinParams=WorkspaceManager.DEFAULT_BIN_PARAMS, OutputWorkspace="some_ws", UnitX="dSpacing"
        )
        self.assertIs(result, mock_create_sim.return_value)

    def test_forwards_explicit_unit_and_bin_params(self, mock_create_sim):
        wm = _make_manager("IMAT")

        wm.create_simulation_workspace("some_ws", unit="Wavelength", bin_params="1,2,3")

        mock_create_sim.assert_called_once_with(Instrument="IMAT", BinParams="1,2,3", OutputWorkspace="some_ws", UnitX="Wavelength")

    def test_reads_instrument_from_the_model_at_call_time(self, mock_create_sim):
        # the instrument is a live property, so a switch must be picked up without rebuilding the manager
        model = MagicMock()
        model.instrument.get_instrument.return_value = "ENGINX"
        wm = WorkspaceManager(model)
        model.instrument.get_instrument.return_value = "IMAT"

        wm.create_simulation_workspace("some_ws")

        self.assertEqual(mock_create_sim.call_args.kwargs["Instrument"], "IMAT")


class TestWorkspaceManager_CreateBinParamsAroundPoint(unittest.TestCase):
    @staticmethod
    def _edges(params):
        start, step, stop = (float(p) for p in params.split(","))
        return np.arange(start, stop + step / 2, step)

    def _centres(self, params):
        edges = self._edges(params)
        return np.convolve(edges, np.ones(2), "valid") / 2

    def test_point_lands_on_the_middle_bin_centre(self):
        # read_attenuation_coefficient_at_value interpolates the MC output at the point, so the point
        # must sit on a bin centre, not merely inside the range
        for point in (0.2, 1.5, 7.5, 100.0):
            with self.subTest(point=point):
                centres = self._centres(WorkspaceManager.create_bin_params_around_point(point))
                self.assertEqual(len(centres), 3)
                self.assertAlmostEqual(centres[1], point)

    def test_builds_three_bins(self):
        # CreateSimulationWorkspace rejects anything with fewer than three bin boundaries, and
        # interpolation needs at least two points either side to be well defined
        edges = self._edges(WorkspaceManager.create_bin_params_around_point(1.5))

        self.assertEqual(len(edges), 4)

    def test_bracket_is_narrow_and_scales_with_the_point(self):
        # a fixed-width bracket would be far too wide for a small point and too narrow for a large one
        for point in (0.2, 1.5, 7.5):
            with self.subTest(point=point):
                edges = self._edges(WorkspaceManager.create_bin_params_around_point(point))
                self.assertAlmostEqual((edges[-1] - edges[0]) / point, 0.03)

    def test_bracket_stays_strictly_positive(self):
        # negative dSpacing/wavelength is meaningless and CreateSimulationWorkspace would reject it
        for point in (0.2, 1.5, 7.5):
            with self.subTest(point=point):
                self.assertGreater(self._edges(WorkspaceManager.create_bin_params_around_point(point))[0], 0)

    def test_non_positive_point_is_clamped_rather_than_raising(self):
        # a stray non-physical point should degrade to a garbage number, not kill the whole calculation
        for point in (0.0, -1.5):
            with self.subTest(point=point):
                params = WorkspaceManager.create_bin_params_around_point(point)
                self.assertGreater(self._edges(params)[0], 0)
                self.assertAlmostEqual(self._centres(params)[1], 1e-6)


if __name__ == "__main__":
    unittest.main()
