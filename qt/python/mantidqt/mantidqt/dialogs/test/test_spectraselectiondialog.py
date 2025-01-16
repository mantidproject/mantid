# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialogButtonBox

from mantid.api import WorkspaceFactory
from unittest import mock
from mantid.simpleapi import AddSampleLog, CreateSampleWorkspace, ExtractSpectra
from mantidqt.dialogs import spectraselectordialog
from mantidqt.dialogs.spectraselectordialog import parse_selection_str, SpectraSelectionDialog
from mantidqt.dialogs.spectraselectorutils import get_spectra_selection
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class SpectraSelectionDialogTest(unittest.TestCase):
    _mock_get_icon = None
    _single_spec_ws = None
    _multi_spec_ws = None

    def setUp(self):
        # patch away getting a real icon as it can hit a race condition when running tests
        # in parallel
        patcher = mock.patch("mantidqt.dialogs.spectraselectordialog.get_icon")
        self._mock_get_icon = patcher.start()
        self._mock_get_icon.return_value = QIcon()
        self.addCleanup(patcher.stop)
        if self._single_spec_ws is None:
            self.__class__._single_spec_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=1, XLength=1, YLength=1)
            self.__class__._multi_spec_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=200, XLength=1, YLength=1)
        SpectraSelectionDialog._check_number_of_plots = mock.Mock(return_value=True)

        spectraselectordialog.RED_ASTERISK = None

    def test_initial_dialog_setup(self):
        workspaces = [self._multi_spec_ws]
        dlg = SpectraSelectionDialog(workspaces)
        self.assertFalse(dlg._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_filling_workspace_details_single_workspace(self):
        workspaces = [self._multi_spec_ws]
        dlg = SpectraSelectionDialog(workspaces)
        self.assertEqual("valid range: 1-200", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-199", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_multiple_workspaces_of_same_size(self):
        workspaces = [self._multi_spec_ws, self._multi_spec_ws]
        dlg = SpectraSelectionDialog(workspaces)
        self.assertEqual("valid range: 1-200", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-199", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_multiple_workspaces_of_different_sizes(self):
        cropped_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=50, XLength=1, YLength=1)
        for i in range(cropped_ws.getNumberHistograms()):
            cropped_ws.getSpectrum(i).setSpectrumNo(51 + i)
        dlg = SpectraSelectionDialog([cropped_ws, self._multi_spec_ws])
        self.assertEqual("valid range: 51-100", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-49", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_single_workspace_with_spectra_gaps(self):
        gappy_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=50, XLength=1, YLength=1)
        for i in range(10):
            gappy_ws.getSpectrum(i).setSpectrumNo(1 + i)
        for i in range(10, 16):
            gappy_ws.getSpectrum(i).setSpectrumNo(1 + (2 * i))
        for i in range(17, 20):
            gappy_ws.getSpectrum(i).setSpectrumNo(1 + i)
        for i in range(20, gappy_ws.getNumberHistograms()):
            gappy_ws.getSpectrum(i).setSpectrumNo(51 + i)
        dlg = SpectraSelectionDialog([gappy_ws])
        self.assertEqual("valid range: 1-10, 17-21, 23, 25, 27, 29, 31, 71-100", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-49", dlg._ui.wkspIndices.placeholderText())

    def test_filling_workspace_details_multiple_workspace_with_spectra_gaps(self):
        gappy_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=50, XLength=1, YLength=1)
        for i in range(20):
            gappy_ws.getSpectrum(i).setSpectrumNo(1 + i)
        for i in range(20, gappy_ws.getNumberHistograms()):
            gappy_ws.getSpectrum(i).setSpectrumNo(161 + i)
        dlg = SpectraSelectionDialog([gappy_ws, self._multi_spec_ws])
        self.assertEqual("valid range: 1-20, 181-200", dlg._ui.specNums.placeholderText())
        self.assertEqual("valid range: 0-49", dlg._ui.wkspIndices.placeholderText())

    def test_valid_text_in_boxes_activates_ok(self):
        dlg = SpectraSelectionDialog([self._multi_spec_ws])

        def do_test(input_box):
            input_box.setText("1")
            self.assertTrue(dlg._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())
            input_box.clear()
            self.assertFalse(dlg._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

        do_test(dlg._ui.wkspIndices)
        do_test(dlg._ui.specNums)

    def test_plot_all_gives_only_spectra_numbers(self):
        dlg = SpectraSelectionDialog([self._multi_spec_ws])
        dlg._ui.buttonBox.button(QDialogButtonBox.YesToAll).click()
        self.assertNotEqual(dlg.selection, None)
        self.assertEqual(dlg.selection.spectra, list(range(1, 201)))
        self.assertEqual(dlg.selection.wksp_indices, None)

    def test_entered_workspace_indices_gives_correct_selection_back(self):
        dlg = SpectraSelectionDialog([self._multi_spec_ws])
        dlg._ui.wkspIndices.setText("1-3,5")
        dlg._ui.buttonBox.button(QDialogButtonBox.Ok).click()

        self.assertNotEqual(dlg.selection, None)
        self.assertEqual(dlg.selection.spectra, None)
        self.assertEqual([1, 2, 3, 5], dlg.selection.wksp_indices)

    def test_entered_spectra_gives_correct_selection_back(self):
        dlg = SpectraSelectionDialog([self._multi_spec_ws])
        dlg._ui.wkspIndices.setText("50-60")
        dlg._ui.buttonBox.button(QDialogButtonBox.Ok).click()

        self._mock_get_icon.assert_called_once_with("mdi.asterisk", "red", 0.6)
        self.assertNotEqual(dlg.selection, None)
        self.assertEqual(dlg.selection.spectra, None)
        self.assertEqual(list(range(50, 61)), dlg.selection.wksp_indices)

    def test_parse_selection_str_single_number(self):
        s = "1"
        self.assertEqual([1], parse_selection_str(s, 1, 3))
        s = "2"
        self.assertEqual([2], parse_selection_str(s, 1, 3))
        s = "3"
        self.assertEqual([3], parse_selection_str(s, 1, 3))
        s = "-1"
        self.assertEqual(parse_selection_str(s, 1, 1), None)
        s = "1"
        self.assertEqual(parse_selection_str(s, 2, 2), None)
        s = "1"
        self.assertEqual(parse_selection_str(s, 2, 3), None)

    def test_parse_selection_str_single_range(self):
        s = "1-3"
        self.assertEqual([1, 2, 3], parse_selection_str(s, 1, 3))
        s = "2-4"
        self.assertEqual([2, 3, 4], parse_selection_str(s, 1, 5))
        s = "2-4"
        self.assertEqual(parse_selection_str(s, 2, 3), None)
        s = "2-4"
        self.assertEqual(parse_selection_str(s, 3, 5), None)

    def test_parse_selection_str_mix_number_range_spaces(self):
        s = "1-3, 5,8,10, 11 ,12-14 , 15 -16, 16- 19"
        self.assertEqual([1, 2, 3, 5, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19], parse_selection_str(s, 1, 20))

    def test_construction_with_non_MatrixWorkspace_type_removes_non_MatrixWorkspaces_from_list(self):
        table = WorkspaceFactory.Instance().createTable()
        workspaces = [self._single_spec_ws, table]
        ssd = SpectraSelectionDialog(workspaces)
        self.assertEqual(ssd._workspaces, [self._single_spec_ws])

    def test_get_spectra_selection_removes_wrong_workspace_types_from_list(self):
        table = WorkspaceFactory.Instance().createTable()
        workspaces = [self._single_spec_ws, table]
        self.assertEqual(get_spectra_selection(workspaces).workspaces, [self._single_spec_ws])

    # ---------- Advanced Plots ----------

    def test_advanced_options_widget_created_when_advanced_is_true(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        self.assertTrue(hasattr(ssd._ui, "advanced_options_widget"))

    def test_advanced_plots_includes_surface_and_contour_when_there_are_more_than_two_workspaces(self):
        workspaces = [self._single_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        self.assertNotEqual(ssd._ui.plotType.findText("Surface"), -1)
        self.assertNotEqual(ssd._ui.plotType.findText("Contour"), -1)

    def test_error_bars_check_box_is_disabled_for_surface_plots(self):
        workspaces = [self._single_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(3)
        self.assertFalse(ssd._ui.advanced_options_widget.ui.error_bars_check_box.isEnabled())

    def test_error_bars_check_box_is_disabled_for_contour_plots(self):
        workspaces = [self._single_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(4)
        self.assertFalse(ssd._ui.advanced_options_widget.ui.error_bars_check_box.isEnabled())

    def test_custom_log_values_line_edit_becomes_enabled_when_log_value_set_to_custom(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        custom_index = ssd._ui.advanced_options_widget.ui.log_value_combo_box.count() - 1
        ssd._ui.advanced_options_widget.ui.log_value_combo_box.setCurrentIndex(custom_index)

        self._mock_get_icon.assert_called_once_with("mdi.asterisk", "red", 0.6)
        self.assertTrue(ssd._ui.advanced_options_widget.ui.custom_log_line_edit.isEnabled())

    def test_log_value_combo_box_contains_custom_option(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        self.assertNotEqual(ssd._ui.advanced_options_widget.ui.log_value_combo_box.findText("Custom"), -1)

    def test_log_value_combo_box_contains_workspace_name_option_for_non_surface_or_contour_plots(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        self.assertNotEqual(ssd._ui.advanced_options_widget.ui.log_value_combo_box.findText("Workspace name"), -1)

    def test_log_value_combo_box_contains_workspace_index_option_for_surface_plots(self):
        workspaces = [self._single_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(3)
        self.assertNotEqual(ssd._ui.advanced_options_widget.ui.log_value_combo_box.findText("Workspace"), -1)

    def test_log_value_combo_box_contains_workspace_index_option_for_contour_plots(self):
        workspaces = [self._single_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(4)
        self.assertNotEqual(ssd._ui.advanced_options_widget.ui.log_value_combo_box.findText("Workspace"), -1)

    def test_log_value_combo_box_contains_sample_logs(self):
        ws1 = CreateSampleWorkspace()
        ws2 = CreateSampleWorkspace()
        workspaces = [ws1, ws2]

        for i, ws in enumerate(workspaces):
            AddSampleLog(Workspace=ws.name(), LogName="Test", LogText=str(i))

        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        self.assertNotEqual(ssd._ui.advanced_options_widget.ui.log_value_combo_box.findText("Test"), -1)

    def test_log_options_are_disabled_for_tiled_plots(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(2)
        self.assertFalse(ssd._ui.advanced_options_widget.ui.log_value_combo_box.isEnabled())

    def test_ok_button_disabled_when_log_value_set_to_custom_and_custom_log_values_left_blank(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.specNums.setText("1")
        custom_index = ssd._ui.advanced_options_widget.ui.log_value_combo_box.count() - 1
        ssd._ui.advanced_options_widget.ui.log_value_combo_box.setCurrentIndex(custom_index)
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_ok_button_disabled_when_log_value_set_to_custom_and_custom_log_values_contains_non_floats(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.specNums.setText("1")
        custom_index = ssd._ui.advanced_options_widget.ui.log_value_combo_box.count() - 1
        ssd._ui.advanced_options_widget.ui.log_value_combo_box.setCurrentIndex(custom_index)
        ssd._ui.advanced_options_widget.ui.custom_log_line_edit.setText("0,1,test")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_ok_button_disabled_when_number_of_custom_log_values_does_not_equal_number_of_plots(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.specNums.setText("1")
        custom_index = ssd._ui.advanced_options_widget.ui.log_value_combo_box.count() - 1
        ssd._ui.advanced_options_widget.ui.log_value_combo_box.setCurrentIndex(custom_index)
        ssd._ui.advanced_options_widget.ui.custom_log_line_edit.setText("0,1")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_ok_button_disabled_when_custom_log_values_contains_repeated_value(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.specNums.setText("3")
        custom_index = ssd._ui.advanced_options_widget.ui.log_value_combo_box.count() - 1
        ssd._ui.advanced_options_widget.ui.log_value_combo_box.setCurrentIndex(custom_index)
        ssd._ui.advanced_options_widget.ui.custom_log_line_edit.setText("0,0,1")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_ok_button_disabled_when_custom_log_values_contains_decreasing_values(self):
        workspaces = [self._single_spec_ws]
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.specNums.setText("3")
        custom_index = ssd._ui.advanced_options_widget.ui.log_value_combo_box.count() - 1
        ssd._ui.advanced_options_widget.ui.log_value_combo_box.setCurrentIndex(custom_index)
        ssd._ui.advanced_options_widget.ui.custom_log_line_edit.setText("2,1,3")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_plot_all_button_disabled_when_plot_type_is_surface(self):
        workspaces = [self._single_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(3)
        self.assertTrue(ssd._ui.buttonBox.button(QDialogButtonBox.YesToAll).isEnabled())

    def test_plot_all_button_disabled_when_plot_type_is_contour(self):
        workspaces = [self._single_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(4)
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.YesToAll).isEnabled())

    def test_ok_button_disabled_when_plot_type_is_surface_and_more_than_one_spectrum_number_entered(self):
        workspaces = [self._multi_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(3)
        ssd._ui.specNums.setText("1,2")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_ok_button_disabled_when_plot_type_is_contour_and_more_than_one_spectrum_number_entered(self):
        workspaces = [self._multi_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(4)
        ssd._ui.specNums.setText("1,2")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_ok_button_disabled_when_plot_type_is_surface_and_more_than_one_workspace_index_entered(self):
        workspaces = [self._multi_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(3)
        ssd._ui.wkspIndices.setText("1,2")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_ok_button_disabled_when_plot_type_is_contour_and_more_than_one_workspace_index_entered(self):
        workspaces = [self._multi_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)
        ssd._ui.plotType.setCurrentIndex(4)
        ssd._ui.wkspIndices.setText("1,2")
        self.assertFalse(ssd._ui.buttonBox.button(QDialogButtonBox.Ok).isEnabled())

    def test_plot_all_sets_correct_selection_values(self):
        workspaces = [self._multi_spec_ws] * 3
        ssd = SpectraSelectionDialog(workspaces, advanced=True)

        ssd._ui.plotType.setCurrentIndex(2)
        ssd._ui.buttonBox.button(QDialogButtonBox.YesToAll).click()

        self.assertEqual(ssd.selection.wksp_indices, None)
        self.assertEqual(ssd.selection.spectra, list(range(1, 201)))
        self.assertEqual(ssd.selection.plot_type, 2)
        self.assertEqual(ssd.selection.errors, False)
        self.assertEqual(ssd.selection.log_name, "Workspace name")
        self.assertEqual(ssd.selection.axis_name, "Workspace name")

    # --------------- failure tests -----------

    def test_set_placeholder_text_raises_error_if_workspaces_have_no_common_spectra(self):
        spectra_1 = ExtractSpectra(InputWorkspace=self._multi_spec_ws, StartWorkspaceIndex=0, EndWorkspaceIndex=5)
        spectra_2 = ExtractSpectra(InputWorkspace=self._multi_spec_ws, StartWorkspaceIndex=6, EndWorkspaceIndex=10)
        workspaces = [spectra_1, spectra_2]
        self.assertRaises(Exception, "Error: Workspaces have no common spectra.", SpectraSelectionDialog, workspaces)


if __name__ == "__main__":
    unittest.main()
