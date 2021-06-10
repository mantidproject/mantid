# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqt.widgets.superplot.SuperplotPresenter import SuperplotPresenter


class SuperplotPresenterTest(unittest.TestCase):

    def setUp(self):
        pyModule = "mantidqt.widgets.superplot.SuperplotPresenter"

        patch = mock.patch(pyModule + ".SuperplotView")
        self.mView = patch.start()
        self.mView = self.mView.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(pyModule + ".SuperplotModel")
        self.mModel = patch.start()
        self.mModel = self.mModel.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(pyModule + ".mtd")
        self.mMtd = patch.start()
        self.addCleanup(patch.stop)

        self.mCanvas = mock.Mock()
        self.mFigure = self.mCanvas.figure
        self.mAxes = self.mCanvas.figure.gca.return_value
        self.mAxes.get_lines.return_value = list()
        self.mAxes.creation_args = [{}]
        a1 = mock.Mock()
        ws = mock.Mock()
        sp = mock.Mock()
        self.mAxes.get_artists_workspace_and_workspace_index.return_value = \
            ws, sp
        self.mAxes.get_tracked_artists.return_value = [a1]
        self.mView.get_selection.return_value = {}
        self.presenter = SuperplotPresenter(self.mCanvas)
        self.mView.reset_mock()
        self.mModel.reset_mock()

    def test_init(self):
        self.presenter.__init__(self.mCanvas)
        self.mModel.sig_workspace_deleted.connect.assert_called_once()
        self.mModel.sig_workspace_renamed.connect.assert_called_once()
        self.mModel.sig_workspace_replaced.connect.assert_called_once()
        self.mView.set_available_modes.assert_called()
        self.mModel.set_spectrum_mode.assert_called_once()
        self.mAxes.creation_args = [{"axis": 0}]
        self.presenter = SuperplotPresenter(self.mCanvas)
        self.mModel.set_bin_mode.assert_called_once()

    def test_getSideView(self):
        self.presenter.get_side_view()
        self.mView.get_side_widget.assert_called_once()

    def test_getBottomView(self):
        self.presenter.get_bottom_view()
        self.mView.get_bottom_widget.assert_called_once()

    def test_close(self):
        self.presenter.close()
        self.mView.close.assert_called_once()

    def test_onVisibilityChanged(self):
        self.mCanvas.reset_mock()
        self.mFigure.resetMock()
        self.presenter.on_visibility_changed(True)
        self.presenter.on_visibility_changed(False)
        self.mFigure.tight_layout.assert_called_once()
        self.mCanvas.draw_idle.assert_called_once()

    def test_onAddButtonClicked(self):
        self.presenter._update_list = mock.Mock()
        self.presenter.on_add_button_clicked()
        self.mModel.add_workspace.assert_called_once()
        self.presenter._update_list.assert_called_once()
        self.mView.set_selection.assert_called_once()

    def test_onDelButtonClicked(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.mView.get_selection.return_value = {"ws1": 1, "ws2": 2}
        self.presenter.on_del_button_clicked()
        self.mView.get_selection.assert_called_once()
        calls = [mock.call("ws1"), mock.call("ws2")]
        self.mModel.del_workspace.assert_has_calls(calls)
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_list.reset_mock()
        self.presenter._update_plot.reset_mock()
        self.presenter._update_spectrum_slider.reset_mock()
        self.mModel.reset_mock()
        self.presenter.on_del_button_clicked("ws3")
        self.mModel.del_workspace.assert_called_once_with("ws3")
        self.presenter._update_plot.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()

    def test_updateSpectrumSlider(self):
        self.presenter._update_spectrum_slider()
        self.mView.set_spectrum_slider_position.assert_called_once_with(0)
        self.mView.set_spectrum_slider_max.assert_called_once_with(0)
        self.mView.set_spectrum_spin_box_value.assert_called_once_with(0)
        self.mView.set_spectrum_spin_box_max.assert_called_once_with(0)
        self.mView.set_spectrum_disabled.assert_called_once_with(True)
        self.mView.reset_mock()
        ws = mock.Mock()
        ws.getNumberHistograms.return_value = 50
        ws.blocksize.return_value = 60
        self.mMtd.__getitem__.return_value = ws
        self.mView.get_mode.return_value = self.presenter.SPECTRUM_MODE_TEXT
        self.mView.get_selection.return_value = {"ws1": [10]}
        self.presenter._update_spectrum_slider()
        ws.getNumberHistograms.assert_called_once()
        self.mView.set_spectrum_disabled.assert_called_once_with(False)
        self.mView.set_spectrum_slider_max.assert_called_once_with(49)
        self.mView.set_spectrum_slider_position.assert_called_once_with(10)
        self.mView.set_spectrum_spin_box_max.assert_called_once_with(49)
        self.mView.set_spectrum_spin_box_value.assert_called_once_with(10)

    def test_updateHoldButton(self):
        self.mModel.get_plotted_data.return_value = [("ws1", 1), ("ws2", 2)]
        self.mView.get_selection.return_value = {"ws1": []}
        self.mView.get_spectrum_slider_position.return_value = 10
        self.presenter._update_hold_button()
        self.mView.check_hold_button.assert_called_once_with(False)
        self.mView.reset_mock()
        self.mView.get_selection.return_value = {"ws2": []}
        self.mView.get_spectrum_slider_position.return_value = 2
        self.presenter._update_hold_button()
        self.mView.check_hold_button.assert_called_once_with(True)

    def test_updateList(self):
        self.mModel.get_workspaces.return_value = ["ws1", "ws2", "ws5"]
        self.mModel.get_plotted_data.return_value = [("ws5", 5), ("ws2", 1)]
        self.presenter._update_list()
        self.mView.set_workspaces_list.assert_called_once_with(["ws1", "ws2",
                                                              "ws5"])
        calls = [mock.call("ws1", []), mock.call("ws2", [1]),
                 mock.call("ws5", [5])]
        self.mView.set_spectra_list.assert_has_calls(calls)

    def test_updatePlot(self):
        self.mAxes.reset_mock()
        self.mModel.get_plotted_data.return_value = [("ws5", 5), ("ws2", 1)]
        self.mModel.is_spectrum_mode.return_value = True
        self.mModel.is_bin_mode.return_value = False
        self.mView.get_mode.return_value = self.presenter.SPECTRUM_MODE_TEXT
        self.mView.get_selection.return_value = {"ws1": [10]}
        self.mView.get_spectrum_slider_position.return_value = 10
        self.mAxes.plot.return_value = [mock.Mock()]
        a1 = mock.Mock()
        a1.get_label.return_value = "label"
        a1.get_color.return_value = "color"
        a2 = mock.Mock()
        a2.get_label.return_value = "label"
        a2.get_color.return_value = "color"
        a3 = mock.Mock()
        a3.get_label.return_value = "label"
        a3.get_color.return_value = "color"
        self.mAxes.get_tracked_artists.return_value = [a1, a2, a3]
        ws5 = mock.Mock()
        ws5.name.return_value = "ws5"
        ws2 = mock.Mock()
        ws2.name.return_value = "ws2"
        ws1 = mock.Mock()
        ws1.name.return_value = "ws1"
        self.mAxes.get_artists_workspace_and_workspace_index.side_effect = \
                [(ws5, 5), (ws2, 1), (ws1, 1)]
        line = mock.Mock()
        line.get_label.return_value = "label"
        line.get_color.return_value = "color"
        self.mAxes.plot.return_value = [line]
        self.presenter._update_plot()
        self.mAxes.remove_artists_if.assert_called_once()
        calls = [mock.call("ws5", 5, "label", "color"),
                 mock.call("ws2", 1, "label", "color"),
                 mock.call("ws1", 10, "label", "color")]
        self.mView.modify_spectrum_label.assert_has_calls(calls)

    def test_onWorkspaceSelectionChanged(self):
        self.presenter._update_plot = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.mView.get_selection.return_value = {}
        self.presenter.on_workspace_selection_changed()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_spectrum_slider.reset_mock()
        self.mView.get_selection.return_value = {"ws1": [1], "ws2": [1]}
        self.presenter.on_workspace_selection_changed()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_spectrum_slider.reset_mock()
        self.mView.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_workspace_selection_changed()
        self.presenter._update_spectrum_slider.assert_called_once()

    def test_onSpectrumSliderMoved(self):
        self.presenter._update_hold_button = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter.on_spectrum_slider_moved(1)
        self.mView.set_spectrum_spin_box_value.assert_called_once_with(1)
        self.presenter._update_hold_button.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_onSpectrumSpinBoxChanged(self):
        self.presenter._update_hold_button = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter.on_spectrum_spin_box_changed(1)
        self.mView.set_spectrum_slider_position.assert_called_once()
        self.presenter._update_hold_button.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_onDelSpectrumButtonClicked(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.mView.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_del_spectrum_button_clicked("ws3", 10)
        self.presenter._update_list.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.mView.set_selection.assert_called_once_with({"ws1": [1],
                                                         "ws2": [2]})
        self.mView.reset_mock()
        self.presenter.on_del_spectrum_button_clicked("ws1", 1)
        self.mView.set_selection.assert_called_once_with({"ws2": [2]})

    def test_onHold(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.mView.is_spectrum_disabled.return_value = False
        self.mView.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.mView.get_spectrum_slider_position.return_value = 10
        self.mView.get_mode.return_value = self.presenter.SPECTRUM_MODE_TEXT
        self.presenter._on_hold()
        calls = [mock.call("ws1", 10), mock.call("ws2", 10)]
        self.mModel.add_data.assert_has_calls(calls)
        self.mModel.set_spectrum_mode.assert_called_once()
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.mView.set_selection.assert_called_once_with({"ws1": [1],
                                                         "ws2": [2]})
        self.mView.get_mode.return_value = self.presenter.BIN_MODE_TEXT
        self.presenter._on_hold()
        self.mModel.set_bin_mode.assert_called_once()

    def test_onUnHold(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.mView.is_spectrum_disabled.return_value = False
        self.mView.get_selection.return_value = {"ws1": [], "ws2": []}
        self.mView.get_spectrum_slider_position.return_value = 10
        self.presenter._on_un_hold()
        calls = [mock.call("ws1", 10), mock.call("ws2", 10)]
        self.mModel.remove_data.assert_has_calls(calls)
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()

    def test_onHoldButtonToggled(self):
        self.presenter._on_hold = mock.Mock()
        self.presenter._on_un_hold = mock.Mock()
        self.presenter.on_hold_button_toggled(True)
        self.presenter._on_hold.assert_called_once()
        self.presenter._on_un_hold.assert_not_called
        self.presenter.on_hold_button_toggled(False)
        self.presenter._on_un_hold.assert_called_once()

    def test_onModeChanged(self):
        self.presenter._update_spectrum_slider = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.mView.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_mode_changed("mode")
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_onWorkspaceDeleted(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.mView.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_workspace_deleted("ws1")
        self.mView.set_selection.assert_called_once_with({"ws2": [2]})
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_onWorkspaceRenamed(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.mView.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_workspace_renamed("ws1", "ws3")
        self.mView.set_selection.assert_called_once_with({"ws3": [1],
                                                         "ws2": [2]})
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_onWorkspaceReplaced(self):
        self.presenter._update_plot = mock.Mock()
        self.presenter.on_workspace_replaced("ws1")
        self.presenter._update_plot.assert_called_once()
