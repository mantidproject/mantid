# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys

from qtpy.QtWidgets import QApplication

from mantidqt.widgets.superplot.view import SuperplotView


qapp = QApplication(sys.argv)


class SuperplotViewTest(unittest.TestCase):

    def setUp(self):
        py_module = "mantidqt.widgets.superplot.view"

        patch = mock.patch(py_module + ".SuperplotViewSide")
        self.m_dock_side = patch.start()
        self.m_dock_side = self.m_dock_side.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(py_module + ".SuperplotViewBottom")
        self.m_dock_bottom = patch.start()
        self.m_dock_bottom = self.m_dock_bottom.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(py_module + ".WorkspaceItem")
        self.m_ws_item = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch(py_module + ".SpectrumItem")
        self.m_sp_item = patch.start()
        self.addCleanup(patch.stop)

        self.m_presenter = mock.Mock()
        self.view = SuperplotView(self.m_presenter)

    def test_get_side_widget(self):
        self.assertEqual(self.view.get_side_widget(), self.view._side_view)

    def test_get_bottom_widget(self):
        self.assertEqual(self.view.get_bottom_widget(), self.view._bottom_view)

    def test_close(self):
        self.view.close()
        self.m_dock_side.close.assert_called_once()
        self.m_dock_bottom.close.assert_called_once()

    def test_get_selected_workspace(self):
        self.view.get_selected_workspace()
        self.m_dock_side.workspaceSelector.currentText.assert_called_once()

    def test_set_selection(self):
        self.m_dock_side.workspacesList.topLevelItemCount.return_value = 3
        ws1 = mock.Mock()
        ws1.get_workspace_name.return_value = "ws1"
        ws1.childCount.return_value = 4
        sp1 = mock.Mock()
        sp1.get_spectrum_index.return_value = 1
        sp2 = mock.Mock()
        sp2.get_spectrum_index.return_value = 2
        sp3 = mock.Mock()
        sp3.get_spectrum_index.return_value = 3
        sp4 = mock.Mock()
        sp4.get_spectrum_index.return_value = 4
        ws1.child.side_effect = [sp1, sp2, sp3, sp4]
        ws2 = mock.Mock()
        ws2.get_workspace_name.return_value = "ws2"
        ws2.childCount.return_value = 2
        ws2.child.side_effect = [sp1, sp2]
        ws3 = mock.Mock()
        ws3.get_workspace_name.return_value = "ws3"
        ws3.childCount.return_value = 0
        self.m_dock_side.workspacesList.topLevelItem.side_effect = [ws1, ws2, ws3]
        self.view.set_selection({"ws1": [1, 2, 3], "ws2": [-1]})
        self.m_dock_side.workspacesList.clearSelection.assert_called_once()
        ws1.setSelected.assert_not_called()
        sp1.setSelected.assert_called_once()
        sp2.setSelected.assert_called_once()
        sp3.setSelected.assert_called_once()
        sp4.setSelected.assert_not_called()
        ws2.setSelected.assert_called_once()
        ws3.setSelected.assert_not_called()

    def test_get_selection(self):
        ws1_item = mock.Mock()
        ws1_item.get_workspace_name.return_value = "ws1"
        ws1_item.parent.return_value = None
        sp1_item = mock.Mock()
        sp1_item.get_spectrum_index.return_value = 1
        sp1_item.parent.return_value = ws1_item
        sp2_item = mock.Mock()
        sp2_item.get_spectrum_index.return_value = 2
        sp2_item.parent.return_value = ws1_item
        ws2_item = mock.Mock()
        ws2_item.get_workspace_name.return_value = "ws2"
        ws2_item.parent.return_value = None
        self.m_dock_side.workspacesList.selectedItems.return_value = [sp1_item,
                                                                      sp2_item,
                                                                      ws2_item]
        self.assertDictEqual(self.view.get_selection(), {"ws1": [1, 2],
                                                         "ws2": [-1]})

    def test_modify_spectrum_label(self):
        self.m_dock_side.workspacesList.findItems.return_value = []
        self.view.modify_spectrum_label("test", 1, "label", "#000000")
        it1 = mock.Mock()
        sp1 = mock.Mock()
        sp1.get_spectrum_index.return_value = 1
        sp2 = mock.Mock()
        sp2.get_spectrum_index.return_value = 2
        it1.childCount.return_value = 2
        it1.child.side_effect = [sp1, sp2]
        it2 = mock.Mock()
        sp3 = mock.Mock()
        sp3.get_spectrum_index.return_value = 3
        it2.childCount.return_value = 1
        it2.child.side_effect = [sp3]
        self.m_dock_side.workspacesList.findItems.return_value = [it1, it2]
        self.view.modify_spectrum_label("wsName", 1, "label", "#000000")
        sp1.foreground.assert_called_once()
        sp1.setForeground.assert_called_once()
        sp1.setText.assert_called_once()
        sp2.foreground.assert_not_called()
        sp2.setForeground.assert_not_called()
        sp2.setText.assert_not_called()
        sp3.foreground.assert_not_called()
        sp3.setForeground.assert_not_called()
        sp3.setText.assert_not_called()

    def test_set_workspaces_list(self):
        self.view.set_workspaces_list(["ws1", "ws2", "ws3"])
        self.m_dock_side.workspacesList.clear.assert_called_once()
        calls = [mock.call(self.m_dock_side.workspacesList, "ws1"),
                 mock.call(self.m_dock_side.workspacesList, "ws2"),
                 mock.call(self.m_dock_side.workspacesList, "ws3")]
        self.m_ws_item.assert_has_calls(calls, any_order=True)

    def test_set_spectra_list(self):
        it1 = mock.Mock()
        self.m_dock_side.workspacesList.findItems.return_value = [it1]
        self.view.set_spectra_list("wsName", [1, 2, 3])
        it1.takeChildren.assert_called_once()
        calls = [mock.call(it1, 1), mock.call(it1, 2), mock.call(it1, 3)]
        self.m_sp_item.assert_has_calls(calls, any_order=True)

    def test_get_spectra_list(self):
        ws1 = mock.Mock()
        ws1.childCount.return_value = 2
        sp1 = mock.Mock()
        sp1.get_spectrum_index.return_value = 1
        sp2 = mock.Mock()
        sp2.get_spectrum_index.return_value = 2
        ws1.child.side_effect = [sp1, sp2]
        self.m_dock_side.workspacesList.findItems.return_value = []
        self.assertEqual(self.view.get_spectra_list("wsName"), [])
        self.m_dock_side.workspacesList.findItems.return_value = [ws1]
        self.assertEqual(self.view.get_spectra_list("wsName"), [1, 2])

    def test_set_hold_button_text(self):
        self.view.set_hold_button_text("test")
        self.m_dock_bottom.holdButton.setText.assert_called_once_with("test")

    def test_get_hold_button_text(self):
        self.m_dock_bottom.holdButton.text.return_value = "test"
        text = self.view.get_hold_button_text()
        self.assertEqual(text, "test")

    def test_set_spectrum_selection_disabled(self):
        widget1 = self.m_dock_bottom.spectrumSlider
        widget2 = self.m_dock_bottom.spectrumSpinBox
        self.view.set_spectrum_selection_disabled(True)
        widget1.setDisabled.assert_called_once_with(True)
        widget2.setDisabled.assert_called_once_with(True)

    def test_get_spectrum_disabled(self):
        widget = self.m_dock_bottom.spectrumSlider
        widget.isEnabled.return_value = True
        self.assertFalse(self.view.is_spectrum_selection_disabled())
        widget.isEnabled.assert_called_once()

    def test_set_spectrum_slider_max(self):
        widget = self.m_dock_bottom.spectrumSlider
        self.view.set_spectrum_slider_max(100)
        widget.setMaximum.assert_called_once_with(100)

    def test_set_spectrum_slider_sosition(self):
        widget = self.m_dock_bottom.spectrumSlider
        self.view.set_spectrum_slider_position(10)
        widget.setSliderPosition.assert_called_once_with(10)

    def test_get_spectrum_slider_position(self):
        widget = self.m_dock_bottom.spectrumSlider
        widget.value.return_value = 10
        value = self.view.get_spectrum_slider_position()
        self.assertEqual(value, 10)
        widget.value.assert_called_once()

    def test_set_spectrum_spin_box_max(self):
        widget = self.m_dock_bottom.spectrumSpinBox
        self.view.set_spectrum_spin_box_max(100)
        widget.setMaximum.assert_called_once_with(100)

    def test_set_spectrum_spin_box_value(self):
        widget = self.m_dock_bottom.spectrumSpinBox
        self.view.set_spectrum_spin_box_value(10)
        widget.setValue.assert_called_once_with(10)

    def test_get_spectrum_spin_box_value(self):
        widget = self.m_dock_bottom.spectrumSpinBox
        widget.value.return_value = 10
        value = self.view.get_spectrum_spin_box_value()
        self.assertEqual(value, 10)
        widget.value.assert_called_once()

    def test_set_available_modes(self):
        widget = self.m_dock_bottom.modeComboBox
        self.view.set_available_modes(["mode1", "mode2"])
        widget.addItems.assert_called_once_with(["mode1", "mode2"])

    def test_set_mode(self):
        widget = self.m_dock_bottom.modeComboBox
        self.view.set_mode("mode")
        widget.setCurrentText.assert_called_once_with("mode")

    def test_get_mode(self):
        widget = self.m_dock_bottom.modeComboBox
        widget.currentText.return_value = "mode1"
        mode = self.view.get_mode()
        self.assertEqual(mode, "mode1")
        widget.currentText.assert_called_once()
