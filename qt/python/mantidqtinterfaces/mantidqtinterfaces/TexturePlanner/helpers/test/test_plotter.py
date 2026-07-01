# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

from unittest.mock import patch, MagicMock, call
from scipy.spatial.transform import Rotation

from mantidqtinterfaces.TexturePlanner.helpers.plotter import TexturePlotter

file_path = "mantidqtinterfaces.TexturePlanner.helpers.plotter"


def _make_model(
    dir_names=None,
    ax_transform=None,
    projection="ster",
    gonio_index=0,
    plot_transmission=False,
    orientations=None,
    detQs_lab=None,
    det_k=None,
    source_position=(-1.0, 0.0, 0.0),
    scattering_centre=(0.0, 0.0, 0.0),
    gauge_volume_str=None,
):
    # NB: the plotter owns its visual config (vis toggles + colours + transmission clim); those are
    # configured on the plotter itself via _make_plotter(...), not on the model.
    model = MagicMock()
    model.dir_names = dir_names or ["RD", "ND", "TD"]
    model.ax_transform = np.eye(3) if ax_transform is None else ax_transform
    model.projection = projection
    model.gonio_index = gonio_index
    model.plot_transmission = plot_transmission
    if orientations is None:
        orient = MagicMock()
        orient.gRs = [Rotation.identity()]
        orient.R = Rotation.identity()
        orient.include = True
        orient.pf_points = np.array([[0.1, 0.2], [0.3, 0.4]])
        orient.transmission = np.array([0.5, 0.7])
        orientations = {0: orient}
    model.orientations = MagicMock()
    model.orientations.__getitem__.side_effect = lambda i: orientations[i]
    model.orientations.items.return_value = list(orientations.items())
    model.orientations.values.return_value = list(orientations.values())
    model.geometry.detQs_lab = np.array([[1.0, 0.0, 0.0]]) if detQs_lab is None else detQs_lab
    model.geometry.det_k = np.array([[0.0, 1.0, 0.0]]) if det_k is None else det_k
    model.workspaces.ws.componentInfo.return_value.sourcePosition.return_value = list(source_position)
    model.workspaces.scattering_centre = np.asarray(scattering_centre, dtype=float)
    model.workspaces.wsname = "ws_name"
    model.workspaces.gauge_volume_str = gauge_volume_str
    return model


def _make_plotter(model, *, vis_settings=None, gon_colors=None, dir_cols=None, transmission_use_data_range=None):
    # the plotter owns its visual config; by default we leave the constructor defaults in place so
    # __init__ wiring is actually exercised. Tests that intentionally drive a specific config pass
    # the relevant override(s), which are applied on top of the real defaults.
    plotter = TexturePlotter(model)
    if vis_settings is not None:
        plotter.vis_settings = vis_settings
    if gon_colors is not None:
        plotter.gon_colors = gon_colors
    if dir_cols is not None:
        plotter.dir_cols = dir_cols
    if transmission_use_data_range is not None:
        plotter.transmission_use_data_range = transmission_use_data_range
    return plotter


