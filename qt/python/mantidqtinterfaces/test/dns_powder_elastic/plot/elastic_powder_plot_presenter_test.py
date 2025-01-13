# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from unittest.mock import patch

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model import DNSElasticPowderPlotModel
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_presenter import DNSElasticPowderPlotPresenter
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_view import DNSElasticPowderPlotView


class DNSElasticPowderPlotPresenterTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods

    view = None
    model = None
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.view = mock.create_autospec(DNSElasticPowderPlotView, instance=True)
        cls.view.sig_plot.connect = mock.Mock()
        cls.view.sig_grid_state_change.connect = mock.Mock()
        cls.view.sig_error_bar_change.connect = mock.Mock()
        cls.view.sig_linestyle_change.connect = mock.Mock()
        cls.view.sig_log_change.connect = mock.Mock()
        cls.view.get_check_plots.return_value = [0, 1]
        cls.view.get_data_list.return_value = [3, 4]
        cls.model = mock.create_autospec(DNSElasticPowderPlotModel, instance=True)
        cls.model.get_x_y_yerr.return_value = [[1, 2], [3, 4], [4, 5]]
        cls.model.get_updated_ws_list.return_value = [["mat_test"], True]
        cls.presenter = DNSElasticPowderPlotPresenter(view=cls.view, model=cls.model, parent=cls.parent)
        cls.param_dict = {
            "elastic_powder_options": {"separation": True, "wavelength": 4.74, "norm_monitor": True},
            "elastic_powder_script_generator": {"subtract": [0, 1], "script_number": 0},
        }

    def setUp(self):
        self.model.reset_mock()
        self.view.reset_mock()
        self.presenter.errorbar = 0
        self.presenter.gridstate = 0
        self.presenter.linestyle = 0
        self.presenter.param_dict = self.param_dict

    def test___init__(self):
        self.presenter = DNSElasticPowderPlotPresenter(view=self.view, model=self.model, parent=self.parent)
        self.assertIsInstance(self.presenter, DNSElasticPowderPlotPresenter)
        self.assertIsInstance(self.presenter, DNSObserver)

        self.view.sig_plot.connect.assert_called_once_with(self.presenter._plot)
        self.view.sig_grid_state_change.connect.assert_called_once_with(self.presenter._change_grid_state)
        self.view.sig_error_bar_change.connect.assert_called_once_with(self.presenter._change_error_bar)
        self.view.sig_linestyle_change.connect.assert_called_once_with(self.presenter._change_line_style)
        self.view.sig_log_change.connect.assert_called_once_with(self.presenter._change_log)
        self.assertEqual(self.presenter._error_bar, 2)
        self.assertEqual(self.presenter._grid_state, 0)
        self.assertEqual(self.presenter._line_style, 2)

    def test_change_log(self):
        self.presenter._change_log(True)
        self.view.set_y_scale.assert_called_once_with("symlog")
        self.view.reset_mock()
        self.presenter._change_log(False)
        self.view.set_y_scale.assert_called_once_with("linear")

    @patch("mantidqtinterfaces.dns_powder_elastic.plot." "elastic_powder_plot_presenter." "DNSElasticPowderPlotPresenter._plot")
    def test_change_linestyle(self, mock_plot):
        self.presenter._change_line_style()
        self.assertEqual(self.presenter._line_style, 0)
        mock_plot.assert_called_once()
        self.presenter._change_line_style()
        self.assertEqual(self.presenter._line_style, 1)
        self.presenter._change_line_style()
        self.assertEqual(self.presenter._line_style, 2)

    @patch("mantidqtinterfaces.dns_powder_elastic.plot." "elastic_powder_plot_presenter." "DNSElasticPowderPlotPresenter._plot")
    def test_change_errorbar(self, mock_plot):
        self.presenter._change_error_bar()
        self.assertEqual(self.presenter._error_bar, 0)
        mock_plot.assert_called_once()
        self.presenter._change_error_bar()
        self.assertEqual(self.presenter._error_bar, 1)
        self.presenter._change_error_bar()
        self.assertEqual(self.presenter._error_bar, 2)

    def test_change_gridstate(self):
        self.presenter._change_grid_state(False)
        self.view.set_no_grid.assert_called_once()
        self.assertEqual(self.presenter._grid_state, 0)
        self.presenter._change_grid_state(True)
        self.view.set_major_grid.assert_called_once()
        self.assertEqual(self.presenter._grid_state, 1)
        self.presenter._change_grid_state(True)
        self.view.set_major_minor_grid.assert_called_once()
        self.assertEqual(self.presenter._grid_state, 2)
        self.view.reset_mock()
        self.presenter._change_grid_state(True)
        self.view.set_no_grid.assert_called_once()
        self.assertEqual(self.presenter._grid_state, 0)
        self.view.draw.assert_called_once()

    def test_plot(self):
        self.presenter._plot()
        self.view.get_check_plots.assert_called_once()
        self.view.get_x_axis.assert_called_once()
        self.model.get_x_axis_label.assert_called_once()
        self.view.create_plot.assert_called_once()
        self.model.get_y_norm_label.assert_called_once()
        self.model.get_max_int_of_workspaces.assert_called_once()
        self.assertEqual(self.model.get_x_y_yerr.call_count, 2)
        self.assertEqual(self.view.single_error_plot.call_count, 2)
        self.view.finish_plot.assert_called_once()

    def test_single_plot(self):
        self.presenter._single_plot(" 1_", 2, 3, 4)
        self.view.single_error_plot.assert_called_once_with(2, 3, 4, label="1", capsize=3, linestyle=2)
        self.presenter._error_bar = 0
        self.presenter._single_plot(" 1_", 2, 3, 4)
        self.view.single_plot.assert_called_once_with(2, 3, label="1", linestyle=2)

    def test_auto_select_curve(self):
        self.presenter._auto_select_curve()
        self.view.check_separated.assert_called_once()
        self.presenter.param_dict["elastic_powder_options"]["separation"] = False
        self.presenter._auto_select_curve()
        self.view.check_first.assert_called_once()
        self.view.check_separated.assert_called_once()

    def test_get_wavelength(self):
        self.assertEqual(self.presenter._get_wavelength(), 4.74)

    def test_get_norm(self):
        self.assertTrue(self.presenter._get_norm())

    def test_get_workspaces(self):
        self.assertEqual(self.presenter._get_workspaces(), [0, 1])

    def test_get_scriptnumber(self):
        self.assertEqual(self.presenter._get_script_number(), 0)

    def test_tab_got_focus(self):
        self.presenter.tab_got_focus()
        self.view.get_data_list.assert_called_once()
        self.model.get_updated_ws_list.assert_called_once_with([0, 1], [3, 4], 0)
        self.view.set_data_list.assert_called_once_with(["test"])
        self.view.start_timer.assert_called_once()

    def test_process_auto_reduction_request(self):
        self.presenter.process_auto_reduction_request()
        self.view.clear_plot.assert_called_once()


if __name__ == "__main__":
    unittest.main()
