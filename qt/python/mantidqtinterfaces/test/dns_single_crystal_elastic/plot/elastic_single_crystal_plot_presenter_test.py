# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from unittest.mock import patch
from matplotlib.colors import LinearSegmentedColormap
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter import DNSElasticSCPlotPresenter
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_view import DNSElasticSCPlotView
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model import DNSElasticSCPlotModel
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_plot import DNSScPlot
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_datalist import DNSDatalist


class DNSElasticSCPlotPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.view = mock.create_autospec(DNSElasticSCPlotView)
        cls.view.single_crystal_plot = mock.create_autospec(DNSScPlot)
        cls.view.datalist = mock.create_autospec(DNSDatalist)
        cls.view.sig_plot.connect = mock.Mock()
        cls.view.sig_update_omega_offset.connect = mock.Mock()
        cls.view.sig_update_dxdy.connect = mock.Mock()
        cls.view.sig_calculate_projection.connect = mock.Mock()
        cls.view.sig_change_colormap.connect = mock.Mock()
        cls.view.sig_change_log.connect = mock.Mock()
        cls.view.sig_change_linestyle.connect = mock.Mock()
        cls.view.sig_change_cb_range_on_zoom.connect = mock.Mock()
        cls.view.sig_change_cb_range_on_zoom.disconnect = mock.Mock()
        cls.view.sig_manual_lim_changed.connect = mock.Mock()
        cls.view.sig_change_grid.connect = mock.Mock()
        cls.view.sig_change_crystal_axes.connect = mock.Mock()
        cls.view.sig_change_fontsize.connect = mock.Mock()
        cls.view.sig_home_button_clicked.connect = mock.Mock()

        cls.model = mock.create_autospec(DNSElasticSCPlotModel)
        cls.presenter = DNSElasticSCPlotPresenter(view=cls.view, model=cls.model, parent=parent)

    def setUp(self):
        self.view.reset_mock()
        self.model.reset_mock()
        self.view.get_axis_type.return_value = {
            "plot_type": "quasmesh",
            "type": "hkl",
            "switch": False,
            "interpolate": False,
            "shading": 0,
            "fix_aspect": False,
            "zoom": {"fix_xy": False, "fix_z": False},
        }

        self.view.get_state.return_value = {
            "colormap": "jet",
            "fontsize": 18,
            "crystal_axes": False,
            "x_range": [1, 2],
            "y_range": [3, 4],
            "z_range": [5, 6],
            "log_scale": False,
            "invert_cb": True,
        }
        self.presenter._plot_param.xlim = None
        self.presenter._plot_param.ylim = None
        self.presenter._plot_param.zlim = None
        self.presenter._plot_param.sny_zoom_in = None
        self.presenter._plot_param.projections = False

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSElasticSCPlotPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)
        self.assertTrue(hasattr(self.presenter, "_plotted_script_number"))
        self.assertTrue(hasattr(self.presenter, "_plot_param"))
        self.assertIsInstance(self.presenter._plot_param, ObjectDict)
        self.assertTrue(hasattr(self.presenter._plot_param, "grid_state"))
        self.assertTrue(hasattr(self.presenter._plot_param, "grid_helper"))
        self.assertTrue(hasattr(self.presenter._plot_param, "colormap_name"))
        self.assertTrue(hasattr(self.presenter._plot_param, "fontsize"))
        self.assertTrue(hasattr(self.presenter._plot_param, "lines"))
        self.assertTrue(hasattr(self.presenter._plot_param, "pointsize"))
        self.assertTrue(hasattr(self.presenter._plot_param, "xlim"))
        self.assertTrue(hasattr(self.presenter._plot_param, "ylim"))
        self.assertTrue(hasattr(self.presenter._plot_param, "zlim"))
        self.assertTrue(hasattr(self.presenter._plot_param, "projections"))

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._calculate_projections"
    )
    def test__toggle_projections(self, mock_calculate_proj, mock_plot):
        mock_calculate_proj.return_value = (1, 2)
        # self.view.get_axis_type.return_value = {'switch': False}
        self.presenter._toggle_projections(False)
        self.view.get_axis_type.assert_called_once()
        self.view.single_crystal_plot.remove_projections.assert_called_once()
        mock_plot.assert_called_once()

        self.view.reset_mock()
        mock_plot.reset_mock()
        self.presenter._toggle_projections(True)
        mock_calculate_proj.assert_called_once()
        self.view.draw.assert_called_once()
        self.view.single_crystal_plot.set_projections.assert_called_once_with(1, 2)

        self.view.get_axis_type.return_value = {"switch": True}
        self.view.reset_mock()
        self.presenter._toggle_projections(True)
        self.view.single_crystal_plot.set_projections.assert_called_once_with(2, 1)
        self.view.draw.assert_called_once()
        mock_plot.assert_not_called()

    def test__calculate_projections(self):
        self.view.single_crystal_plot.get_active_limits.return_value = (1, 2)
        self.model.get_projections.return_value = (3, 4)
        test_v = self.presenter._calculate_projections(False)
        self.model.get_projections.assert_called_once_with(1, 2)
        self.assertEqual(test_v, (3, 4))
        self.model.get_projections.reset_mock()
        self.presenter._calculate_projections(True)
        self.model.get_projections.assert_called_once_with(2, 1)

    def test__datalist_updated(self):
        self.view.datalist.get_datalist.return_value = None
        self.presenter._plotted_script_number = 2
        self.presenter.param_dict = {"elastic_single_crystal_script_generator": {"script_number": 2}}
        test_v = self.presenter._datalist_updated(["1"])
        self.view.datalist.get_datalist.assert_called_once()
        self.assertTrue(test_v)
        test_v = self.presenter._datalist_updated(None)
        self.assertFalse(test_v)
        self.presenter._plotted_script_number = 1
        test_v = self.presenter._datalist_updated(None)
        self.assertTrue(test_v)

    def test__plot(self):
        self.presenter.param_dict = {
            "elastic_single_crystal_script_generator": {"data_arrays": {"a": [1, 2, 3]}},
            "elastic_single_crystal_options": {},
        }
        self.view.datalist.get_checked_plots.return_value = ["a"]
        self.model.get_axis_labels.return_value = ("c", "d")
        self.model.get_m_limits.return_value = ([2, 3], [3, 4])
        self.model.get_mz_limit.return_value = [9, 10]
        self.model.get_data_xy_lim.return_value = ([2, 3], [4, 5])
        self.model.get_dx_dy.return_value = (1, 2)
        self.model.get_data_z_min_max.return_value = (7, 8, 9)
        self.presenter._plot(initial_values="x")
        self.assertEqual(self.view.get_axis_type.call_count, 9)
        self.view.datalist.get_checked_plots.assert_called_once()
        self.model.create_single_crystal_map.assert_called_once_with([1, 2, 3], {}, "x")
        self.view.create_subfigure.assert_called_once()
        self.view.single_crystal_plot.create_colorbar.assert_called_once()
        self.view.connect_resize.assert_called_once()
        self.view.single_crystal_plot.on_resize.assert_called_once()
        self.view.single_crystal_plot.connect_ylim_change.assert_called_once()
        self.view.draw.assert_called_once()

    def test___set_initial_oof_dxdy(self):
        self.model.get_omega_offset.return_value = 3
        self.model.get_dx_dy.return_value = (1, 2)
        self.presenter._set_initial_omega_offset_dx_dy()
        self.model.get_omega_offset.assert_called_once()
        self.model.get_dx_dy.assert_called_once()
        self.view.set_initial_omega_offset_dx_dy.assert_called_once_with(3, 1, 2)
        # oof = self.model.get_omega_offset()

    def test_process_auto_reduction_request(self):
        self.presenter.process_auto_reduction_request()
        self.view.single_crystal_plot.clear_plot.assert_called_once()

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._datalist_updated"
    )
    def test_tab_got_focus(self, mock_datalist_updated):
        self.presenter.param_dict = {"elastic_single_crystal_script_generator": {"plot_list": ["b", "a"], "script_number": 0}}
        mock_datalist_updated.return_value = True
        self.presenter.tab_got_focus()
        mock_datalist_updated.assert_called_once_with(["a", "b"])
        self.view.datalist.set_datalist.assert_called_once_with(["a", "b"])
        self.view.process_events.assert_called_once()
        self.view.datalist.check_first.assert_called_once()
        self.view.reset_mock()
        mock_datalist_updated.return_value = False
        self.presenter.tab_got_focus()
        self.view.process_events.assert_not_called()
        self.view.datalist.check_first.assert_not_called()

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    def test__update_omegaoffset(self, mock_plot):
        self.presenter._update_omega_offset(2)
        mock_plot.assert_called_once_with({"omega_offset": 2})

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    def test__update_dx_dy(self, mock_plot):
        self.presenter._update_dx_dy(1, 2)
        mock_plot.assert_called_once_with({"dx": 1, "dy": 2})

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_plot_styles"
    )
    def test__plot_quadmesh(self, mock_get_plot_styles):
        mock_get_plot_styles.return_value = ("jet", False, "flat")
        self.model.get_interpolated_quadmesh.return_value = (1, 2, 3)
        self.model.switch_axis.return_value = (1, 2, 3)
        self.presenter._plot_quadmesh(True, {}, False)
        self.model.get_interpolated_quadmesh.assert_called_once_with(True, {})
        self.model.switch_axis.assert_called_once_with(1, 2, 3, False)
        self.view.single_crystal_plot.plot_quadmesh.assert_called_once_with(1, 2, 3, "jet", False, "nearest")

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_plot_styles"
    )
    def test_plot_triangulation(self, mock_get_plot_styles):
        mock_get_plot_styles.return_value = ("jet", False, "flat")
        self.model.get_interpolated_triangulation.return_value = (1, 2)
        self.presenter._plot_triangulation(False, {}, True)
        self.model.get_interpolated_triangulation.assert_called_once_with(False, {}, True)
        self.view.single_crystal_plot.plot_triangulation.assert_called_once_with(1, 2, "jet", False, "flat")

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_plot_styles"
    )
    def test__plot_scatter(self, mock_get_plot_styles):
        mock_get_plot_styles.return_value = ("jet", False, "flat")
        self.model.get_interpolated_quadmesh.return_value = (1, 2, 3)
        self.model.switch_axis.return_value = (1, 2, 3)
        self.presenter._plot_scatter({}, True)
        self.model.get_interpolated_quadmesh.assert_called_once_with(False, {})
        self.model.switch_axis.assert_called_once_with(1, 2, 3, True)
        self.view.single_crystal_plot.plot_scatter.assert_called_once_with(1, 2, 3, "jet")

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot_quadmesh"
    )
    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._plot_triangulation"
    )
    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot_scatter"
    )
    def test__want_plot(self, mock_scatter, mock_triangulation, mock_quadmesh):
        self.presenter._want_plot("quadmesh")
        mock_quadmesh.assert_called_once_with(False, "hkl", False)
        self.presenter._want_plot("triangulation")
        mock_triangulation.assert_called_once_with(False, "hkl", False)
        self.presenter._want_plot("scatter")
        mock_scatter.assert_called_once_with("hkl", False)

    def test__get_plot_styles(self):
        self.presenter._plot_param.lines = 0
        test_v = self.presenter._get_plot_styles()
        self.view.get_state.assert_called_once()
        self.view.get_axis_type.assert_called_once()
        self.assertIsInstance(test_v[0], LinearSegmentedColormap)
        self.assertEqual(test_v[0].name, "jet_r")
        self.assertEqual(test_v[1], "face")
        self.assertEqual(test_v[2], 0)

    def test__set_axis_labels(self):
        self.model.get_axis_labels.return_value = (1, 2)
        self.presenter._set_axis_labels()
        self.view.get_state.assert_called_once()
        self.view.get_axis_type.assert_called_once()
        self.model.get_axis_labels.assert_called_once_with("hkl", False)
        self.view.single_crystal_plot.set_axis_labels.assert_called_once_with(1, 2)
        self.view.reset_mock()
        self.view.get_axis_type.return_value["switch"] = True
        self.presenter._set_axis_labels()
        self.view.single_crystal_plot.set_axis_labels.assert_called_once_with(2, 1)

    def test__set_aspect_ratio(self):
        self.model.get_aspect_ratio.return_value = 3
        self.presenter._set_aspect_ratio()
        self.view.get_axis_type.assert_called_once()
        self.model.get_aspect_ratio.assert_called_once()
        self.view.single_crystal_plot.set_aspect_ratio.assert_called_once_with(3)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._create_grid_helper"
    )
    def test__change_crystal_axes_grid(self, mock_grid_helper):
        self.presenter._plot_param.grid_state = 1
        self.presenter._change_crystal_axes_grid()
        mock_grid_helper.assert_called_once()
        self.view.single_crystal_plot.set_grid.assert_called_once_with(major=True)
        self.assertEqual(self.presenter._plot_param.grid_state, 1)
        self.presenter._plot_param.grid_state = 4
        self.presenter._change_crystal_axes_grid()
        self.assertEqual(self.presenter._plot_param.grid_state, 0)

    def test_change_normal_grid(self):
        self.presenter._plot_param.grid_helper = 1
        self.presenter._plot_param.grid_state = 1
        self.presenter._change_normal_grid()
        self.view.single_crystal_plot.set_grid.assert_called_once_with(major=1, minor=0)
        self.assertEqual(self.presenter._plot_param.grid_state, 1)
        self.presenter._plot_param.grid_state = 3
        self.view.reset_mock()
        self.presenter._change_normal_grid()
        self.assertEqual(self.presenter._plot_param.grid_state, 0)
        self.assertIsNone(self.presenter._plot_param.grid_helper, 0)
        self.view.single_crystal_plot.set_grid.assert_called_once_with(major=0, minor=0)
        self.presenter._plot_param.grid_state = 2
        self.view.reset_mock()
        self.presenter._change_normal_grid()
        self.view.single_crystal_plot.set_grid.assert_called_once_with(major=2, minor=1)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._change_normal_grid"
    )
    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._change_crystal_axes_grid"
    )
    def test__change_grid_state(self, mock_cg, mock_ng):
        self.presenter._change_grid_state(draw=True)
        self.view.get_state.assert_called_once()
        self.view.draw.assert_called_once()
        mock_ng.assert_called_once()
        self.view.reset_mock()
        self.view.get_state.return_value["crystal_axes"] = True
        self.presenter._change_grid_state(draw=False)
        self.view.draw.assert_not_called()
        mock_cg.assert_called_once()

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    def test__change_crystal_axes(self, mock_plot):
        self.presenter._change_crystal_axes()
        self.assertEqual(self.presenter._plot_param.grid_state, 1)
        mock_plot.assert_called_once()

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.get_grid_helper")
    def test__create_grid_helper(self, mock_get_grid_helper):
        self.presenter._plot_param.grid_helper = None
        self.presenter._plot_param.grid_state = 0
        self.model.get_changing_hkl_components.return_value = 1, 2, 3, 4
        self.presenter._create_grid_helper()
        self.view.get_axis_type.assert_called_once()
        mock_get_grid_helper.assert_called_once_with(None, 0, 1, 2, 3, 4, False)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_plot_styles"
    )
    def test__set_colormap(self, mock_get_plot_styles):
        mock_get_plot_styles.return_value = ("jet", False, "flat")
        self.presenter._set_colormap()
        mock_get_plot_styles.assert_called_once()
        self.view.single_crystal_plot.set_cmap.assert_called_once_with("jet")
        self.view.draw.assert_called_once()

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.mpl_helpers.get_log_norm")
    def test__set_log(self, mock_get_log_norm):
        mock_get_log_norm.return_value = 1
        self.presenter._set_log()
        self.assertEqual(self.view.get_state.call_count, 3)
        self.view.single_crystal_plot.set_norm.assert_called_once_with(1)
        self.view.draw.assert_called_once()
        mock_get_log_norm.assert_called_once_with(False, [9, 10])

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    def test__change_fontsize(self, mock_plot):
        self.presenter._plot_param.fontsize = 0
        self.presenter._change_fontsize(draw=True)
        self.view.get_state.assert_called_once()
        self.assertEqual(self.presenter._plot_param.fontsize, 18)
        self.view.single_crystal_plot.set_fontsize.assert_called_once_with(18)
        mock_plot.assert_called_once()
        self.view.reset_mock()
        mock_plot.reset_mock()
        self.presenter._plot_param.fontsize = 18
        self.presenter._change_fontsize(draw=True)
        self.view.single_crystal_plot.set_fontsize.assert_not_called()
        mock_plot.assert_not_called()
        self.presenter._plot_param.fontsize = 0
        self.presenter._change_fontsize(draw=False)
        mock_plot.assert_not_called()

    def test__change_linestyle(self):
        self.presenter._plot_param.pointsize = 0
        self.view.get_axis_type.return_value["plot_type"] = "scatter"
        self.presenter._change_linestyle()
        self.view.get_axis_type.assert_called_once()
        self.view.single_crystal_plot.set_pointsize.assert_called_once_with(1)
        self.view.reset_mock()
        self.presenter._plot_param.pointsize = 4
        self.presenter._change_linestyle()
        self.view.single_crystal_plot.set_pointsize.assert_called_once_with(0)
        self.view.reset_mock()
        self.presenter._plot_param.lines = 0
        self.view.get_axis_type.return_value["plot_type"] = "quadmesh"
        self.presenter._change_linestyle()
        self.view.single_crystal_plot.set_linecolor.assert_called_once_with(1)
        self.presenter._plot_param.lines = 2
        self.view.get_axis_type.return_value["plot_type"] = "quadmesh"
        self.view.reset_mock()
        self.presenter._change_linestyle()
        self.view.single_crystal_plot.set_linecolor.assert_called_once_with(0)
        self.view.draw.assert_called_once()

    def test__set_ax_formatter(self):
        self.model.get_format_coord.return_value = 123
        self.presenter._set_ax_formatter()
        self.view.get_axis_type.assert_called_once()
        self.model.get_format_coord.assert_called_once()
        self.view.single_crystal_plot.set_format_coord.assert_called_once_with(123)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._change_color_bar_range"
    )
    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_current_limits"
    )
    def test__manual_lim_changed(self, mock_get_current_limits, mock_change_cb_range):
        mock_get_current_limits.return_value = [1, 2], [3, 4], 1, 1
        self.presenter._manual_lim_changed()
        self.view.single_crystal_plot.set_xlim.assert_called_once_with([1, 2])
        self.view.single_crystal_plot.set_ylim.assert_called_once_with([3, 4])
        mock_change_cb_range.assert_called_once_with(zoom=False)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._toggle_projections"
    )
    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_current_limits"
    )
    def test__change_cb_range(self, mock_get_current_limits, mock_tg_proj):
        self.view.datalist.get_checked_plots.return_value = 3
        self.presenter._plot_param.projections = False

        self.presenter._plot_param.xlim = None
        self.presenter._plot_param.ylim = None
        self.presenter._plot_param.zlim = None
        self.presenter._plot_param.sny_zoom_in = None
        mock_get_current_limits.return_value = [1, 2], [3, 4], 5, 6
        self.presenter._change_color_bar_range(zoom=True)
        mock_get_current_limits.assert_called_once_with(True)
        self.view.single_crystal_plot.set_zlim.assert_called_once_with(5)
        self.assertEqual(self.presenter._plot_param.xlim, [1, 2])
        self.assertEqual(self.presenter._plot_param.ylim, [3, 4])
        self.assertEqual(self.presenter._plot_param.zlim, 5)
        self.assertEqual(self.presenter._plot_param.sny_zoom_in, 3)
        self.view.datalist.get_checked_plots.assert_called_once()
        mock_tg_proj.assert_not_called()
        self.view.draw.assert_called_once()
        self.view.reset_mock()
        self.presenter._plot_param.projections = True
        self.presenter._change_color_bar_range(zoom=False)
        mock_tg_proj.assert_called_once()
        self.view.draw.assert_not_called()

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._change_color_bar_range"
    )
    def test__home_button_clicked(self, mock_change_cb_range):
        self.view.get_state.return_value["log_scale"] = True
        self.view.get_axis_type.return_value["zoom"]["fix_xy"] = True
        self.model.get_data_xy_lim.return_value = 1, 2
        self.presenter._home_button_clicked()
        self.view.single_crystal_plot.disconnect_ylim_change.assert_called_once()
        self.view.get_state.assert_called_once()
        self.view.get_axis_type.assert_called_once()
        self.view.single_crystal_plot.redraw_colorbar.assert_called_once()
        self.model.get_data_xy_lim.assert_called_once()
        self.view.single_crystal_plot.set_xlim.assert_called_once_with(1)
        self.view.single_crystal_plot.set_ylim.assert_called_once_with(2)
        self.assertEqual(self.presenter._plot_param.xlim, 1)
        self.assertEqual(self.presenter._plot_param.ylim, 2)
        mock_change_cb_range.assert_called_once()
        self.view.single_crystal_plot.connect_ylim_change.assert_called_once()

    def test__get_current_xy_lim(self):
        self.model.get_m_limits.return_value = [1, 2], [3, 4]
        self.model.get_data_xy_lim.return_value = [5, 6], [7, 8]
        self.presenter._plot_param.xlim = [10, 11]
        self.presenter._plot_param.ylim = [12, 13]

        test_v = self.presenter._get_current_xy_lim(zoom=False)
        self.view.get_state.assert_called_once()
        self.view.get_axis_type.assert_called_once()
        self.model.get_m_limits.assert_called_once_with([1, 2], [3, 4])
        self.model.get_data_xy_lim.assert_called_once_with(False)
        self.assertEqual(test_v, ([1, 2], [3, 4]))

        self.model.get_m_limits.return_value = [None, None], [None, None]
        test_v = self.presenter._get_current_xy_lim(zoom=False)
        self.assertEqual(test_v, ([5, 6], [7, 8]))

        self.view.single_crystal_plot.get_active_limits.return_value = [22, 23], [24, 25]
        self.model.get_m_limits.return_value = [None, None], [1, 2]
        test_v = self.presenter._get_current_xy_lim(zoom=True)
        self.view.single_crystal_plot.get_active_limits.assert_called_once()
        self.assertEqual(test_v, ([22, 23], [1, 2]))

        self.view.get_axis_type.return_value["zoom"]["fix_xy"] = True
        test_v = self.presenter._get_current_xy_lim(zoom=False)
        self.assertEqual(test_v, ([10, 11], [12, 13]))

    def test__get_current_z_lim(self):
        self.model.get_data_z_min_max.return_value = 11, 12, 13
        self.model.get_mz_limit.return_value = [4, 5]
        test_v = self.presenter._get_current_z_lim([0, 1], [2, 3], False)
        self.view.get_state.assert_called_once()
        self.view.get_axis_type.assert_called_once()
        self.model.get_data_z_min_max.assert_called_once_with([0, 1], [2, 3])
        self.model.get_mz_limit.assert_called_once_with([5, 6])
        self.assertEqual(test_v, ([4, 5], True))

        self.model.reset_mock()
        self.view.get_axis_type.return_value["switch"] = True
        self.model.get_mz_limit.return_value = [None, 5]
        test_v = self.presenter._get_current_z_lim([0, 1], [2, 3], False)
        self.model.get_data_z_min_max.assert_called_once_with([2, 3], [0, 1])
        self.assertEqual(test_v, ([11, 12], False))

        self.model.get_mz_limit.return_value = [-2, -3]
        test_v = self.presenter._get_current_z_lim([0, 1], [2, 3], False)
        self.assertEqual(test_v, ([-2, -3], True))
        self.view.get_axis_type.return_value["zoom"]["fix_z"] = True
        self.view.get_state.return_value["log_scale"] = True
        test_v = self.presenter._get_current_z_lim([0, 1], [2, 3], True)
        self.assertEqual(test_v, ([13, -3], True))

        self.presenter._plot_param.zlim = [10, 20]
        test_v = self.presenter._get_current_z_lim([0, 1], [2, 3], False)
        self.assertEqual(test_v, ([10, 20], True))

        self.presenter._plot_param.zlim = [None, None]
        test_v = self.presenter._get_current_z_lim([0, 1], [2, 3], False)
        self.assertEqual(test_v, ([13, -3], True))

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_current_z_lim"
    )
    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._get_current_xy_lim"
    )
    def test__get_current_limits(self, mock_get_current_xy_lim, mock_get_current_z_lim):
        mock_get_current_xy_lim.return_value = 1, 2
        mock_get_current_z_lim.return_value = 3, True
        test_v = self.presenter._get_current_limits(False)
        mock_get_current_xy_lim.assert_called_once_with(False)
        mock_get_current_z_lim.assert_called_once_with(1, 2, False)
        self.assertEqual(test_v, (1, 2, 3, True))


if __name__ == "__main__":
    unittest.main()