class TestTexturePlotter_UpdatePlot(unittest.TestCase):
    @patch(file_path + ".plot_sample_only")
    @patch(file_path + ".ShowSampleModel")
    def test_orchestrates_helpers_in_order(self, mock_show_sample, mock_plot_sample):
        model = _make_model()
        # stub the internal helpers so we only verify orchestration
        plotter = _make_plotter(model)
        plotter._draw_goniometers = MagicMock(return_value=[np.array([1.0, 0.0, 0.0])])
        plotter._draw_sample_and_axes = MagicMock()
        plotter._draw_beam_and_detectors = MagicMock()
        plotter._project_goniometer_poles = MagicMock(return_value=np.array([[0.1, 0.2]]))
        plotter._draw_pole_figure = MagicMock()
        plotter._decorate_pole_figure = MagicMock()

        # fake mesh on updated_mesh_ws
        mesh = np.ones((2, 3, 3))
        model.workspaces.updated_mesh_ws.sample.return_value.getShape.return_value.getMesh.return_value = mesh

        fig = MagicMock()
        lab_ax = MagicMock()
        proj_ax = MagicMock()

        plotter.update_plot([(1, 0, 0)], [1], [10.0], fig, lab_ax, proj_ax, 0)

        lab_ax.clear.assert_called_once_with()
        proj_ax.clear.assert_called_once_with()
        # _draw_goniometers receives (lab_ax, vecs, senses, angles, gRs, n_gon, extent)
        gon_args = plotter._draw_goniometers.call_args.args
        self.assertIs(gon_args[0], lab_ax)
        self.assertEqual(gon_args[1:4], ([(1, 0, 0)], [1], [10.0]))
        # _draw_sample_and_axes receives (fig, lab_ax, rot_mesh, extent, n_gon, scat_centre)
        sample_args = plotter._draw_sample_and_axes.call_args.args
        self.assertIs(sample_args[0], fig)
        self.assertIs(sample_args[1], lab_ax)
        # _draw_beam_and_detectors receives (lab_ax, scat_centre, extent, n_gon)
        beam_args = plotter._draw_beam_and_detectors.call_args.args
        self.assertIs(beam_args[0], lab_ax)
        # _project_goniometer_poles receives (R, g_vecs)
        proj_args = plotter._project_goniometer_poles.call_args.args
        np.testing.assert_allclose(proj_args[0].as_matrix(), np.eye(3))
        plotter._draw_pole_figure.assert_called_once_with(proj_ax, plotter._project_goniometer_poles.return_value, 0)
        plotter._decorate_pole_figure.assert_called_once_with(proj_ax)
        lab_ax.set_axis_off.assert_called_once_with()
        fig.canvas.draw_idle.assert_called_once_with()
        proj_ax.figure.canvas.draw_idle.assert_called_once_with()

    @patch(file_path + ".plot_sample_only")
    @patch(file_path + ".ShowSampleModel")
    def test_applies_orientation_R_to_workspace_goniometer(self, mock_show_sample, mock_plot_sample):
        R = Rotation.from_euler("z", 45, degrees=True)
        orient = MagicMock()
        orient.gRs = [Rotation.identity()]
        orient.R = R
        orient.include = True
        orient.pf_points = np.array([[0.0, 0.0]])
        model = _make_model(orientations={0: orient})
        gonio = MagicMock()
        model.workspaces.ws.run.return_value.getGoniometer.return_value = gonio
        model.workspaces.updated_mesh_ws.sample.return_value.getShape.return_value.getMesh.return_value = np.ones((2, 3, 3))

        plotter = _make_plotter(model)
        plotter._draw_goniometers = MagicMock(return_value=[np.array([1.0, 0.0, 0.0])])
        plotter._draw_sample_and_axes = MagicMock()
        plotter._draw_beam_and_detectors = MagicMock()
        plotter._project_goniometer_poles = MagicMock(return_value=np.array([[0.0, 0.0]]))
        plotter._draw_pole_figure = MagicMock()
        plotter._decorate_pole_figure = MagicMock()

        plotter.update_plot([], [], [], MagicMock(), MagicMock(), MagicMock(), 0)

        self.assertEqual(len(gonio.setR.call_args_list), 1)
        np.testing.assert_allclose(gonio.setR.call_args.args[0], R.as_matrix())


