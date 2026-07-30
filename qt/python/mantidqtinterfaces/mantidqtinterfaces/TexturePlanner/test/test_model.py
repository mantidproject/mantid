# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from scipy.spatial.transform import Rotation
from unittest.mock import patch, MagicMock

from mantidqtinterfaces.TexturePlanner.model import TexturePlannerModel

file_path = "mantidqtinterfaces.TexturePlanner.model"


def _patch_collaborators(test_cls):
    """Class decorator stacking patches for all model collaborators and helpers.

    Tests receive mocks in the order: InstrumentHelper, WorkspaceManager,
    OrientationTable, DetectorGeometry, AbsorptionCalculator, OrientationExporter, TexturePlotter.
    """
    # Decorators are applied bottom-of-list-first; the first one applied is innermost
    # and maps to the FIRST argument in each test method (after self).
    decorators = [
        patch(file_path + ".InstrumentHelper"),
        patch(file_path + ".WorkspaceManager"),
        patch(file_path + ".OrientationTable"),
        patch(file_path + ".DetectorGeometry"),
        patch(file_path + ".AbsorptionCalculator"),
        patch(file_path + ".OrientationExporter"),
        patch(file_path + ".TexturePlotter"),
    ]
    for d in decorators:
        test_cls = d(test_cls)
    return test_cls


