# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqt.widgets.superplot.presenter import SuperplotPresenter


class SuperplotPresenterTest(unittest.TestCase):

    def setUp(self):
        py_module = "mantidqt.widgets.superplot.presenter"

        patch = mock.patch(py_module + ".SuperplotView")
        self.m_view = patch.start()
        self.m_view = self.m_view.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(py_module + ".SuperplotModel")
        self.m_model = patch.start()
        self.m_model = self.m_model.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(py_module + ".mtd")
        self.m_mtd = patch.start()
        self.addCleanup(patch.stop)

        self.m_canvas = mock.Mock()
        self.m_figure = self.m_canvas.figure
        self.m_axes = self.m_canvas.figure.gca.return_value
        self.m_axes.get_lines.return_value = list()
        self.m_axes.creation_args = [{}]
        a1 = mock.Mock()
        ws = mock.Mock()
        sp = mock.Mock()
        self.m_axes.get_artists_workspace_and_workspace_index.return_value = \
            ws, sp
        self.m_axes.get_tracked_artists.return_value = [a1]
        self.m_view.get_selection.return_value = {}
        self.presenter = SuperplotPresenter(self.m_canvas)
        self.m_view.reset_mock()
        self.m_model.reset_mock()

    def test_init(self):
        self.presenter.__init__(self.m_canvas)
        self.m_model.sig_workspace_deleted.connect.assert_called_once()
        self.m_model.sig_workspace_renamed.connect.assert_called_once()
        self.m_model.sig_workspace_replaced.connect.assert_called_once()
        self.m_view.set_available_modes.assert_called()
        self.m_model.set_spectrum_mode.assert_called_once()
        self.m_axes.creation_args = [{"axis": 0}]
        self.presenter = SuperplotPresenter(self.m_canvas)
        self.m_model.set_bin_mode.assert_called_once()

    def test_get_side_view(self):
        self.presenter.get_side_view()
        self.m_view.get_side_widget.assert_called_once()

    def test_get_bottom_view(self):
        self.presenter.get_bottom_view()
        self.m_view.get_bottom_widget.assert_called_once()

    def test_close(self):
        self.presenter.close()
        self.m_view.close.assert_called_once()

    def test_on_visibility_changed(self):
        self.m_canvas.reset_mock()
        self.m_figure.resetMock()
        self.presenter.on_visibility_changed(True)
        self.presenter.on_visibility_changed(False)
        self.m_figure.tight_layout.assert_called_once()
        self.m_canvas.draw_idle.assert_called_once()

    def test_on_add_button_clicked(self):
        self.presenter._update_list = mock.Mock()
        self.presenter.on_add_button_clicked()
        self.m_model.add_workspace.assert_called_once()
        self.presenter._update_list.assert_called_once()
        self.m_view.set_selection.assert_called_once()

    def test_on_del_button_clicked(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.m_view.get_selection.return_value = {"ws1": 1, "ws2": 2}
        self.presenter.on_del_button_clicked()
        self.m_view.get_selection.assert_called_once()
        calls = [mock.call("ws1"), mock.call("ws2")]
        self.m_model.del_workspace.assert_has_calls(calls)
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_list.reset_mock()
        self.presenter._update_plot.reset_mock()
        self.presenter._update_spectrum_slider.reset_mock()
        self.m_model.reset_mock()
        self.presenter.on_del_button_clicked("ws3")
        self.m_model.del_workspace.assert_called_once_with("ws3")
        self.presenter._update_plot.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()

    def test_update_spectrum_slider(self):
        self.presenter._update_spectrum_slider()
        self.m_view.set_spectrum_slider_position.assert_called_once_with(0)
        self.m_view.set_spectrum_slider_max.assert_called_once_with(0)
        self.m_view.set_spectrum_spin_box_value.assert_called_once_with(0)
        self.m_view.set_spectrum_spin_box_max.assert_called_once_with(0)
        self.m_view.set_spectrum_selection_disabled.assert_called_once_with(True)
        self.m_view.reset_mock()
        ws = mock.Mock()
        ws.getNumberHistograms.return_value = 50
        ws.blocksize.return_value = 60
        self.m_mtd.__getitem__.return_value = ws
        self.m_view.get_mode.return_value = self.presenter.SPECTRUM_MODE_TEXT
        self.m_view.get_selection.return_value = {"ws1": [10]}
        self.presenter._update_spectrum_slider()
        ws.getNumberHistograms.assert_called_once()
        self.m_view.set_spectrum_selection_disabled.assert_called_once_with(False)
        self.m_view.set_spectrum_slider_max.assert_called_once_with(49)
        self.m_view.set_spectrum_slider_position.assert_called_once_with(10)
        self.m_view.set_spectrum_spin_box_max.assert_called_once_with(49)
        self.m_view.set_spectrum_spin_box_value.assert_called_once_with(10)

    def test_update_hold_button(self):
        self.m_model.get_plotted_data.return_value = [("ws1", 1), ("ws2", 2)]
        self.m_view.get_selection.return_value = {"ws1": []}
        self.m_view.get_spectrum_slider_position.return_value = 10
        self.presenter._update_hold_button()
        self.m_view.set_hold_button_text.assert_called_once_with(
                self.presenter.HOLD_BUTTON_TEXT_UNCHECKED)
        self.m_view.reset_mock()
        self.m_view.get_selection.return_value = {"ws2": []}
        self.m_view.get_spectrum_slider_position.return_value = 2
        self.presenter._update_hold_button()
        self.m_view.set_hold_button_text.assert_called_once_with(
                self.presenter.HOLD_BUTTON_TEXT_CHECKED)

    def test_update_list(self):
        self.m_model.get_workspaces.return_value = ["ws1", "ws2", "ws5"]
        self.m_model.get_plotted_data.return_value = [("ws5", 5), ("ws2", 1)]
        self.presenter._update_list()
        self.m_view.set_workspaces_list.assert_called_once_with(["ws1", "ws2",
                                                                 "ws5"])
        calls = [mock.call("ws1", []), mock.call("ws2", [1]),
                 mock.call("ws5", [5])]
        self.m_view.set_spectra_list.assert_has_calls(calls)

    def test_update_plot(self):
        self.m_axes.reset_mock()
        self.m_model.get_plotted_data.return_value = [("ws5", 5), ("ws2", 1)]
        self.m_model.is_spectrum_mode.return_value = True
        self.m_model.is_bin_mode.return_value = False
        self.m_view.get_mode.return_value = self.presenter.SPECTRUM_MODE_TEXT
        self.m_view.get_selection.return_value = {"ws1": [10]}
        self.m_view.get_spectrum_slider_position.return_value = 10
        self.m_axes.plot.return_value = [mock.Mock()]
        a1 = mock.Mock()
        a1.get_label.return_value = "label"
        a1.get_color.return_value = "color"
        a2 = mock.Mock()
        a2.get_label.return_value = "label"
        a2.get_color.return_value = "color"
        a3 = mock.Mock()
        a3.get_label.return_value = "label"
        a3.get_color.return_value = "color"
        self.m_axes.get_tracked_artists.return_value = [a1, a2, a3]
        ws5 = mock.Mock()
        ws5.name.return_value = "ws5"
        ws2 = mock.Mock()
        ws2.name.return_value = "ws2"
        ws1 = mock.Mock()
        ws1.name.return_value = "ws1"
        self.m_axes.get_artists_workspace_and_workspace_index.side_effect = \
            [(ws5, 5), (ws2, 1), (ws1, 1)]
        line = mock.Mock()
        line.get_label.return_value = "label"
        line.get_color.return_value = "color"
        self.m_axes.plot.return_value = [line]
        self.presenter._update_plot()
        self.m_axes.remove_artists_if.assert_called_once()
        calls = [mock.call("ws5", 5, "label", "color"),
                 mock.call("ws2", 1, "label", "color"),
                 mock.call("ws1", 10, "label", "color")]
        self.m_view.modify_spectrum_label.assert_has_calls(calls)

    def test_on_workspace_selection_changed(self):
        self.presenter._update_plot = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.m_view.get_selection.return_value = {}
        self.presenter.on_workspace_selection_changed()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_spectrum_slider.reset_mock()
        self.m_view.get_selection.return_value = {"ws1": [1], "ws2": [1]}
        self.presenter.on_workspace_selection_changed()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_spectrum_slider.reset_mock()
        self.m_view.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_workspace_selection_changed()
        self.presenter._update_spectrum_slider.assert_called_once()

    def test_on_spectrum_slider_moved(self):
        self.presenter._update_hold_button = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter.on_spectrum_slider_moved(1)
        self.m_view.set_spectrum_spin_box_value.assert_called_once_with(1)
        self.presenter._update_hold_button.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_on_spectrum_spin_box_changed(self):
        self.presenter._update_hold_button = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter.on_spectrum_spin_box_changed(1)
        self.m_view.set_spectrum_slider_position.assert_called_once()
        self.presenter._update_hold_button.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_on_del_spectrum_button_clicked(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.m_view.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_del_spectrum_button_clicked("ws3", 10)
        self.presenter._update_list.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.m_view.set_selection.assert_called_once_with({"ws1": [1],
                                                           "ws2": [2]})
        self.m_view.reset_mock()
        self.presenter.on_del_spectrum_button_clicked("ws1", 1)
        self.m_view.set_selection.assert_called_once_with({"ws2": [2]})

    def test_on_hold(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.m_view.is_spectrum_selection_disabled.return_value = False
        self.m_view.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.m_view.get_spectrum_slider_position.return_value = 10
        self.m_view.get_mode.return_value = self.presenter.SPECTRUM_MODE_TEXT
        self.presenter._on_hold()
        calls = [mock.call("ws1", 10), mock.call("ws2", 10)]
        self.m_model.add_data.assert_has_calls(calls)
        self.m_model.set_spectrum_mode.assert_called_once()
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.m_view.set_selection.assert_called_once_with({"ws1": [10],
                                                           "ws2": [10]})
        self.m_view.get_mode.return_value = self.presenter.BIN_MODE_TEXT
        self.presenter._on_hold()
        self.m_model.set_bin_mode.assert_called_once()

    def test_on_un_hold(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.presenter._update_spectrum_slider = mock.Mock()
        self.m_view.is_spectrum_selection_disabled.return_value = False
        self.m_view.get_selection.return_value = {"ws1": [], "ws2": []}
        self.m_view.get_spectrum_slider_position.return_value = 10
        self.presenter._on_un_hold()
        calls = [mock.call("ws1", 10), mock.call("ws2", 10)]
        self.m_model.remove_data.assert_has_calls(calls)
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()
        self.presenter._update_spectrum_slider.assert_called_once()

    def test_on_hold_button_clicked(self):
        self.presenter._on_hold = mock.Mock()
        self.presenter._on_un_hold = mock.Mock()
        self.m_view.get_hold_button_text = mock.Mock()
        self.m_view.get_hold_button_text.return_value = \
            self.presenter.HOLD_BUTTON_TEXT_CHECKED
        self.presenter.on_hold_button_clicked()
        self.presenter._on_un_hold.assert_called_once()
        self.m_view.get_hold_button_text.return_value = \
            self.presenter.HOLD_BUTTON_TEXT_UNCHECKED
        self.presenter.on_hold_button_clicked()
        self.presenter._on_hold.assert_called_once()

    def test_on_mode_changed(self):
        self.presenter._update_spectrum_slider = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.m_view.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_mode_changed("mode")
        self.presenter._update_spectrum_slider.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_on_workspace_releted(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.m_view.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_workspace_deleted("ws1")
        self.m_view.set_selection.assert_called_once_with({"ws2": [2]})
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_on_workspace_renamed(self):
        self.presenter._update_list = mock.Mock()
        self.presenter._update_plot = mock.Mock()
        self.m_view.get_selection.return_value = {"ws1": [1], "ws2": [2]}
        self.presenter.on_workspace_renamed("ws1", "ws3")
        self.m_view.set_selection.assert_called_once_with({"ws3": [1],
                                                           "ws2": [2]})
        self.presenter._update_list.assert_called_once()
        self.presenter._update_plot.assert_called_once()

    def test_on_workspace_replaced(self):
        self.presenter._update_plot = mock.Mock()
        self.presenter.on_workspace_replaced("ws1")
        self.presenter._update_plot.assert_called_once()