@patch(file_path + ".ring")
class TestTexturePlotter_DrawGoniometers(unittest.TestCase):
    def _stub_ring(self, mock_ring):
        # ring returns shape (3, res); the code transposes / applies and reverses
        mock_ring.return_value = np.tile(np.arange(360.0), (3, 1))

    def test_skipped_entirely_when_goniometers_vis_off(self, mock_ring):
        self._stub_ring(mock_ring)
        model = _make_model()
        plotter = _make_plotter(
            model, vis_settings={"goniometers": False, "directions": True, "incident": True, "ks": True, "scattered": True}
        )
        lab_ax = MagicMock()

        g_vecs = plotter._draw_goniometers(lab_ax, [(1.0, 0.0, 0.0)], [1], [0.0], [Rotation.identity()], 1, 1.0)

        lab_ax.plot.assert_not_called()
        lab_ax.quiver.assert_not_called()
        lab_ax.legend.assert_not_called()
        # g_vecs still computed even when vis is off
        self.assertEqual(len(g_vecs), 1)

    def test_draws_two_segments_and_quiver_per_axis_when_visible(self, mock_ring):
        self._stub_ring(mock_ring)
        model = _make_model()
        plotter = _make_plotter(model, gon_colors=["red", "green", "blue"])
        lab_ax = MagicMock()

        plotter._draw_goniometers(
            lab_ax,
            [(1.0, 0.0, 0.0), (0.0, 1.0, 0.0)],
            [1, 1],
            [45.0, 90.0],
            [Rotation.identity(), Rotation.identity()],
            n_gon=2,
            extent=1.0,
        )

        # 2 ring segments per axis * 2 axes = 4 plot calls; colors alternate per-axis-colour then "grey"
        plot_colors = [c.kwargs["color"] for c in lab_ax.plot.call_args_list]
        self.assertEqual(plot_colors, ["red", "grey", "green", "grey"])
        # one quiver per axis
        quiver_colors = [c.kwargs["color"] for c in lab_ax.quiver.call_args_list]
        quiver_labels = [c.kwargs["label"] for c in lab_ax.quiver.call_args_list]
        self.assertEqual(quiver_colors, ["red", "green"])
        self.assertEqual(quiver_labels, ["Axis 0", "Axis 1"])
        lab_ax.legend.assert_called_once_with()

    def test_quiver_linestyle_solid_for_active_axis_dashed_for_others(self, mock_ring):
        self._stub_ring(mock_ring)
        model = _make_model(gonio_index=1)
        plotter = _make_plotter(model)
        lab_ax = MagicMock()

        plotter._draw_goniometers(
            lab_ax,
            [(1.0, 0.0, 0.0), (0.0, 1.0, 0.0)],
            [1, 1],
            [10.0, 10.0],
            [Rotation.identity(), Rotation.identity()],
            n_gon=2,
            extent=1.0,
        )

        linestyles = [c.kwargs["ls"] for c in lab_ax.quiver.call_args_list]
        self.assertEqual(linestyles, ["--", "-"])
        labels = [c.kwargs["label"] for c in lab_ax.quiver.call_args_list]
        self.assertEqual(labels, ["Axis 0", "Axis 1"])

    def test_returns_one_g_vec_per_input_vec(self, mock_ring):
        self._stub_ring(mock_ring)
        model = _make_model()
        plotter = _make_plotter(model)

        g_vecs = plotter._draw_goniometers(
            MagicMock(),
            [(1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)],
            [1, 1, 1],
            [0.0, 0.0, 0.0],
            [Rotation.identity()] * 3,
            n_gon=3,
            extent=1.0,
        )

        self.assertEqual(len(g_vecs), 3)


@patch(file_path + ".plot_sample_only")
@patch(file_path + ".ShowSampleModel")
class TestTexturePlotter_DrawSampleAndAxes(unittest.TestCase):
    def test_calls_plot_sample_only_with_grey_alpha(self, mock_show_sample, mock_plot_sample):
        model = _make_model()
        plotter = _make_plotter(model)
        fig = MagicMock()
        lab_ax = MagicMock()
        rot_mesh = np.ones((2, 3, 3))

        plotter._draw_sample_and_axes(fig, lab_ax, rot_mesh, extent=1.0, n_gon=2, scat_centre=np.zeros(3))

        fig.sca.assert_called_once_with(lab_ax)
        mock_plot_sample.assert_called_once_with(fig, rot_mesh, 0.5, "grey")

    def test_sets_axis_limits_and_aspect_from_extent_and_n_gon(self, mock_show_sample, mock_plot_sample):
        model = _make_model()
        plotter = _make_plotter(model)
        lab_ax = MagicMock()

        plotter._draw_sample_and_axes(MagicMock(), lab_ax, np.ones((2, 3, 3)), extent=3.0, n_gon=2, scat_centre=np.zeros(3))

        lim = 3.0 * 2 / 1.5
        lab_ax.set.assert_called_once_with(xlim=[-lim, lim], ylim=[-lim, lim], zlim=[-lim, lim], aspect="equal")

    def test_plot_sample_directions_called_only_when_directions_vis_on(self, mock_show_sample, mock_plot_sample):
        sample_model = MagicMock()
        mock_show_sample.return_value = sample_model
        model = _make_model()
        plotter = _make_plotter(
            model, vis_settings={"goniometers": True, "directions": False, "incident": True, "ks": True, "scattered": True}
        )

        plotter._draw_sample_and_axes(MagicMock(), MagicMock(), np.ones((2, 3, 3)), extent=1.0, n_gon=1, scat_centre=np.zeros(3))

        sample_model.plot_sample_directions.assert_not_called()

    def test_plot_gauge_vol_called_when_gauge_volume_str_set(self, mock_show_sample, mock_plot_sample):
        sample_model = MagicMock()
        mock_show_sample.return_value = sample_model
        model = _make_model(gauge_volume_str="<gv/>")
        plotter = _make_plotter(model)

        plotter._draw_sample_and_axes(MagicMock(), MagicMock(), np.ones((2, 3, 3)), extent=1.0, n_gon=1, scat_centre=np.zeros(3))

        sample_model.plot_gauge_vol.assert_called_once_with()

    def test_plot_gauge_vol_skipped_when_no_gauge_volume_str(self, mock_show_sample, mock_plot_sample):
        sample_model = MagicMock()
        mock_show_sample.return_value = sample_model
        model = _make_model(gauge_volume_str=None)
        plotter = _make_plotter(model)

        plotter._draw_sample_and_axes(MagicMock(), MagicMock(), np.ones((2, 3, 3)), extent=1.0, n_gon=1, scat_centre=np.zeros(3))

        sample_model.plot_gauge_vol.assert_not_called()


