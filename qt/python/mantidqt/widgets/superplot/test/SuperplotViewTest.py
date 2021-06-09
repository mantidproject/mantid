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

from mantidqt.widgets.superplot.SuperplotView import SuperplotView


qapp = QApplication(sys.argv)


class SuperplotViewTest(unittest.TestCase):

    def setUp(self):
        pyModule = "mantidqt.widgets.superplot.SuperplotView"

        patch = mock.patch(pyModule + ".SuperplotViewSide")
        self.mDockSide = patch.start()
        self.mDockSide = self.mDockSide.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(pyModule + ".SuperplotViewBottom")
        self.mDockBottom = patch.start()
        self.mDockBottom = self.mDockBottom.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch(pyModule + ".WorkspaceItem")
        self.mWsItem = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch(pyModule + ".SpectrumItem")
        self.mSpItem = patch.start()
        self.addCleanup(patch.stop)

        self.mPresenter = mock.Mock()
        self.view = SuperplotView(self.mPresenter)

    def test_getSideWidget(self):
        self.assertEqual(self.view.get_side_widget(), self.view._side_view)

    def test_getBottomWidget(self):
        self.assertEqual(self.view.get_bottom_widget(), self.view._bottom_view)

    def test_close(self):
        self.view.close()
        self.mDockSide.close.assert_called_once()
        self.mDockBottom.close.assert_called_once()

    def test_getSelectedWorkspace(self):
        self.view.get_selected_workspace()
        self.mDockSide.workspaceSelector.currentText.assert_called_once()

    def test_setSelection(self):
        self.mDockSide.workspacesList.topLevelItemCount.return_value = 3
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
        self.mDockSide.workspacesList.topLevelItem.side_effect = [ws1, ws2, ws3]
        self.view.set_selection({"ws1": [1, 2, 3], "ws2": [-1]})
        self.mDockSide.workspacesList.clearSelection.assert_called_once()
        ws1.setSelected.assert_not_called()
        sp1.setSelected.assert_called_once()
        sp2.setSelected.assert_called_once()
        sp3.setSelected.assert_called_once()
        sp4.setSelected.assert_not_called()
        ws2.setSelected.assert_called_once()
        ws3.setSelected.assert_not_called()

    def test_getSelection(self):
        ws1Item = mock.Mock()
        ws1Item.get_workspace_name.return_value = "ws1"
        ws1Item.parent.return_value = None
        sp1Item = mock.Mock()
        sp1Item.get_spectrum_index.return_value = 1
        sp1Item.parent.return_value = ws1Item
        sp2Item = mock.Mock()
        sp2Item.get_spectrum_index.return_value = 2
        sp2Item.parent.return_value = ws1Item
        ws2Item = mock.Mock()
        ws2Item.get_workspace_name.return_value = "ws2"
        ws2Item.parent.return_value = None
        self.mDockSide.workspacesList.selectedItems.return_value = [sp1Item,
                                                                    sp2Item,
                                                                    ws2Item]
        self.assertDictEqual(self.view.get_selection(), {"ws1": [1, 2],
                                                        "ws2": [-1]})

    def test_modifySpectrumLabel(self):
        self.mDockSide.workspacesList.findItems.return_value = []
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
        self.mDockSide.workspacesList.findItems.return_value = [it1, it2]
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

    def test_setWorkspacesList(self):
        self.view.set_workspaces_list(["ws1", "ws2", "ws3"])
        self.mDockSide.workspacesList.clear.assert_called_once()
        calls = [mock.call(self.mDockSide.workspacesList, "ws1"),
                 mock.call(self.mDockSide.workspacesList, "ws2"),
                 mock.call(self.mDockSide.workspacesList, "ws3")]
        self.mWsItem.assert_has_calls(calls, any_order=True)

    def test_setSpectraList(self):
        it1 = mock.Mock()
        self.mDockSide.workspacesList.findItems.return_value = [it1]
        self.view.set_spectra_list("wsName", [1, 2, 3])
        it1.takeChildren.assert_called_once()
        calls = [mock.call(it1, 1), mock.call(it1, 2), mock.call(it1, 3)]
        self.mSpItem.assert_has_calls(calls, any_order=True)

    def test_getSpectraList(self):
        ws1 = mock.Mock()
        ws1.childCount.return_value = 2
        sp1 = mock.Mock()
        sp1.get_spectrum_index.return_value = 1
        sp2 = mock.Mock()
        sp2.get_spectrum_index.return_value = 2
        ws1.child.side_effect = [sp1, sp2]
        self.mDockSide.workspacesList.findItems.return_value = []
        self.assertEqual(self.view.get_spectra_list("wsName"), [])
        self.mDockSide.workspacesList.findItems.return_value = [ws1]
        self.assertEqual(self.view.get_spectra_list("wsName"), [1, 2])

    def test_checkHoldButton(self):
        widget = self.mDockBottom.holdButton
        self.view.check_hold_button(True)
        widget.setChecked.assert_called_once_with(True)

    def test_setSpectrumDisabled(self):
        widget1 = self.mDockBottom.spectrumSlider
        widget2 = self.mDockBottom.spectrumSpinBox
        self.view.set_spectrum_disabled(True)
        widget1.setDisabled.assert_called_once_with(True)
        widget2.setDisabled.assert_called_once_with(True)

    def test_getSpectrumDisabled(self):
        widget = self.mDockBottom.spectrumSlider
        widget.isEnabled.return_value = True
        self.assertFalse(self.view.is_spectrum_disabled())
        widget.isEnabled.assert_called_once()

    def setSpectrumSliderMax(self):
        widget = self.mDockBottom.spectrumSlider
        self.view.set_spectrum_slider_max(100)
        widget.setMaximum.assert_called_once_with(100)

    def test_setSpectrumSliderPosition(self):
        widget = self.mDockBottom.spectrumSlider
        self.view.set_spectrum_slider_position(10)
        widget.setSliderPosition.assert_called_once_with(10)

    def test_getSpectrumSliderPosition(self):
        widget = self.mDockBottom.spectrumSlider
        widget.value.return_value = 10
        value = self.view.get_spectrum_slider_position()
        self.assertEqual(value, 10)
        widget.value.assert_called_once()

    def test_setSpectrumSpinBoxMax(self):
        widget = self.mDockBottom.spectrumSpinBox
        self.view.set_spectrum_spin_box_max(100)
        widget.setMaximum.assert_called_once_with(100)

    def test_setSpectrumSpinBoxValue(self):
        widget = self.mDockBottom.spectrumSpinBox
        self.view.set_spectrum_spin_box_value(10)
        widget.setValue.assert_called_once_with(10)

    def test_getSpectrumSpinBoxValue(self):
        widget = self.mDockBottom.spectrumSpinBox
        widget.value.return_value = 10
        value = self.view.get_spectrum_spin_box_value()
        self.assertEqual(value, 10)
        widget.value.assert_called_once()

    def test_setAvailableModes(self):
        widget = self.mDockBottom.modeComboBox
        self.view.set_available_modes(["mode1", "mode2"])
        widget.addItems.assert_called_once_with(["mode1", "mode2"])

    def test_setMode(self):
        widget = self.mDockBottom.modeComboBox
        self.view.set_mode("mode")
        widget.setCurrentText.assert_called_once_with("mode")

    def test_getMode(self):
        widget = self.mDockBottom.modeComboBox
        widget.currentText.return_value = "mode1"
        mode = self.view.get_mode()
        self.assertEqual(mode, "mode1")
        widget.currentText.assert_called_once()
