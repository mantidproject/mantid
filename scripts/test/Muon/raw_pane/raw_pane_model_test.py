# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from Muon.GUI.Common.plot_widget.raw_pane.raw_pane_model import RawPaneModel
from Muon.GUI.Common.test_helpers.context_setup import setup_context


class RawPaneModelTest(unittest.TestCase):

    def setUp(self):
        self.context = setup_context(False)
        self.context.data_context.instrument = "MUSR"
        self.model = RawPaneModel(self.context)
        self.name = self.model.name

    def test_generate_run_indices_3_runs(self):
        self.model._max_spec = 4
        ws_list = [mock.Mock(), mock.Mock(),mock.Mock()]
        indices = [k for k in range(self.model._max_spec)]
        self.assertEqual(indices, self.model._generate_run_indices(ws_list, "1:4"))

    def test_generate_run_indices(self):
        self.model._max_spec = 12
        ws_list = [mock.Mock()]
        indices = [k for k in range(self.model._max_spec)]
        self.assertEqual(indices, self.model._generate_run_indices(ws_list, "1:12"))

    def test_generate_run_indices_from_other_detectors(self):
        self.model._max_spec = 4
        ws_list = [mock.Mock()]
        det = "5:8"
        # minus 1 to convert to ws index, add one to max to include in range
        indices = [k for k in range(int(det.split(":")[0])-1, int(det.split(":")[1]))]
        self.assertEqual(indices, self.model._generate_run_indices(ws_list, det))

    def test_get_first_and_last_detector(self):
        # these are detector ID's, first one is at 1
        detectors = "12:34"
        lower, upper = self.model._get_first_and_last_detector_to_plot(detectors)
        # minus 1 to convert to ws index
        self.assertEqual(lower, 11)
        # plus one to include the value in range -> no change to value
        self.assertEqual(upper, 34)

    def test_get_first_and_last_no_detectors(self):
        # these are detector ID's, first one is at 1
        detectors = ""
        lower, upper = self.model._get_first_and_last_detector_to_plot(detectors)
        self.assertEqual(lower, 0)
        self.assertEqual(upper, 0)

    def test_create_tiled_keys(self):
        self.model._max_spec = 4
        keys = ['Detector: 1', 'Detector: 2', 'Detector: 3', 'Detector: 4']
        self.assertEqual(keys, self.model.create_tiled_keys("unused"))

    def test_create_tiled_keys_with_offset(self):
        self.model._max_spec = 4
        keys = ['Detector: 9', 'Detector: 10', 'Detector: 11', 'Detector: 12']
        self.assertEqual(keys, self.model.create_tiled_keys("unused", 9))

    def test_get_workspace_plot_axis(self):
        index = 2
        axis_map = mock.Mock()
        self.context.plot_panes_context[self.name].settings.set_tiled(False)
        self.assertEqual(0, self.model._get_workspace_plot_axis("test", axis_map, index))

    def test_get_workspace_plot_axis_tiled(self):
        index = 2
        axis_map = mock.Mock()
        self.context.plot_panes_context[self.name].settings.set_tiled(True)
        self.assertEqual(index, self.model._get_workspace_plot_axis("test", axis_map, index))

    def test_create_Workspace_label_periods(self):
        name = "MUSR62260_raw_data_period_4 MA"
        index = 5
        self.assertEqual("Run62260_period4_Det6", self.model._create_workspace_label(name,index))

    def test_create_Workspace_label(self):
        name = "MUSR62260_raw_data MA"
        index = 5
        self.assertEqual("Run62260_Det6", self.model._create_workspace_label(name,index))

    @mock.patch('Muon.GUI.Common.plot_widget.raw_pane.raw_pane_model.get_raw_data_workspace_name')
    def test_get_ws_names(self, get_name_mock):
        get_name_mock.return_value = "test"
        self.model.get_num_detectors = mock.Mock(return_value = 100)
        self.model._spec_limit = 16
        names = self.model.get_ws_names("1234", False, 1, "1:16")
        # dont plot all of the spec in one go
        self.assertEqual(len(names), self.model._spec_limit)

    @mock.patch('Muon.GUI.Common.plot_widget.raw_pane.raw_pane_model.get_raw_data_workspace_name')
    def test_get_ws_names_less_spec_than_limit(self, get_name_mock):
        get_name_mock.return_value = "test"
        self.model.get_num_detectors = mock.Mock(return_value = 4)
        self.model._spec_limit = 16
        names = self.model.get_ws_names("1234", False, 1, "1:4")
        # plot the number of spec in ws
        self.assertEqual(len(names), self.model.get_num_detectors())

    def test_check_num_detectors(self):
        self.model.get_num_detectors = mock.Mock(return_value = 24)
        self.model._spec_limit = 12
        self.model._max_spec = 4
        self.model.check_num_detectors()

        self.assertEqual(self.model._max_spec, self.model._spec_limit)

    def test_check_num_detectors_small_number(self):
        self.model.get_num_detectors = mock.Mock(return_value = 4)
        self.model._spec_limit = 12
        self.model._max_spec = 10
        self.model.check_num_detectors()

        self.assertEqual(self.model._max_spec, self.model.get_num_detectors())

    def test_get_workspaces_to_plot(self):
        self.model.get_ws_names = mock.Mock(return_value = ["test"])
        self.context.data_context.num_periods = mock.Mock(return_value =1)
        self.context.data_context._current_runs = [[62260], [62261]]

        names = self.model.get_workspaces_to_plot(True, "Counts", "1:16", "62260")
        self.model.get_ws_names.assert_called_once_with("62260", False, '1', "1:16")
        self.assertEqual(names, ["test"])

    def test_get_workspaces_to_plot_multi_period(self):
        self.model.get_ws_names = mock.Mock(return_value = ["test"])
        self.context.data_context.num_periods = mock.Mock(return_value =2)
        self.context.data_context._current_runs = [[62260], [62261]]

        names = self.model.get_workspaces_to_plot(True, "Counts", "1:16", "62260")
        self.model.get_ws_names.assert_any_call("62260", True, '1', "1:16")
        self.model.get_ws_names.assert_any_call("62260", True, '2', "1:16")
        self.assertEqual(self.model.get_ws_names.call_count, 2)
        self.assertEqual(names, ["test", "test"])

    def test_get_workspaces_to_plot_multi_period_selection(self):
        self.model.get_ws_names = mock.Mock(return_value = ["test"])
        self.context.data_context.num_periods = mock.Mock(return_value =2)
        self.context.data_context._current_runs = [[62260], [62261]]

        names = self.model.get_workspaces_to_plot(True, "Counts", "1:16", "62260","2")
        self.model.get_ws_names.assert_called_once_with("62260", True, '2', "1:16")
        self.assertEqual(names, ["test"])

    def test_convert_index_to_axis(self):
        self.model._max_spec = 4
        self.assertEqual(self.model.convert_index_to_axis(2), 2)

    def test_convert_index_to_axis_greater_than_max(self):
        self.model._max_spec = 4
        # display on second selection starting with 5 -> axis 2
        self.assertEqual(self.model.convert_index_to_axis(6), 2)

    def test_get_detector_options(self):
        self.model.get_num_detectors = mock.Mock(return_value = 16)
        self.model._max_spec = 4
        expected = ["1:4", "5:8", "9:12", "13:16"]
        self.assertEqual(expected, self.model.gen_detector_options())

    def test_get_detector_options_one_selection(self):
        self.model.get_num_detectors = mock.Mock(return_value = 4)
        self.model._max_spec = 16
        expected = ["1:4"]
        self.assertEqual(expected, self.model.gen_detector_options())


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