class TestTexturePlotter_DrawBeamAndDetectors(unittest.TestCase):
    def _make_plotter_with_stubbed_quiver_bundle(self, model, vis_settings):
        plotter = _make_plotter(model, vis_settings=vis_settings)
        plotter._draw_quiver_bundle = MagicMock()
        return plotter

    def test_incident_beam_drawn_only_when_vis_incident_on(self):
        model = _make_model()
        vis_settings = {"goniometers": True, "directions": True, "incident": True, "ks": False, "scattered": False}
        plotter = self._make_plotter_with_stubbed_quiver_bundle(model, vis_settings)
        lab_ax = MagicMock()

        plotter._draw_beam_and_detectors(lab_ax, np.zeros(3), extent=1.0, n_gon=1)

        # incident-beam quiver should be the single call, with these styling kwargs
        self.assertEqual(len(lab_ax.quiver.call_args_list), 1)
        kw = lab_ax.quiver.call_args.kwargs
        self.assertEqual(kw["color"], "black")
        self.assertEqual(kw["alpha"], 0.25)
        self.assertEqual(kw["arrow_length_ratio"], 0.05)

    def test_incident_beam_skipped_when_vis_incident_off(self):
        model = _make_model()
        vis_settings = {"goniometers": True, "directions": True, "incident": False, "ks": False, "scattered": False}
        plotter = self._make_plotter_with_stubbed_quiver_bundle(model, vis_settings)
        lab_ax = MagicMock()

        plotter._draw_beam_and_detectors(lab_ax, np.zeros(3), extent=1.0, n_gon=1)

        lab_ax.quiver.assert_not_called()

    def test_ks_bundle_drawn_when_vis_ks_on(self):
        model = _make_model()
        vis_settings = {"goniometers": True, "directions": True, "incident": False, "ks": True, "scattered": False}
        plotter = self._make_plotter_with_stubbed_quiver_bundle(model, vis_settings)
        lab_ax = MagicMock()
        scat_centre = np.zeros(3)

        plotter._draw_beam_and_detectors(lab_ax, scat_centre, extent=2.0, n_gon=1)

        self.assertEqual(len(plotter._draw_quiver_bundle.call_args_list), 1)
        args = plotter._draw_quiver_bundle.call_args
        self.assertIs(args.args[0], lab_ax)
        np.testing.assert_array_equal(args.args[1], model.geometry.detQs_lab)
        np.testing.assert_array_equal(args.args[2], scat_centre)
        self.assertEqual(args.args[3], 2.0)
        self.assertEqual(args.args[4], "dodgerblue")
        self.assertEqual(args.kwargs, {"linestyle": "--"})

    def test_scattered_bundle_drawn_when_vis_scattered_on(self):
        model = _make_model()
        vis_settings = {"goniometers": True, "directions": True, "incident": False, "ks": False, "scattered": True}
        plotter = self._make_plotter_with_stubbed_quiver_bundle(model, vis_settings)
        lab_ax = MagicMock()

        plotter._draw_beam_and_detectors(lab_ax, np.zeros(3), extent=1.0, n_gon=1)

        self.assertEqual(len(plotter._draw_quiver_bundle.call_args_list), 1)
        args = plotter._draw_quiver_bundle.call_args
        self.assertIs(args.args[0], lab_ax)
        np.testing.assert_array_equal(args.args[1], np.asarray(model.geometry.det_k))
        self.assertEqual(args.args[3], 1.0)
        self.assertEqual(args.args[4], "grey")
        self.assertEqual(args.kwargs, {})