@_patch_collaborators
class TestTexturePlannerModel_Init(unittest.TestCase):
    def test_default_attributes(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()

        np.testing.assert_array_equal(model.ax_transform, np.eye(3))
        self.assertEqual(model.dir_names, ["D1", "D2", "D3"])
        self.assertEqual(model.projection, "azimuthal")
        self.assertEqual(model.gonio_index, 0)
        self.assertFalse(model.plot_transmission)

    def test_custom_instrument_and_projection(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel(instrument="IMAT", projection="stereographic")

        mock_instr.assert_called_once_with(model, "IMAT")
        self.assertEqual(model.projection, "stereographic")

    def test_constructs_all_collaborators_with_self(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()

        mock_wsm.assert_called_once_with(model)
        mock_ot.assert_called_once_with()
        mock_dg.assert_called_once_with(model)
        mock_abs.assert_called_once_with(model)
        mock_exp.assert_called_once_with(model)
        mock_plot.assert_called_once_with(model)
        mock_instr.assert_called_once_with(model, "ENGINX")


@_patch_collaborators
class TestTexturePlannerModel_StaticMethods(unittest.TestCase):
    def test_get_default_texture_directions(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        names, vecs = TexturePlannerModel.get_default_texture_directions()

        self.assertEqual(names, ("RD", "ND", "TD"))
        self.assertEqual(vecs, ((1, 0, 0), (0, 1, 0), (0, 0, 1)))


@_patch_collaborators
class TestTexturePlannerModel_Setters(unittest.TestCase):
    @patch(file_path + ".vec_string_to_norm_array")
    def test_set_ax_transform_normalises_each_vec_and_stacks_as_columns(
        self, mock_norm, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot
    ):
        model = TexturePlannerModel()
        mock_norm.side_effect = [np.array([1.0, 0.0, 0.0]), np.array([0.0, 1.0, 0.0]), np.array([0.0, 0.0, 1.0])]

        model.set_ax_transform("1,0,0", "0,1,0", "0,0,1")

        self.assertEqual([c.args[0] for c in mock_norm.call_args_list], ["1,0,0", "0,1,0", "0,0,1"])
        np.testing.assert_array_equal(model.ax_transform, np.eye(3))

    def test_set_dir_names(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()

        model.set_dir_names("A", "B", "C")

        self.assertEqual(model.dir_names, ["A", "B", "C"])

    def test_set_gonio_index(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()

        model.set_gonio_index(2)

        self.assertEqual(model.gonio_index, 2)

    def test_set_plot_transmission(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()

        model.set_plot_transmission(True)
        self.assertTrue(model.plot_transmission)

        model.set_plot_transmission(False)
        self.assertFalse(model.plot_transmission)

    def test_set_transform_dirs(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()

        model.set_transform_dirs(True)
        self.assertTrue(model.transform_dirs)

        model.set_transform_dirs(False)
        self.assertFalse(model.transform_dirs)


@_patch_collaborators
class TestTexturePlannerModel_EffectiveAxTransform(unittest.TestCase):
    """The initial shape rotation only reaches the texture directions when transform_dirs is set."""

    @staticmethod
    def _model_with_init_R(rotation):
        model = TexturePlannerModel()
        # workspaces is a mock, so init_R has to be given a real Rotation to read a matrix from
        model.workspaces.init_R = rotation
        return model

    def test_untransformed_returns_the_entered_transform_itself(self, *_):
        model = self._model_with_init_R(Rotation.from_euler("z", 90, degrees=True))
        model.ax_transform = np.array([[0.0, 0.0, 1.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]])
        model.transform_dirs = False

        # the same object, not just an equal one: the projection path must not pay for a copy
        self.assertIs(model.effective_ax_transform, model.ax_transform)

    def test_transformed_rotates_each_direction_by_init_R(self, *_):
        # RD/ND/TD along x/y/z, then a 90 deg rotation about z: x->y, y->-x, z->z
        model = self._model_with_init_R(Rotation.from_euler("z", 90, degrees=True))
        model.ax_transform = np.eye(3)
        model.transform_dirs = True

        # columns are the directions, so compare column-wise against the rotated basis vectors
        effective = model.effective_ax_transform
        np.testing.assert_allclose(effective[:, 0], (0.0, 1.0, 0.0), atol=1e-12)
        np.testing.assert_allclose(effective[:, 1], (-1.0, 0.0, 0.0), atol=1e-12)
        np.testing.assert_allclose(effective[:, 2], (0.0, 0.0, 1.0), atol=1e-12)

    def test_transformed_with_identity_init_R_is_a_no_op(self, *_):
        model = self._model_with_init_R(Rotation.identity())
        model.ax_transform = np.array([[0.0, 0.0, 1.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]])
        model.transform_dirs = True

        np.testing.assert_allclose(model.effective_ax_transform, model.ax_transform, atol=1e-12)

    def test_is_derived_so_it_follows_a_later_init_R_change(self, *_):
        model = self._model_with_init_R(Rotation.identity())
        model.ax_transform = np.eye(3)
        model.transform_dirs = True
        np.testing.assert_allclose(model.effective_ax_transform, np.eye(3), atol=1e-12)

        # init_R is rebuilt on every initial-shape edit; the property must not be caching it
        model.workspaces.init_R = Rotation.from_euler("x", 90, degrees=True)

        np.testing.assert_allclose(model.effective_ax_transform[:, 1], (0.0, 0.0, 1.0), atol=1e-12)


@_patch_collaborators
class TestTexturePlannerModel_GetTextureDirections(unittest.TestCase):
    """get_texture_directions feeds the view, which takes one direction per ROW - the transpose of
    the column-per-direction ax_transform."""

    # RD=(0,1,0), ND=(0,0,1), TD=(1,0,0) as set_ax_transform would store them (as columns)
    _COLUMNS = np.array([[0.0, 0.0, 1.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]])
    _ROWS = np.array([[0.0, 1.0, 0.0], [0.0, 0.0, 1.0], [1.0, 0.0, 0.0]])

    def test_sample_frame_returns_entered_directions_as_rows(self, *_):
        model = TexturePlannerModel()
        model.workspaces.init_R = Rotation.from_euler("z", 90, degrees=True)
        model.ax_transform = self._COLUMNS
        model.set_dir_names("A", "B", "C")
        model.transform_dirs = True

        names, vecs = model.get_texture_directions(lab_frame=False)

        self.assertEqual(names, ("A", "B", "C"))
        # the sample frame is what the user entered - unaffected by init_R even when transforming
        np.testing.assert_allclose(vecs, self._ROWS, atol=1e-12)

    def test_lab_frame_returns_rotated_directions_as_rows(self, *_):
        model = TexturePlannerModel()
        model.workspaces.init_R = Rotation.from_euler("z", 90, degrees=True)
        model.ax_transform = self._COLUMNS
        model.transform_dirs = True

        _, vecs = model.get_texture_directions(lab_frame=True)

        # 90 deg about z: RD (0,1,0) -> (-1,0,0), ND (0,0,1) -> (0,0,1), TD (1,0,0) -> (0,1,0)
        np.testing.assert_allclose(vecs, [[-1.0, 0.0, 0.0], [0.0, 0.0, 1.0], [0.0, 1.0, 0.0]], atol=1e-12)

    def test_lab_frame_matches_sample_frame_when_not_transforming(self, *_):
        model = TexturePlannerModel()
        model.workspaces.init_R = Rotation.from_euler("z", 90, degrees=True)
        model.ax_transform = self._COLUMNS
        model.transform_dirs = False

        _, lab = model.get_texture_directions(lab_frame=True)
        _, sample = model.get_texture_directions(lab_frame=False)

        np.testing.assert_allclose(lab, sample, atol=1e-12)

    def test_row_layout_matches_the_defaults(self, *_):
        # both feed set_view_texture_directions, so they must agree on the layout
        model = TexturePlannerModel()
        model.workspaces.init_R = Rotation.identity()
        model.ax_transform = np.eye(3)
        model.transform_dirs = False

        _, default_vecs = TexturePlannerModel.get_default_texture_directions()
        _, vecs = model.get_texture_directions(lab_frame=False)

        np.testing.assert_allclose(vecs, np.array(default_vecs, dtype=float), atol=1e-12)


@_patch_collaborators
class TestTexturePlannerModel_UpdateGonioIndex(unittest.TestCase):
    def test_returns_current_index_when_below_max(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()
        model.gonio_index = 1

        self.assertEqual(model.clamp_gonio_index(num_gonios=4), 1)

    def test_clamps_to_max_when_index_exceeds(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()
        model.gonio_index = 5

        self.assertEqual(model.clamp_gonio_index(num_gonios=3), 2)

    def test_clamps_to_zero_when_only_one_gonio(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()
        model.gonio_index = 4

        self.assertEqual(model.clamp_gonio_index(num_gonios=1), 0)


@_patch_collaborators
class TestTexturePlannerModel_ProjectionOrchestration(unittest.TestCase):
    @patch(file_path + ".project_orientation")
    def test_update_projected_data_writes_pf_points_on_orientation(
        self, mock_proj, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot
    ):
        model = TexturePlannerModel()
        orientation = MagicMock()
        orientation.R = "R_obj"
        model.orientations.__getitem__.return_value = orientation
        model.geometry.detQs_lab = "detQs"
        mock_proj.return_value = "pf_points"

        model.update_projected_data(3)

        model.orientations.__getitem__.assert_called_once_with(3)
        mock_proj.assert_called_once_with("R_obj", "detQs", model.ax_transform, model.projection)
        self.assertEqual(orientation.pf_points, "pf_points")

    @patch(file_path + ".project_orientation")
    def test_update_projected_data_projects_through_init_R_when_transforming_dirs(
        self, mock_proj, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot
    ):
        model = TexturePlannerModel()
        model.orientations.__getitem__.return_value = MagicMock()
        model.geometry.detQs_lab = "detQs"
        model.workspaces.init_R = Rotation.from_euler("z", 90, degrees=True)
        model.ax_transform = np.eye(3)
        model.transform_dirs = True

        model.update_projected_data(0)

        # the projection is handed the rotated directions, not the entered ones
        projected_transform = mock_proj.call_args.args[2]
        np.testing.assert_allclose(projected_transform, Rotation.from_euler("z", 90, degrees=True).as_matrix(), atol=1e-12)

    @patch(file_path + ".project_orientation")
    def test_update_projected_data_skips_absorption_when_transmission_off(
        self, mock_proj, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot
    ):
        model = TexturePlannerModel()
        model.orientations.__getitem__.return_value = MagicMock()
        model.plot_transmission = False

        model.update_projected_data(0)

        model.absorption.calc_for_index.assert_not_called()

    @patch(file_path + ".project_orientation")
    def test_update_projected_data_runs_absorption_when_transmission_on(
        self, mock_proj, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot
    ):
        model = TexturePlannerModel()
        model.orientations.__getitem__.return_value = MagicMock()
        model.plot_transmission = True

        model.update_projected_data(7)

        model.absorption.calc_for_index.assert_called_once_with(7)

    def test_update_all_projected_data_iterates_orientations(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()
        model.orientations.keys.return_value = [0, 1, 4]
        model.update_projected_data = MagicMock()

        model.update_all_projected_data()

        self.assertEqual(
            [c.args[0] for c in model.update_projected_data.call_args_list],
            [0, 1, 4],
        )


if __name__ == "__main__":
    unittest.main()
