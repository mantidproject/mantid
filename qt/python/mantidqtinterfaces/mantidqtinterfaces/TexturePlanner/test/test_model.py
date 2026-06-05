# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

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

        self.assertEqual(model.gon_colors, ("hotpink", "orange", "purple", "goldenrod", "plum", "saddlebrown"))
        self.assertEqual(model.dir_cols, ("red", "green", "blue"))
        np.testing.assert_array_equal(model.ax_transform, np.eye(3))
        self.assertEqual(model.dir_names, ["D1", "D2", "D3"])
        self.assertEqual(model.projection, "azimuthal")
        self.assertEqual(
            model.vis_settings,
            {"directions": True, "goniometers": True, "incident": True, "ks": True, "scattered": False},
        )
        self.assertEqual(model.gonio_index, 0)
        self.assertEqual(model.n_output_points, 1)
        self.assertFalse(model.plot_transmission)
        self.assertFalse(model.transmission_use_data_range)
        self.assertEqual(model.orientation_kwargs, {"Axes": "YXY", "Senses": "-1,-1,-1"})

    def test_custom_instrument_and_projection(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel(instrument="IMAT", projection="stereographic")

        mock_instr.assert_called_once_with(model, "IMAT")
        self.assertEqual(model.projection, "stereographic")

    def test_constructs_all_collaborators_with_self(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()

        mock_wsm.assert_called_once_with(model)
        mock_ot.assert_called_once_with(model)
        mock_dg.assert_called_once_with(model)
        mock_abs.assert_called_once_with(model)
        mock_exp.assert_called_once_with(model)
        mock_plot.assert_called_once_with(model)
        mock_instr.assert_called_once_with(model, "ENGINX")

    def test_mc_kwargs_populated_from_workspace_constants(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        mock_wsm.return_value.WS_MC_INPUT = "mc_in"
        mock_wsm.return_value.WS_MC_OUTPUT = "mc_out"

        model = TexturePlannerModel()

        self.assertEqual(
            model.mc_kwargs,
            {
                "InputWorkspace": "mc_in",
                "OutputWorkspace": "mc_out",
                "EventsPerPoint": 50,
                "MaxScatterPtAttempts": int(1e4),
                "SimulateScatteringPointIn": "SampleOnly",
                "ResimulateTracksForDifferentWavelengths": False,
            },
        )


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


@_patch_collaborators
class TestTexturePlannerModel_UpdateGonioIndex(unittest.TestCase):
    def test_returns_current_index_when_below_max(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()
        model.gonio_index = 1

        self.assertEqual(model.update_gonio_index(num_gonios=4), 1)

    def test_clamps_to_max_when_index_exceeds(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()
        model.gonio_index = 5

        self.assertEqual(model.update_gonio_index(num_gonios=3), 2)

    def test_clamps_to_zero_when_only_one_gonio(self, mock_instr, mock_wsm, mock_ot, mock_dg, mock_abs, mock_exp, mock_plot):
        model = TexturePlannerModel()
        model.gonio_index = 4

        self.assertEqual(model.update_gonio_index(num_gonios=1), 0)


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