class TestTexturePlotter_DrawQuiverBundle(unittest.TestCase):
    def test_calls_quiver_with_n_arrows_from_scattering_centre(self):
        lab_ax = MagicMock()
        dirs = np.array([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0]])
        scat_centre = np.array([10.0, 20.0, 30.0])

        _make_plotter(_make_model())._draw_quiver_bundle(lab_ax, dirs, scat_centre, extent=2.0, tip_color="red")

        args = lab_ax.quiver.call_args.args
        np.testing.assert_array_equal(args[0], np.array([10.0, 10.0]))
        np.testing.assert_array_equal(args[1], np.array([20.0, 20.0]))
        np.testing.assert_array_equal(args[2], np.array([30.0, 30.0]))
        # arrow vectors scaled by 1.25 * extent
        np.testing.assert_allclose(args[3], dirs[:, 0] * 2.5)
        np.testing.assert_allclose(args[4], dirs[:, 1] * 2.5)
        np.testing.assert_allclose(args[5], dirs[:, 2] * 2.5)

    def test_passes_linestyle_through_to_quiver(self):
        lab_ax = MagicMock()
        _make_plotter(_make_model())._draw_quiver_bundle(
            lab_ax, np.array([[1.0, 0.0, 0.0]]), np.zeros(3), extent=1.0, tip_color="red", linestyle="--"
        )

        self.assertEqual(lab_ax.quiver.call_args.kwargs["linestyle"], "--")

    def test_scatters_tips_with_tip_color(self):
        lab_ax = MagicMock()
        dirs = np.array([[1.0, 0.0, 0.0]])
        scat_centre = np.zeros(3)

        _make_plotter(_make_model())._draw_quiver_bundle(lab_ax, dirs, scat_centre, extent=2.0, tip_color="red")

        # tips = dirs * 1.25 * extent + scat_centre = [[2.5, 0, 0]]
        scatter_args = lab_ax.scatter.call_args
        np.testing.assert_allclose(scatter_args.args[0], np.array([2.5]))
        np.testing.assert_allclose(scatter_args.args[1], np.array([0.0]))
        np.testing.assert_allclose(scatter_args.args[2], np.array([0.0]))
        self.assertEqual(scatter_args.kwargs, {"color": "red", "s": 2})


@patch(file_path + ".azim_proj_xy")
@patch(file_path + ".ster_proj_xy")
@patch(file_path + ".get_alpha_beta_from_cart")
class TestTexturePlotter_ProjectGoniometerPoles(unittest.TestCase):
    def test_uses_stereographic_projection_when_projection_is_ster(self, mock_alpha_beta, mock_ster, mock_azim):
        mock_alpha_beta.return_value = np.array([[0.1, 0.2], [0.3, 0.4]])
        mock_ster.return_value = "ster_result"
        model = _make_model(projection="ster")
        plotter = _make_plotter(model)

        result = plotter._project_goniometer_poles(Rotation.identity(), [np.array([1.0, 0.0, 0.0])])

        self.assertEqual(result, "ster_result")
        # ster_proj_xy is called with the two columns of get_alpha_beta_from_cart's transposed return
        ster_args = mock_ster.call_args.args
        np.testing.assert_allclose(ster_args[0], np.array([0.1, 0.3]))
        np.testing.assert_allclose(ster_args[1], np.array([0.2, 0.4]))
        mock_azim.assert_not_called()

    def test_uses_azimuthal_projection_when_projection_is_not_ster(self, mock_alpha_beta, mock_ster, mock_azim):
        mock_alpha_beta.return_value = np.array([[0.1, 0.2], [0.3, 0.4]])
        mock_azim.return_value = "azim_result"
        model = _make_model(projection="azim")
        plotter = _make_plotter(model)

        result = plotter._project_goniometer_poles(Rotation.identity(), [np.array([1.0, 0.0, 0.0])])

        self.assertEqual(result, "azim_result")
        azim_args = mock_azim.call_args.args
        np.testing.assert_allclose(azim_args[0], np.array([0.1, 0.3]))
        np.testing.assert_allclose(azim_args[1], np.array([0.2, 0.4]))
        mock_ster.assert_not_called()


