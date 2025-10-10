# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
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
        cls.view.sig_restore_default_omega_offset = mock.Mock()
        cls.view.sig_restore_default_dxdy = mock.Mock()
        cls.view.sig_update_omega_offset.connect = mock.Mock()
        cls.view.sig_update_dxdy.connect = mock.Mock()
        cls.view.sig_change_colormap.connect = mock.Mock()
        cls.view.sig_change_grid.connect = mock.Mock()
        cls.view.sig_change_crystal_axes.connect = mock.Mock()
        cls.view.sig_change_font_size.connect = mock.Mock()

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
        self.presenter._plot_param = ObjectDict()
        self.presenter._plot_param.grid_state = 0
        self.presenter._plot_param.grid_helper = None
        self.presenter._plot_param.colormap_name = "jet"
        self.presenter._plot_param.font_size = 1
        self.presenter._plot_param.lines = 0

    def test___init__(self):
        self.assertIsInstance(self.presenter, DNSElasticSCPlotPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)
        self.assertTrue(hasattr(self.presenter, "_plot_param"))
        self.assertIsInstance(self.presenter._plot_param, ObjectDict)
        self.assertTrue(hasattr(self.presenter._plot_param, "grid_state"))
        self.assertTrue(hasattr(self.presenter._plot_param, "grid_helper"))
        self.assertTrue(hasattr(self.presenter._plot_param, "colormap_name"))
        self.assertTrue(hasattr(self.presenter._plot_param, "font_size"))
        self.assertTrue(hasattr(self.presenter._plot_param, "lines"))

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
        self.view.get_axis_type.return_value = {"plot_type": "type1"}
        self.view.datalist.get_checked_plots.return_value = ["plot1"]
        self.presenter.param_dict = {
            "elastic_single_crystal_script_generator": {"data_arrays": {"plot1": [1, 2, 3]}},
            "elastic_single_crystal_options": {"options"},
        }
        self.presenter._change_font_size = mock.Mock()
        self.presenter._change_grid_state = mock.Mock()
        self.presenter._want_plot = mock.Mock()
        self.presenter._set_plotting_grid = mock.Mock()
        self.presenter._crystallographical_axes = mock.Mock(return_value="grid_axes")
        self.presenter._set_axis_labels = mock.Mock()
        self.presenter._set_initial_omega_offset_dx_dy = mock.Mock()

        self.view.single_crystal_plot.create_colorbar = mock.Mock()
        self.view.single_crystal_plot.on_resize = mock.Mock()
        self.view.single_crystal_plot = mock.Mock()
        self.view.canvas = mock.Mock()
        self.view.canvas.figure = mock.Mock()
        self.view.canvas.figure.tight_layout = mock.Mock()
        self.view.draw = mock.Mock()

        self.presenter._plot(initial_values={"omega_offset": 1})
        self.presenter._change_font_size.assert_called_once_with(draw=False)
        self.model.create_single_crystal_map.assert_called_once_with([1, 2, 3], {"options"}, {"omega_offset": 1})
        self.presenter._want_plot.assert_called_once_with("type1")
        self.view.create_subfigure.assert_called_once_with(None)
        self.view.single_crystal_plot.create_colorbar.assert_called_once()
        self.view.single_crystal_plot.on_resize.assert_called_once()
        self.view.canvas.figure.tight_layout.assert_called_once()
        self.view.draw.assert_called_once()

    def test___set_initial_omega_offset_dx_dy(self):
        self.model.get_omega_offset.return_value = 3
        self.model.get_dx_dy.return_value = (1, 2)
        self.presenter._set_initial_omega_offset_dx_dy()
        self.model.get_omega_offset.assert_called_once()
        self.model.get_dx_dy.assert_called_once()
        self.view.set_initial_omega_offset_dx_dy.assert_called_once_with(3, 1, 2)
        # oof = self.model.get_omega_offset()

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter."
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
    def test__update_omega_offset(self, mock_plot):
        self.mock_view = mock.Mock()
        self.mock_view.initial_values = {"omega_offset": 0}
        self.presenter = DNSElasticSCPlotPresenter(name="test", parent=None, view=self.mock_view, model=mock.Mock())
        self.presenter._update_omega_offset(2)
        mock_plot.assert_called_once_with({"omega_offset": 2})

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    def test__update_dx_dy(self, mock_plot):
        self.mock_view = mock.Mock()
        self.mock_view.initial_values = {"dx": 0.5, "dy": 1}
        self.presenter = DNSElasticSCPlotPresenter(name="test", parent=None, view=self.mock_view, model=mock.Mock())
        self.presenter._update_dx_dy(1, 2)
        mock_plot.assert_called_once_with({"dx": 1, "dy": 2})

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
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot_triangulation"
    )
    def test__want_plot(self, mock_plot_triangulation):
        self.mock_view = mock.Mock()
        self.mock_view.get_axis_type = mock.Mock()
        self.mock_view.get_axis_type.return_value = {"type": "hkl", "interpolate": "triang", "switch": False}
        self.presenter = DNSElasticSCPlotPresenter(name="test", parent=None, view=self.mock_view, model=mock.Mock())
        self.presenter._want_plot("plot_type_value")
        self.mock_view.get_axis_type.assert_called_once_with()
        mock_plot_triangulation.assert_called_once_with("triang", "hkl", False)

    def test__get_plot_styles(self):
        self.presenter._plot_param.lines = 0
        test_v = self.presenter._get_plot_styles()
        self.view.get_state.assert_called_once()
        self.assertIsInstance(test_v[0], LinearSegmentedColormap)
        self.assertEqual(test_v[0].name, "jet_r")
        self.assertEqual(test_v[1], "face")
        self.assertEqual(test_v[2], "flat")

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._crystallographical_axes"
    )
    def test__set_axis_labels(self, mock_crystallographical_axes):
        mock_crystallographical_axes.return_value = True
        self.model.get_axis_labels.return_value = ("x", "y")
        self.presenter = DNSElasticSCPlotPresenter(name="test", parent=None, view=self.view, model=self.model)
        self.presenter._set_axis_labels()
        self.model.get_axis_labels.assert_called_once_with("hkl", True)
        self.view.single_crystal_plot.set_axis_labels.assert_called_once_with("x", "y")

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot."
        "elastic_single_crystal_plot_presenter."
        "DNSElasticSCPlotPresenter._set_plotting_grid"
    )
    def test__change_crystal_axes_grid(self, mock_set_plotting_grid):
        self.presenter._plot_param.grid_state = 0
        self.presenter._change_crystal_axes_grid()
        mock_set_plotting_grid.assert_called_once_with(crystallographic_axes=True)
        self.assertEqual(self.presenter._plot_param.grid_state, 1)
        self.presenter._plot_param.grid_state = 4
        self.presenter._change_crystal_axes_grid()
        self.assertEqual(self.presenter._plot_param.grid_state, 1)
        self.presenter._plot_param.grid_state = 2
        self.presenter._change_crystal_axes_grid()
        self.assertEqual(self.presenter._plot_param.grid_state, 2)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._set_plotting_grid"
    )
    def test__change_normal_grid(self, mock_set_plotting_grid):
        self.presenter._plot_param.grid_state = 1
        self.presenter._change_normal_grid()
        mock_set_plotting_grid.assert_called_once_with(crystallographic_axes=False)
        self.assertEqual(self.presenter._plot_param.grid_state, 1)
        self.presenter._plot_param.grid_state = 3
        self.view.reset_mock()
        self.presenter._change_normal_grid()
        self.assertEqual(self.presenter._plot_param.grid_state, 0)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._create_grid_helper"
    )
    def test_set_plotting_grid_with_crystallographic_axes(self, mock_create_grid_helper):
        self.mock_view = mock.Mock()
        self.mock_view.single_crystal_plot = mock.Mock()
        self.presenter = DNSElasticSCPlotPresenter(name="test", parent=None, view=self.mock_view, model=mock.Mock())
        self.presenter._plot_param.grid_state = 6
        mock_create_grid_helper.return_value = "grid_helper"
        self.presenter._set_plotting_grid(crystallographic_axes=True)
        self.assertEqual(self.presenter._plot_param.grid_helper, "grid_helper")
        self.mock_view.single_crystal_plot.set_grid.assert_called_once_with(major=6, minor=2)

    @patch(
        "mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._create_grid_helper"
    )
    def test_set_plotting_grid_without_crystallographic_axes(self, mock_create_grid_helper):
        self.mock_view = mock.Mock()
        self.mock_view.single_crystal_plot = mock.Mock()
        self.presenter = DNSElasticSCPlotPresenter(name="test", parent=None, view=self.mock_view, model=mock.Mock())
        self.presenter._plot_param.grid_state = 5
        self.presenter._set_plotting_grid(crystallographic_axes=False)
        self.assertIsNone(self.presenter._plot_param.grid_helper)
        mock_create_grid_helper.assert_not_called()
        self.mock_view.single_crystal_plot.set_grid.assert_called_once_with(major=5, minor=2)

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
        self.view.reset_mock()
        self.view.get_state.return_value["crystal_axes"] = True
        self.presenter._change_grid_state(draw=False)
        self.view.draw.assert_not_called()

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    def test__change_crystal_axes(self, mock_plot):
        self.presenter._change_crystal_axes()
        self.assertEqual(self.presenter._plot_param.grid_state, 0)
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

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_presenter.DNSElasticSCPlotPresenter._plot")
    def test__change_font_size(self, mock_plot):
        self.presenter._plot_param.font_size = 0
        self.presenter._change_font_size(draw=True)
        self.view.get_state.assert_called_once()
        self.assertEqual(self.presenter._plot_param.font_size, 18)
        self.view.single_crystal_plot.set_fontsize.assert_called_once_with(18)
        mock_plot.assert_called_once()
        self.view.reset_mock()
        mock_plot.reset_mock()
        self.presenter._plot_param.font_size = 18
        self.presenter._change_font_size(draw=True)
        self.view.single_crystal_plot.set_fontsize.assert_not_called()
        mock_plot.assert_not_called()
        self.presenter._plot_param.font_size = 0
        self.presenter._change_font_size(draw=False)
        mock_plot.assert_not_called()


if __name__ == "__main__":
    unittest.main()
