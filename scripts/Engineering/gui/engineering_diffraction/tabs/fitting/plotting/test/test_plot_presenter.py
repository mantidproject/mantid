# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from Engineering.gui.engineering_diffraction.tabs.fitting.plotting import plot_model, plot_view, plot_presenter

dir_path = "Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_presenter"


class FittingPlotPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(plot_model.FittingPlotModel)
        self.view = mock.create_autospec(plot_view.FittingPlotView)
        self.presenter = plot_presenter.FittingPlotPresenter(None, self.model, self.view)

    def test_add_workspace_to_plot(self):
        self.view.get_axes.return_value = ["axis1", "axis2"]

        self.presenter.add_workspace_to_plot("workspace")

        self.assertEqual(1, self.view.update_figure.call_count)
        self.assertEqual(2, self.model.add_workspace_to_plot.call_count)
        self.model.add_workspace_to_plot.assert_any_call("workspace", "axis1", plot_presenter.PLOT_KWARGS)
        self.model.add_workspace_to_plot.assert_any_call("workspace", "axis2", plot_presenter.PLOT_KWARGS)

    def test_remove_workspace_from_plot(self):
        self.view.get_axes.return_value = ["axis1", "axis2"]

        self.presenter.remove_workspace_from_plot("workspace")

        self.assertEqual(1, self.view.update_figure.call_count)
        self.assertEqual(2, self.view.remove_ws_from_fitbrowser.call_count)
        self.assertEqual(2, self.model.remove_workspace_from_plot.call_count)
        self.model.remove_workspace_from_plot.assert_any_call("workspace", "axis1")
        self.model.remove_workspace_from_plot.assert_any_call("workspace", "axis2")

    def test_clear_plot(self):
        self.view.get_axes.return_value = ["axis1", "axis2"]

        self.presenter.clear_plot()

        self.assertEqual(1, self.view.clear_figure.call_count)
        self.assertEqual(1, self.view.update_fitbrowser.call_count)
        self.assertEqual(2, self.model.remove_all_workspaces_from_plot.call_count)
        self.model.remove_all_workspaces_from_plot.assert_any_call("axis1")
        self.model.remove_all_workspaces_from_plot.assert_any_call("axis2")

    @mock.patch(dir_path + '.Fit')
    def test_do_sequential_fit(self, mock_fit):
        self.fit_all_helper(self, mock_fit, do_sequential=True)

    @mock.patch(dir_path + '.Fit')
    def test_do_serial_fit(self, mock_fit):
        self.fit_all_helper(self, mock_fit, do_sequential=False)

    @mock.patch(dir_path + '.Fit')
    def test_do_sequential_fit_does_not_use_failed_fit_as_input(self, mock_fit):
        ws_list = ['ws1', 'ws2']
        fun_str_list = ['name=Gaussian,Height=11,PeakCentre=30000,Sigma=40',  # initial
                        'name=Gaussian,Height=10,PeakCentre=35000,Sigma=50']  # fit result of ws1 (and ws2)
        self.view.read_fitprop_from_browser.return_value = {'properties': {'Function': fun_str_list[0]}}  # initial
        mock_fit_output = mock.MagicMock()
        mock_fit_output.OutputStatus = "fail"
        mock_fit_output.Function.fun = fun_str_list[1]
        mock_fit.return_value = mock_fit_output

        self.presenter.do_fit_all(ws_list, do_sequential=True)

        self.assertEqual(mock_fit.call_count, len(ws_list))
        for iws, ws in enumerate(ws_list):
            # check calls to fit use initial guess for both workspaces as first fit was unsuccessful
            _, _, kwargs = mock_fit.mock_calls[iws]
            self.assertEquals(kwargs, {'Function': fun_str_list[0], 'InputWorkspace': ws, 'Output': ws})

    def fit_all_helper(self, mock_fit, do_sequential):
        ws_list = ['ws1', 'ws2']
        fun_str_list = ['name=Gaussian,Height=11,PeakCentre=30000,Sigma=40',  # initial
                        'name=Gaussian,Height=10,PeakCentre=35000,Sigma=50',  # fit result of ws1
                        'name=Gaussian,Height=9,PeakCentre=40000,Sigma=60']  # fit result of ws2
        self.view.read_fitprop_from_browser.return_value = {'properties': {'Function': fun_str_list[0]}}  # initial
        mock_fit_output = [mock.MagicMock(), mock.MagicMock()]
        mock_fit_output[0].OutputStatus = "success"
        mock_fit_output[0].Function.fun = fun_str_list[1]
        mock_fit_output[1].OutputStatus = "fail"
        mock_fit_output[1].Function.fun = fun_str_list[2]
        mock_fit.side_effect = mock_fit_output
        mock_notifier = mock.MagicMock()
        self.presenter.fit_all_done_notifier = mock_notifier

        self.presenter.do_fit_all(ws_list, do_sequential=False)

        self.assertEqual(mock_fit.call_count, len(ws_list))
        fitprop_list = []
        for iws, ws in enumerate(ws_list):
            # check calls to fit
            _, _, kwargs = mock_fit.mock_calls[iws]
            if do_sequential:
                self.assertEquals(kwargs, {'Function': fun_str_list[iws], 'InputWorkspace': ws, 'Output': ws})
            else:
                # use same initial params for all workspaces
                self.assertEquals(kwargs, {'Function': fun_str_list[0], 'InputWorkspace': ws, 'Output': ws})
            # check update browser with fit results
            _, args, _ = self.view.update_browser.mock_calls[iws]
            self.assertEquals(args, (mock_fit_output[iws].OutputStatus, fun_str_list[iws + 1], ws))
            # collect all fitprop dicts together to test notifier
            fitprop_list.append({'properties': {'Function': fun_str_list[iws + 1], 'InputWorkspace': ws, 'Output': ws},
                                 'status': mock_fit_output[iws].OutputStatus})
        mock_notifier.notify_subscribers.assert_called_once_with(fitprop_list)


if __name__ == '__main__':
    unittest.main()
