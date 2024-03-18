# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting import plot_model, plot_view, plot_presenter
from mantidqt.utils.asynchronous import BlockingAsyncTaskWithCallback
from mantid.simpleapi import CreateEmptyTableWorkspace, GroupWorkspaces

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_presenter"


class FittingPlotPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = mock.create_autospec(plot_model.FittingPlotModel)
        self.view = mock.create_autospec(plot_view.FittingPlotView)
        self.presenter = plot_presenter.FittingPlotPresenter(None, self.model, self.view)
        self.call_in_progress = mock.call(
            status="fitting...", minimum=0, maximum=0, value=0, style_sheet=plot_presenter.IN_PROGRESS_STYLE_SHEET
        )
        self.call_fail = mock.call(status="fail", minimum=0, maximum=100, value=0, style_sheet=plot_presenter.FAILED_STYLE_SHEET)
        self.call_success = mock.call(status="success", minimum=0, maximum=100, value=100, style_sheet=plot_presenter.SUCCESS_STYLE_SHEET)
        self.call_empty = mock.call(status="", minimum=0, maximum=100, value=0, style_sheet=plot_presenter.EMPTY_STYLE_SHEET)

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

    @mock.patch(dir_path + ".AsyncTask", wraps=BlockingAsyncTaskWithCallback)
    @mock.patch(dir_path + ".Fit")
    def test_do_sequential_fit(self, mock_fit, mock_async):
        self.fit_all_helper(mock_fit, do_sequential=True)

    @mock.patch(dir_path + ".AsyncTask", wraps=BlockingAsyncTaskWithCallback)
    @mock.patch(dir_path + ".Fit")
    def test_do_serial_fit(self, mock_fit, mock_async):
        self.fit_all_helper(mock_fit, do_sequential=False)

    @mock.patch(dir_path + ".AsyncTask", wraps=BlockingAsyncTaskWithCallback)
    @mock.patch(dir_path + ".Fit")
    def test_do_sequential_fit_does_not_use_failed_fit_as_input(self, mock_fit, mock_async):
        ws_list = ["ws1", "ws2"]
        fun_str_list = [
            "name=Gaussian,Height=11,PeakCentre=30000,Sigma=40",  # initial
            "name=Gaussian,Height=10,PeakCentre=35000,Sigma=50",
        ]  # fit result of ws1 (and ws2)
        self.view.read_fitprop_from_browser.return_value = {"properties": {"Function": fun_str_list[0]}}  # initial
        mock_fit_output = mock.MagicMock()
        mock_fit_output.OutputStatus = "fail"
        mock_fit_output.Function.fun = fun_str_list[1]
        mock_fit.return_value = mock_fit_output
        previous_fit_properties = self.view.read_fitprop_from_browser()
        self.presenter.do_fit_all(previous_fit_properties, ws_list, do_sequential=True)

        self.assertEqual(mock_fit.call_count, len(ws_list))
        for iws, ws in enumerate(ws_list):
            # check calls to fit use initial guess for both workspaces as first fit was unsuccessful
            _, _, kwargs = mock_fit.mock_calls[iws]
            self.assertEqual(kwargs, {"Function": fun_str_list[0], "InputWorkspace": ws, "Output": ws})

    def fit_all_helper(self, mock_fit, do_sequential):
        ws_list = ["ws1", "ws2"]
        fun_str_list = [
            "name=Gaussian,Height=11,PeakCentre=30000,Sigma=40",  # initial
            "name=Gaussian,Height=10,PeakCentre=35000,Sigma=50",  # fit result of ws1
            "name=Gaussian,Height=9,PeakCentre=40000,Sigma=60",
        ]  # fit result of ws2
        self.view.read_fitprop_from_browser.return_value = {"properties": {"Function": fun_str_list[0]}}  # initial
        mock_fit_output = [mock.MagicMock(), mock.MagicMock()]
        mock_fit_output[0].OutputStatus = "success"
        mock_fit_output[0].Function.fun = fun_str_list[1]
        mock_fit_output[1].OutputStatus = "fail"
        mock_fit_output[1].Function.fun = fun_str_list[2]
        mock_fit.side_effect = mock_fit_output
        mock_notifier = mock.MagicMock()
        self.presenter.fit_all_done_notifier = mock_notifier

        self.presenter.do_fit_all_async(ws_list, do_sequential=do_sequential)

        self.assertEqual(mock_fit.call_count, len(ws_list))
        fitprop_list = []
        for iws, ws in enumerate(ws_list):
            # check calls to fit
            _, _, kwargs = mock_fit.mock_calls[iws]
            if do_sequential:
                self.assertEqual(kwargs, {"Function": fun_str_list[iws], "InputWorkspace": ws, "Output": ws})
            else:
                # use same initial params for all workspaces
                self.assertEqual(kwargs, {"Function": fun_str_list[0], "InputWorkspace": ws, "Output": ws})
            # check update browser with fit results
            self.view.update_browser.assert_not_called()
            # collect all fitprop dicts together to test notifier
            fitprop_list.append(
                {
                    "properties": {"Function": fun_str_list[iws + 1], "InputWorkspace": ws, "Output": ws},
                    "status": mock_fit_output[iws].OutputStatus,
                }
            )
        self.assertEqual(self.presenter.fitprop_list, fitprop_list)

    def test_add_workspace_sets_progress_to_zero(self):
        self.presenter.add_workspace_to_plot("workspace")
        self.view.set_progress_bar.assert_has_calls([self.call_empty])

    def test_remove_workspace_sets_progress_to_zero(self):
        self.presenter.remove_workspace_from_plot("workspace")
        self.view.set_progress_bar.assert_has_calls([self.call_empty])

    def test_clear_plot_sets_progress_to_zero(self):
        self.presenter.clear_plot()
        self.view.set_progress_bar.assert_has_calls([self.call_empty])

    def test_final_state_fail_output_list(self):
        self.presenter.set_final_state_progress_bar([{"status": "fail"}])
        self.view.set_progress_bar.assert_has_calls([self.call_fail])

    def test_final_state_fail_status(self):
        self.presenter.set_final_state_progress_bar(output_list=None, status="fail")
        self.view.set_progress_bar.assert_has_calls([self.call_fail])

    def test_final_state_success_output_list(self):
        self.presenter.set_final_state_progress_bar([{"status": "success"}])
        self.view.set_progress_bar.assert_has_calls([self.call_success])

    def test_final_state_success_status(self):
        self.presenter.set_final_state_progress_bar(output_list=None, status="success")
        self.view.set_progress_bar.assert_has_calls([self.call_success])

    def test_setup_toolbar(self):
        # Get's called during setup, so let's begin with a blank slate.
        self.view.reset_mock()

        self.presenter.setup_toolbar()

        self.view.set_slot_for_display_all.assert_called_once()
        self.view.set_slot_for_fit_toggled.assert_called_once_with(self.presenter.fit_toggle)
        self.view.set_slot_for_serial_fit.assert_called_once_with(self.presenter.do_serial_fit)
        self.view.set_slot_for_seq_fit.assert_called_once_with(self.presenter.do_seq_fit)
        self.view.set_slot_for_legend_toggled.assert_called_once()
        self.view.set_slot_for_find_peaks_convolve.assert_called_once_with(self.presenter.run_find_peaks_convolve)

    def test_hide_toolbar_fit_toggle(self):
        self.view.is_fit_browser_visible.return_value = True
        self.presenter.fit_toggle()
        self.view.hide_fit_browser.assert_called_once()
        self.view.hide_fit_progress_bar.assert_called_once()
        self.view.set_progress_bar.assert_called_once()
        self.view.set_find_peaks_convolve_button_status.assert_called_once_with(False)

    def test_show_fail_toolbar_fit_toggle(self):
        self.view.is_fit_browser_visible.return_value = False
        self.presenter.fit_toggle()
        self.view.show_fit_browser.assert_called_once()
        self.view.show_fit_progress_bar.assert_not_called()
        self.view.set_find_peaks_convolve_button_status.assert_not_called()
        self.view.set_progress_bar.assert_called_once()

    def test_show_success_toolbar_fit_toggle(self):
        self.view.is_fit_browser_visible.return_value = False
        self.view.show_fit_browser.side_effect = self.set_browser_visible
        self.presenter.fit_toggle()
        self.view.show_fit_browser.assert_called_once()
        self.view.show_fit_progress_bar.assert_called_once()
        self.view.set_find_peaks_convolve_button_status.assert_called_once_with(True)
        self.view.set_progress_bar.assert_called_once()

    def set_browser_visible(self):
        self.view.is_fit_browser_visible.return_value = True

    @mock.patch(dir_path + ".logger.warning")
    def test_cancel_worker_valid(self, mock_logger):
        self.presenter.fitprop_list = True
        self.presenter.worker = mock.MagicMock(return_value=True)
        self.presenter.worker.abort = mock.MagicMock()
        self.presenter._finished = mock.MagicMock()

        self.presenter.on_cancel_clicked()

        self.presenter.worker.abort.assert_called_once()
        mock_logger.assert_called_once()
        self.assertEqual(None, self.presenter.fitprop_list)
        self.presenter._finished.assert_called_once()

    @mock.patch(dir_path + ".logger.warning")
    def test_cancel_worker_invalid(self, mock_logger):
        self.presenter.fitprop_list = True
        self.presenter.worker = None
        self.presenter.abort_worker = mock.MagicMock()
        self.presenter._finished = mock.MagicMock()

        self.presenter.on_cancel_clicked()

        self.presenter.abort_worker.assert_not_called()
        self.assertEqual(0, mock_logger.call_count)
        self.assertEqual(True, self.presenter.fitprop_list)
        self.presenter._finished.assert_not_called()

    def test_abort_worker(self):
        # this behaviour is assumed in the above cancel worker tests
        self.presenter.worker = mock.MagicMock()
        self.presenter.worker.abort = mock.MagicMock()
        self.presenter.abort_worker()
        self.presenter.worker.abort.assert_called_once()

    @mock.patch(dir_path + ".logger.error")
    def test_on_worker_error_valid(self, mock_logger):
        error_message = "There was an error in the fitting routine."
        self.presenter._on_worker_error(error_message)
        mock_logger.assert_called_once_with("There was an error in the fitting routine.")

    @mock.patch(dir_path + ".logger.error")
    def test_on_worker_error_KeyboardInterrupt(self, mock_logger):
        error_message = "KeyboardInterrupt: There was an error in the fitting routine."
        self.presenter._on_worker_error(error_message)
        mock_logger.assert_not_called()

    @mock.patch(dir_path + ".GenericObservable.notify_subscribers")
    def test_handle_convolve_peaks_added_when_waiting(self, mock_notify):
        self.presenter.is_waiting_convolve_peaks = True
        self.presenter.handle_convolve_peaks_added()
        mock_notify.assert_called_once_with(True)
        self.assertEqual(self.presenter.is_waiting_convolve_peaks, False)

    @mock.patch(dir_path + ".GenericObservable.notify_subscribers")
    def test_handle_convolve_peaks_added_when_not_waiting(self, mock_notify):
        self.presenter.is_waiting_convolve_peaks = False
        self.presenter.handle_convolve_peaks_added()
        mock_notify.assert_not_called()
        self.assertEqual(self.presenter.is_waiting_convolve_peaks, False)

    @mock.patch(dir_path + ".logger.error")
    @mock.patch(dir_path + ".ADS.doesExist")
    def test_run_find_peaks_convolve_non_existing_ws(self, mock_ads_exist, mock_error):
        self.view = mock.MagicMock()
        self.view.fit_browser.workspaceName.return_value = "non_existing_ws"
        self.presenter.view = self.view
        mock_ads_exist.return_value = False
        self.presenter.find_peaks_convolve_started_notifier = mock.MagicMock()
        self.presenter.find_peaks_convolve_done_notifier = mock.MagicMock()
        self.presenter.run_find_peaks_convolve()
        self.presenter.find_peaks_convolve_started_notifier.notify_subscribers.assert_called_once()
        self.presenter.find_peaks_convolve_done_notifier.notify_subscribers.assert_called_once_with(False)
        mock_error.assert_called()

    def _get_sample_findpeaksconvolve_group_ws(self):
        peak_centers = CreateEmptyTableWorkspace(OutputWorkspace="PeakCentre")
        peak_centers.addColumn("int", "SpecIndex")
        peak_centers.addColumn("double", "PeakCentre_0")
        peak_centers.addRow([1, 0.09])

        peak_y = CreateEmptyTableWorkspace(OutputWorkspace="PeakYPosition")
        peak_y.addColumn("int", "SpecIndex")
        peak_y.addColumn("double", "PeakYPosition_0")
        peak_y.addRow([1, 798])

        i_over_sigma = CreateEmptyTableWorkspace(OutputWorkspace="PeakIOverSigma")
        i_over_sigma.addColumn("int", "SpecIndex")
        i_over_sigma.addColumn("double", "PeakIOverSigma_0")
        i_over_sigma.addRow([1, 12.5])
        return GroupWorkspaces([peak_centers, peak_y, i_over_sigma], OutputWorkspace="test")

    @mock.patch(dir_path + ".FunctionWrapper")
    @mock.patch(dir_path + ".FunctionFactory")
    @mock.patch(dir_path + ".FindPeaksConvolve")
    @mock.patch(dir_path + ".logger.error")
    @mock.patch(dir_path + ".ADS")
    def test_run_find_peaks_convolve_existing_ws(self, mock_ads, mock_error, mock_find_peaks_convolve, mock_func_fac, mock_func_wrapper):
        self.view = mock.MagicMock()
        self.view.fit_browser.workspaceName.return_value = "ws_1"
        self.presenter.view = self.view
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.return_value = mock.MagicMock()
        mock_find_peaks_convolve.return_value = self._get_sample_findpeaksconvolve_group_ws()
        self.presenter.find_peaks_convolve_started_notifier = mock.MagicMock()
        self.presenter.find_peaks_convolve_done_notifier = mock.MagicMock()
        self.view.fit_browser.defaultPeakType.return_value = "BackToBackExponential"
        mock_peak_func = mock.MagicMock()
        mock_func_fac.Instance().createPeakFunction.return_value = mock_peak_func

        self.presenter.run_find_peaks_convolve()

        self.presenter.find_peaks_convolve_started_notifier.notify_subscribers.assert_called_once()
        mock_peak_func.setCentre.assert_called_once()
        mock_peak_func.setHeight.assert_called_once()
        mock_peak_func.setMatrixWorkspace.assert_called_once()
        self.view.fit_browser.loadFunction.assert_called_once_with(str(mock_func_wrapper()))
        self.presenter.handle_convolve_peaks_added()
        self.presenter.find_peaks_convolve_done_notifier.notify_subscribers.assert_called_once_with(True)
        mock_error.assert_not_called()
        self.assertEqual(self.presenter.is_waiting_convolve_peaks, False)


if __name__ == "__main__":
    unittest.main()
