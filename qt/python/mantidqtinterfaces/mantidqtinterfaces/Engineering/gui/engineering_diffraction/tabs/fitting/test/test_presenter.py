# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting import view, presenter
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting import plot_presenter
from mantidqt.utils.asynchronous import BlockingAsyncTaskWithCallback

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.presenter"
plot_dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_presenter"

ws_list = ["ws1", "ws2"]
fun_str_list = [
    "name=Gaussian,Height=11,PeakCentre=30000,Sigma=40",  # initial
    "name=Gaussian,Height=10,PeakCentre=35000,Sigma=50",  # fit result of ws1
    "name=Gaussian,Height=9,PeakCentre=40000,Sigma=60",
]  # fit result of ws2
fitprop_list = []
for iws in range(len(ws_list)):
    fitprop_list.append(
        {"properties": {"Function": fun_str_list[iws + 1], "InputWorkspace": ws_list[0], "Output": ws_list[iws]}, "status": "success"}
    )


class FittingPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.FittingView, instance=True)
        self.presenter = presenter.FittingPresenter(self.view)

        self.call_in_progress = mock.call(
            status="fitting...", minimum=0, maximum=0, value=0, style_sheet=plot_presenter.IN_PROGRESS_STYLE_SHEET
        )
        self.call_fail = mock.call(status="fail", minimum=0, maximum=100, value=0, style_sheet=plot_presenter.FAILED_STYLE_SHEET)
        self.call_success = mock.call(status="success", minimum=0, maximum=100, value=100, style_sheet=plot_presenter.SUCCESS_STYLE_SHEET)
        self.call_empty = mock.call(status="", minimum=0, maximum=100, value=0, style_sheet=plot_presenter.EMPTY_STYLE_SHEET)

    def fit_all_helper(self, mock_fit, do_sequential):
        self.presenter.plot_widget.view.read_fitprop_from_browser.return_value = {"properties": {"Function": fun_str_list[0]}}  # initial
        mock_fit_output = [mock.MagicMock(), mock.MagicMock()]
        mock_fit_output[0].OutputStatus = "success"
        mock_fit_output[0].Function.fun = fun_str_list[1]
        mock_fit_output[1].OutputStatus = "fail"
        mock_fit_output[1].Function.fun = fun_str_list[2]
        mock_fit.side_effect = mock_fit_output
        mock_notifier = mock.MagicMock()
        self.presenter.plot_widget.fit_all_done_notifier = mock_notifier

        self.presenter.plot_widget.do_fit_all_async(ws_list, do_sequential=do_sequential)

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
            self.presenter.plot_widget.view.update_browser.assert_not_called()
            # collect all fitprop dicts together to test notifier
            fitprop_list.append(
                {
                    "properties": {"Function": fun_str_list[iws + 1], "InputWorkspace": ws, "Output": ws},
                    "status": mock_fit_output[iws].OutputStatus,
                }
            )
        self.assertEqual(self.presenter.plot_widget.fitprop_list, fitprop_list)

    @mock.patch(plot_dir_path + ".FittingPlotPresenter._finished")
    @mock.patch(plot_dir_path + ".AsyncTask", wraps=BlockingAsyncTaskWithCallback)
    @mock.patch(plot_dir_path + ".Fit")
    def test_do_all_fit_calls_finished(self, mock_fit, mock_async, mock_fit_all_done):
        self.fit_all_helper(mock_fit, do_sequential=False)
        self.assertEqual(1, mock_fit_all_done.call_count)

    @mock.patch(plot_dir_path + ".FittingPlotPresenter._finished")
    @mock.patch(plot_dir_path + ".AsyncTask", wraps=BlockingAsyncTaskWithCallback)
    @mock.patch(plot_dir_path + ".Fit")
    def test_do_all_fit_sets_progress_bar_to_in_progress_then_final(self, mock_fit, mock_async, mock_fit_all_done):
        self.presenter.plot_widget.view.read_fitprop_from_browser.return_value = {"properties": {"Function": fun_str_list[0]}}  # initial
        mock_fit_output = mock.MagicMock()
        mock_fit_output.OutputStatus = "fail"
        mock_fit_output.Function.fun = fun_str_list[1]
        mock_fit.return_value = mock_fit_output
        fitprop_list = []
        for iws in range(len(ws_list)):
            fitprop_list.append(
                {
                    "properties": {"Function": fun_str_list[iws + 1], "InputWorkspace": ws_list[0], "Output": ws_list[iws]},
                    "status": "success",
                }
            )
        self.presenter.data_widget.presenter.model = mock.MagicMock()
        self.presenter.data_widget.presenter.model.get_active_ws_sorted_by_primary_log.return_value = ws_list
        self.presenter.plot_widget.fit_all_started_notifier.notify_subscribers(True)
        self.presenter.plot_widget.fitprop_list = fitprop_list
        self.assertEqual(1, mock_fit_all_done.call_count)

    def test_do_all_fit_finished_notifies_presenter(self):
        self.presenter.plot_widget.fit_all_done_notifier.notify_subscribers = mock.MagicMock()
        self.presenter.plot_widget.fitprop_list = fitprop_list
        self.presenter.plot_widget._finished()
        self.assertEqual(1, self.presenter.plot_widget.fit_all_done_notifier.notify_subscribers.call_count)

    def test_fit_all_done(self):
        log_ws_name = "bleh"
        self.presenter.plot_widget.update_browser = mock.MagicMock()
        self.presenter.plot_widget.set_final_state_progress_bar = mock.MagicMock()
        self.presenter.enable_view = mock.MagicMock()
        self.presenter.data_widget.presenter.model.get_log_workspace_group_name = mock.MagicMock(return_value=log_ws_name)
        self.presenter.plot_widget.fit_completed = mock.MagicMock()

        self.presenter.plot_widget.fitprop_list = fitprop_list
        self.presenter.plot_widget._finished()

        self.presenter.plot_widget.update_browser.assert_called_once()
        self.presenter.plot_widget.set_final_state_progress_bar.assert_called_once_with(output_list=fitprop_list)
        self.presenter.enable_view.assert_called_once_with(fit_all=True)
        self.presenter.plot_widget.fit_completed.assert_called_once_with(fitprop_list, [], [], log_ws_name)

    def test_fit_all_done_calls_set_final_state_indirectly(self):
        self.presenter.data_widget.presenter.model.get_all_log_workspaces_names = mock.MagicMock(return_value=[])
        self.presenter.plot_widget.update_browser = mock.MagicMock()
        self.presenter.plot_widget.update_progress_bar = mock.MagicMock()
        self.presenter.plot_widget.set_final_state_progress_bar = mock.MagicMock()
        self.presenter.enable_view = mock.MagicMock()
        self.presenter.plot_widget.fit_completed = mock.MagicMock()

        self.presenter.plot_widget.fitprop_list = fitprop_list
        self.presenter.plot_widget._finished()

        self.presenter.plot_widget.update_progress_bar.assert_called_once()
        self.presenter.plot_widget.set_final_state_progress_bar.assert_not_called()
        # blocked by mocking out update_progress_bar which uses fit_props saved to the plot_presenter class

    def test_fit_all_started_notified_sequential(self):
        self.presenter.plot_widget.fit_all_started_notifier.notify_subscribers = mock.MagicMock()
        self.presenter.plot_widget.do_seq_fit()
        self.presenter.plot_widget.fit_all_started_notifier.notify_subscribers.assert_called_once_with((True))

    def test_fit_all_started_notified_serial(self):
        self.presenter.plot_widget.fit_all_started_notifier.notify_subscribers = mock.MagicMock()
        self.presenter.plot_widget.do_serial_fit()
        self.presenter.plot_widget.fit_all_started_notifier.notify_subscribers.assert_called_once_with((False))

    def test_fit_all_started_sequential(self):
        self.presenter.data_widget.model.get_active_ws_sorted_by_primary_log = mock.MagicMock(return_value=ws_list)
        self.presenter.plot_widget.set_progress_bar_to_in_progress = mock.MagicMock()
        self.presenter.disable_view = mock.MagicMock()
        self.presenter.plot_widget.do_fit_all_async = mock.MagicMock()

        self.presenter.plot_widget.fitprop_list = fitprop_list
        self.presenter.plot_widget.do_seq_fit()

        self.presenter.plot_widget.set_progress_bar_to_in_progress(output_list=fitprop_list)
        self.presenter.disable_view.assert_called_once_with(fit_all=True)
        self.presenter.plot_widget.do_fit_all_async.assert_called_once_with(ws_list, True)

    def test_fit_all_started_serial(self):
        self.presenter.data_widget.model.get_active_ws_name_list = mock.MagicMock(return_value=ws_list)
        self.presenter.plot_widget.set_progress_bar_to_in_progress = mock.MagicMock()
        self.presenter.disable_view = mock.MagicMock()
        self.presenter.plot_widget.do_fit_all_async = mock.MagicMock()

        self.presenter.plot_widget.fitprop_list = fitprop_list
        self.presenter.plot_widget.do_serial_fit()

        self.presenter.plot_widget.set_progress_bar_to_in_progress(output_list=fitprop_list)
        self.presenter.disable_view.assert_called_once_with(fit_all=True)
        self.presenter.plot_widget.do_fit_all_async.assert_called_once_with(ws_list, False)

    def test_fit_started_call_order(self):
        mock_manager = mock.Mock()
        self.presenter.disable_view = mock.MagicMock()
        self.presenter.plot_widget.set_progress_bar_to_in_progress = mock.MagicMock()
        mock_manager.disable, mock_manager.progress_bar = (
            self.presenter.disable_view,
            self.presenter.plot_widget.set_progress_bar_to_in_progress,
        )

        self.presenter.fit_started()
        self.presenter.disable_view.assert_called_once_with()
        mock_manager.assert_has_calls([mock.call.progress_bar(), mock.call.disable()])

    def test_fit_done_call_order(self):
        self.presenter.data_widget.presenter.model.get_all_log_workspaces_names = mock.MagicMock(return_value=[])
        mock_manager = mock.Mock()
        self.presenter.enable_view = mock.MagicMock()
        self.presenter.plot_widget.set_final_state_progress_bar = mock.MagicMock()
        self.presenter.plot_widget.fit_completed = mock.MagicMock()
        mock_manager.enable, mock_manager.progress_bar, mock_manager.fit_completed = (
            self.presenter.enable_view,
            self.presenter.plot_widget.set_final_state_progress_bar,
            self.presenter.plot_widget.fit_completed,
        )

        self.presenter.fit_done(fitprop_list)
        mock_manager.assert_has_calls(
            [mock.call.enable(), mock.call.progress_bar(fitprop_list), mock.call.fit_completed(fitprop_list, [], [], "")]
        )

    def test_fit_done_call_order_failed_fit(self):
        self.presenter.data_widget.presenter.model.get_all_log_workspaces_names = mock.MagicMock(return_value=[])
        mock_manager = mock.Mock()
        self.presenter.enable_view = mock.MagicMock()
        self.presenter.plot_widget.set_final_state_progress_bar = mock.MagicMock()
        self.presenter.plot_widget.fit_completed = mock.MagicMock()
        mock_manager.enable, mock_manager.progress_bar = (
            self.presenter.enable_view,
            self.presenter.plot_widget.set_final_state_progress_bar,
        )

        self.presenter.fit_done([])
        mock_manager.assert_has_calls([mock.call.enable(), mock.call.progress_bar(None, status="Failed, invalid fit.")])
        self.presenter.plot_widget.fit_completed.assert_not_called()

    def test_enable_view_not_fit_all(self):
        self.presenter.data_widget.view.setEnabled = mock.MagicMock()
        self.presenter.plot_widget.enable_view_components = mock.MagicMock()
        self.presenter.plot_widget.view.show_cancel_button = mock.MagicMock()

        self.presenter.enable_view()

        self.presenter.data_widget.view.setEnabled.assert_called_once_with(True)
        self.presenter.plot_widget.enable_view_components.assert_called_once_with(True)
        self.presenter.plot_widget.view.show_cancel_button.assert_not_called()

    def test_enable_view_fit_all(self):
        self.presenter.data_widget.view.setEnabled = mock.MagicMock()
        self.presenter.plot_widget.enable_view_components = mock.MagicMock()
        self.presenter.plot_widget.view.show_cancel_button = mock.MagicMock()

        self.presenter.enable_view(fit_all=True)

        self.presenter.data_widget.view.setEnabled.assert_called_once_with(True)
        self.presenter.plot_widget.enable_view_components.assert_called_once_with(True)
        self.presenter.plot_widget.view.show_cancel_button.assert_called_once_with(False)

    def test_disable_view_not_fit_all(self):
        self.presenter.data_widget.view.setEnabled = mock.MagicMock()
        self.presenter.plot_widget.enable_view_components = mock.MagicMock()
        self.presenter.plot_widget.view.show_cancel_button = mock.MagicMock()

        self.presenter.disable_view()

        self.presenter.data_widget.view.setEnabled.assert_called_once_with(False)
        self.presenter.plot_widget.enable_view_components.assert_called_once_with(False)
        self.presenter.plot_widget.view.show_cancel_button.assert_not_called()

    def test_disable_view_fit_all(self):
        self.presenter.data_widget.view.setEnabled = mock.MagicMock()
        self.presenter.plot_widget.enable_view_components = mock.MagicMock()
        self.presenter.plot_widget.view.show_cancel_button = mock.MagicMock()

        self.presenter.disable_view(fit_all=True)

        self.presenter.data_widget.view.setEnabled.assert_called_once_with(False)
        self.presenter.plot_widget.enable_view_components.assert_called_once_with(False)
        self.presenter.plot_widget.view.show_cancel_button.assert_called_once_with(True)

    def test_on_cancel_clicked(self):
        mock_manager = mock.Mock()
        self.presenter.plot_widget.on_cancel_clicked = mock.MagicMock()
        self.presenter.enable_view = mock.MagicMock()
        self.presenter.plot_widget.set_progress_bar_zero = mock.MagicMock()
        mock_manager.cancel_clicked, mock_manager.enable, mock_manager.progress_bar = (
            self.presenter.plot_widget.on_cancel_clicked,
            self.presenter.enable_view,
            self.presenter.plot_widget.set_progress_bar_zero,
        )

        self.presenter.on_cancel_clicked()

        mock_manager.assert_has_calls([mock.call.cancel_clicked(), mock.call.enable(), mock.call.progress_bar])


if __name__ == "__main__":
    unittest.main()
