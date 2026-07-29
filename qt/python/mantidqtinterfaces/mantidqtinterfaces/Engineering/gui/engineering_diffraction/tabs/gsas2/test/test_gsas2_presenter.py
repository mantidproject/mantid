# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import patch


from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2 import model, view, presenter

# from testhelpers import assertRaisesNothing

presenter_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2"


class TestGSAS2Presenter(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.GSAS2View, instance=True)
        self.model = mock.create_autospec(model.GSAS2Model, instance=True)
        self.model.x_limits = mock.create_autospec(model.XLimitsState, instance=True)

        self.presenter = presenter.GSAS2Presenter(self.model, self.view, test=True)

    @patch(presenter_path + ".view.GSAS2View.get_refinement_parameters")
    @patch(presenter_path + ".view.GSAS2View.get_project_name")
    @patch(presenter_path + ".presenter.GSAS2Presenter._get_load_parameters")
    @patch(presenter_path + ".presenter.GSAS2Presenter.save_latest_load_parameters")
    @patch(presenter_path + ".presenter.GSAS2Presenter.get_limits_if_same_load_parameters")
    @patch(presenter_path + ".presenter.GSAS2Presenter.clear_plot")
    def test_on_refine_clicked_refined_successful(
        self, mock_clear, mock_saved_limits, mock_save_load, mock_get_load, mock_get_project, mock_get_refine
    ):
        mock_get_load.return_value = ["inst", "phase", "data"]
        mock_get_project.return_value = "project_name"
        mock_get_refine.return_value = ["Pawley", "3.65", False, True, True]
        mock_saved_limits.return_value = [18500, 50000]
        self.model.x_limits.x_min = [0, 1, 2]
        self.model.x_limits.x_max = [3, 4, 5]
        self.presenter.rb_num = "experiment"
        self.view.get_axes.return_value = ["axis"]

        self.model.run_model.return_value = 1
        self.presenter.on_refine_clicked()

        self.assertEqual(mock_clear.call_count, 2)
        self.assertEqual(self.model.run_model.call_count, 1)
        self.assertEqual(self.model.plot_result.call_count, 1)
        self.view.set_number_histograms.assert_called_once_with(1)
        mock_save_load.assert_called_once()

    @patch(presenter_path + ".presenter.GSAS2Presenter.save_latest_load_parameters")
    @patch(presenter_path + ".presenter.GSAS2Presenter.get_limits_if_same_load_parameters")
    @patch(presenter_path + ".presenter.GSAS2Presenter.clear_plot")
    def test_on_refine_clicked_refined_failed(self, mock_clear, mock_saved_limits, mock_save_load):
        mock_saved_limits.return_value = [18500, 50000]
        self.model.x_limits.x_min = [0, 1, 2]
        self.model.x_limits.x_max = [3, 4, 5]
        self.presenter.rb_num = "experiment"
        self.view.get_axes.return_value = ["axis"]

        self.model.run_model.return_value = 0
        self.presenter.on_refine_clicked()

        self.assertEqual(mock_clear.call_count, 1)
        self.assertEqual(self.model.run_model.call_count, 1)
        self.assertEqual(self.model.plot_result.call_count, 0)
        self.assertEqual(self.view.set_number_histograms.call_count, 0)
        self.assertEqual(mock_save_load.call_count, 0)

    @patch(presenter_path + ".presenter.GSAS2Presenter.plot_result")
    def test_on_plot_index_changed(self, mock_plot_result):
        self.presenter.current_plot_index = 1

        # Same index
        self.presenter.on_plot_index_changed(new_plot_index=None)
        self.assertEqual(mock_plot_result.call_count, 0)
        # No new index
        self.presenter.on_plot_index_changed(new_plot_index=1)
        self.assertEqual(mock_plot_result.call_count, 0)
        # New index is different
        self.presenter.on_plot_index_changed(new_plot_index=2)
        self.assertEqual(mock_plot_result.call_count, 1)

    def test_get_limits_if_same_load_parameters(self):
        self.presenter.latest_load_parameters = ["inst", "phase", "data"]
        self.view.initial_x_limits = [17000, 51000]

        # no new load params
        self._patch_parameters_from_view()
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), None)
        # load params different
        self._patch_parameters_from_view("inst_DIFFERENT", "phase", "data")
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), None)
        # no current limits
        self.view.get_x_limits_from_line_edits.return_value = None
        self._patch_parameters_from_view("inst", "phase", "data")
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), None)
        # Success
        self.view.get_x_limits_from_line_edits.return_value = [["18000"], ["50000"]]
        self._patch_parameters_from_view("inst", "phase", "data")
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), [[18000.0], [50000.0]])
        # Success with limits reversed
        self.view.get_x_limits_from_line_edits.return_value = [["50000"], ["8000"]]
        self._patch_parameters_from_view("inst", "phase", "data")
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), [[8000.0], [50000.0]])

    def _patch_parameters_from_view(self, instr=None, phase=None, data=None):
        self.view.get_instrument_group.return_value = instr
        self.model.get_phase_files.return_value = phase
        self.view.get_focused_data.return_value = data

    # ================
    # Phase Selection
    # ================

    def test_get_load_parameters_combines_view_inputs_with_resolved_phase(self):
        self.view.get_phase_combo_text.return_value = "FE_GAMMA"
        self.view.get_phase_finder_file.return_value = ["custom.cif"]
        self.view.get_instrument_group.return_value = ["inst.prm"]
        self.view.get_focused_data.return_value = ["data.gss"]
        self.model.get_phase_files.return_value = ["resolved_phase.cif"]

        result = self.presenter._get_load_parameters()

        # The phases are resolved by the model from the combo selection and the finder paths
        self.model.get_phase_files.assert_called_once_with("FE_GAMMA", ["custom.cif"])
        self.assertEqual(result, [["inst.prm"], ["resolved_phase.cif"], ["data.gss"]])

    def test_populate_phase_combo_box_sets_options_and_refreshes_visibility(self):
        self.view.reset_mock()
        self.model.get_cif_combo_options.return_value = ["AL", "FE_GAMMA", "Custom"]

        with patch.object(self.presenter, "phase_combo_changed") as mock_changed:
            self.presenter.populate_phase_combo_box()

        self.view.set_cif_combo_options.assert_called_once_with(["AL", "FE_GAMMA", "Custom"])
        mock_changed.assert_called_once()

    def test_phase_combo_changed_shows_finder_when_custom_selected(self):
        self.view.get_phase_combo_text.return_value = "Custom"
        self.model.phase_is_custom.return_value = True

        self.presenter.phase_combo_changed()

        self.model.phase_is_custom.assert_called_with("Custom")
        self.view.set_phase_finder_visible.assert_called_with(True)

    def test_phase_combo_changed_hides_finder_for_default_phase(self):
        self.view.get_phase_combo_text.return_value = "FE_GAMMA"
        self.model.phase_is_custom.return_value = False

        self.presenter.phase_combo_changed()

        self.model.phase_is_custom.assert_called_with("FE_GAMMA")
        self.view.set_phase_finder_visible.assert_called_with(False)


if __name__ == "__main__":
    unittest.main()