class TestTexturePlotter_DrawPoleFigure(unittest.TestCase):
    def test_goniometer_pole_on_unit_circle_draws_line_segment(self):
        model = _make_model(gonio_index=0)
        plotter = _make_plotter(model)
        proj_ax = MagicMock()
        # a pole exactly on the unit circle (norm == 1) -> line
        g_pole_xy = [np.array([1.0, 0.0])]

        plotter._draw_pole_figure(proj_ax, g_pole_xy, current_index=0)

        self.assertTrue(any(call_ for call_ in proj_ax.plot.call_args_list))

    def test_goniometer_pole_inside_unit_circle_draws_scatter(self):
        model = _make_model(gonio_index=0)
        plotter = _make_plotter(model, gon_colors=["red", "green", "blue"])
        proj_ax = MagicMock()
        g_pole_xy = [np.array([0.1, 0.0])]  # norm < 1

        plotter._draw_pole_figure(proj_ax, g_pole_xy, current_index=0)

        proj_ax.scatter.assert_any_call(0.0, 0.1, s=30, edgecolor="red", facecolor="red")

    def test_goniometer_pole_inactive_axis_uses_no_fill(self):
        model = _make_model(gonio_index=0)
        plotter = _make_plotter(model, gon_colors=["red", "green", "blue"])
        proj_ax = MagicMock()
        # first pole active (index 0 == gonio_index); second pole inactive
        g_pole_xy = [np.array([0.1, 0.0]), np.array([0.0, 0.2])]

        plotter._draw_pole_figure(proj_ax, g_pole_xy, current_index=0)

        # inactive axis gets facecolor "None"
        self.assertIn(
            call(0.2, 0.0, s=30, edgecolor="green", facecolor="None"),
            proj_ax.scatter.call_args_list,
        )

    def test_included_orientation_draws_pf_points_with_filled_color_for_current(self):
        orient = MagicMock()
        orient.include = True
        orient.pf_points = np.array([[0.1, 0.2]])
        model = _make_model(orientations={0: orient}, plot_transmission=False)
        plotter = _make_plotter(model)
        proj_ax = MagicMock()

        plotter._draw_pole_figure(proj_ax, [], current_index=0)

        proj_ax.scatter.assert_any_call(np.array([0.2]), np.array([0.1]), s=20, c="dodgerblue")

    def test_included_orientation_draws_open_circles_when_not_current(self):
        cur = MagicMock()
        cur.include = True
        cur.pf_points = np.array([[0.0, 0.0]])
        other = MagicMock()
        other.include = True
        other.pf_points = np.array([[0.1, 0.2]])
        model = _make_model(orientations={0: cur, 1: other}, plot_transmission=False)
        plotter = _make_plotter(model)
        proj_ax = MagicMock()

        plotter._draw_pole_figure(proj_ax, [], current_index=0)

        proj_ax.scatter.assert_any_call(np.array([0.2]), np.array([0.1]), s=20, facecolor="None", edgecolor="dodgerblue")

    def test_excluded_current_orientation_drawn_in_grey(self):
        orient = MagicMock()
        orient.include = False
        orient.pf_points = np.array([[0.1, 0.2]])
        model = _make_model(orientations={0: orient}, plot_transmission=False)
        plotter = _make_plotter(model)
        proj_ax = MagicMock()

        plotter._draw_pole_figure(proj_ax, [], current_index=0)

        proj_ax.scatter.assert_any_call(np.array([0.2]), np.array([0.1]), s=20, facecolor="None", edgecolor="grey", alpha=0.5)

    def test_transmission_mode_aggregates_included_and_adds_colorbar(self):
        a = MagicMock()
        a.include = True
        a.pf_points = np.array([[0.1, 0.2]])
        a.transmission = np.array([0.3])
        b = MagicMock()
        b.include = True
        b.pf_points = np.array([[0.3, 0.4]])
        b.transmission = np.array([0.7])
        excluded = MagicMock()
        excluded.include = False
        model = _make_model(orientations={0: a, 1: b, 2: excluded}, plot_transmission=True)
        plotter = _make_plotter(model)
        proj_ax = MagicMock()
        scatt_obj = MagicMock()
        proj_ax.scatter.return_value = scatt_obj

        plotter._draw_pole_figure(proj_ax, [], current_index=0)

        scatter_call = proj_ax.scatter.call_args
        np.testing.assert_array_equal(scatter_call.args[0], np.array([0.2, 0.4]))
        np.testing.assert_array_equal(scatter_call.args[1], np.array([0.1, 0.3]))
        np.testing.assert_array_equal(scatter_call.kwargs["c"], np.array([0.3, 0.7]))
        self.assertEqual(scatter_call.kwargs["vmin"], 0)
        self.assertEqual(scatter_call.kwargs["vmax"], 1)
        self.assertEqual(scatter_call.kwargs["cmap"], "jet")
        proj_ax.inset_axes.assert_called_once_with((0.9, 0.15, 0.05, 0.7))
        proj_ax.figure.colorbar.assert_called_once_with(scatt_obj, cax=proj_ax.inset_axes.return_value)

    def test_transmission_mode_uses_data_range_when_enabled(self):
        a = MagicMock()
        a.include = True
        a.pf_points = np.array([[0.1, 0.2]])
        a.transmission = np.array([0.3])
        model = _make_model(orientations={0: a}, plot_transmission=True)
        plotter = _make_plotter(model, transmission_use_data_range=True)
        proj_ax = MagicMock()

        plotter._draw_pole_figure(proj_ax, [], current_index=0)

        scatter_call = proj_ax.scatter.call_args
        self.assertNotIn("vmin", scatter_call.kwargs)
        self.assertNotIn("vmax", scatter_call.kwargs)
        self.assertEqual(scatter_call.kwargs["cmap"], "jet")


