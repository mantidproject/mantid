# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
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
        self.view = mock.create_autospec(view.GSAS2View)
        self.model = mock.create_autospec(model.GSAS2Model)
        self.presenter = presenter.GSAS2Presenter(self.model, self.view, test=True)

    @patch(presenter_path + ".view.GSAS2View.get_refinement_parameters")
    @patch(presenter_path + ".view.GSAS2View.get_project_name")
    @patch(presenter_path + ".view.GSAS2View.get_load_parameters")
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
        self.model.x_min = [0, 1, 2]
        self.model.x_max = [3, 4, 5]
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
        self.model.x_min = [0, 1, 2]
        self.model.x_max = [3, 4, 5]
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
        self.view.get_load_parameters.return_value = []
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), None)
        # load params different
        self.view.get_load_parameters.return_value = ["inst_DIFFERENT", "phase", "data"]
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), None)
        # no current limits
        self.view.get_x_limits_from_line_edits.return_value = None
        self.view.get_load_parameters.return_value = ["inst", "phase", "data"]
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), None)
        # Success
        self.view.get_x_limits_from_line_edits.return_value = [["18000"], ["50000"]]
        self.view.get_load_parameters.return_value = ["inst", "phase", "data"]
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), [[18000.0], [50000.0]])
        # Success with limits reversed
        self.view.get_x_limits_from_line_edits.return_value = [["50000"], ["8000"]]
        self.view.get_load_parameters.return_value = ["inst", "phase", "data"]
        self.assertEqual(self.presenter.get_limits_if_same_load_parameters(), [[8000.0], [50000.0]])


if __name__ == "__main__":
    unittest.main()