@patch(file_path + ".plt")
class TestTexturePlotter_DecoratePoleFigure(unittest.TestCase):
    def test_sets_aspect_limits_and_axis_off(self, mock_plt):
        model = _make_model()
        plotter = _make_plotter(model)
        proj_ax = MagicMock()

        plotter._decorate_pole_figure(proj_ax)

        proj_ax.set.assert_called_once_with(xlim=(-1.5, 1.5), ylim=(-1.5, 1.5), aspect="equal")
        proj_ax.set_axis_off.assert_called_once_with()

    def test_draws_two_basis_quivers(self, mock_plt):
        model = _make_model()
        plotter = _make_plotter(model, dir_cols=["a", "b", "c"])
        proj_ax = MagicMock()

        plotter._decorate_pole_figure(proj_ax)

        self.assertEqual(
            proj_ax.quiver.call_args_list,
            [
                call(-1, -1, 1, 0, color="c", scale=5),
                call(-1, -1, 0, 1, color="a", scale=5),
            ],
        )

    def test_adds_unit_circle_patch(self, mock_plt):
        model = _make_model()
        plotter = _make_plotter(model)
        proj_ax = MagicMock()
        circle = MagicMock()
        mock_plt.Circle.return_value = circle

        plotter._decorate_pole_figure(proj_ax)

        mock_plt.Circle.assert_called_once_with((0, 0), 1, color="grey", fill=False, linestyle="-")
        proj_ax.add_patch.assert_called_once_with(circle)

    def test_annotates_with_first_and_third_dir_names(self, mock_plt):
        model = _make_model(dir_names=["A", "B", "C"])
        plotter = _make_plotter(model)
        proj_ax = MagicMock()

        plotter._decorate_pole_figure(proj_ax)

        self.assertEqual(
            proj_ax.annotate.call_args_list,
            [
                call("A", (-0.95, -0.8)),
                call("C", (-0.8, -0.95)),
            ],
        )


if __name__ == "__main__":
    unittest.main()
