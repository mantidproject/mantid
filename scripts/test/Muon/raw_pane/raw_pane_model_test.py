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

    def test_generate_run_indicies_3_runs(self):
        self.model._max_spec = 4
        ws_list = [mock.Mock(), mock.Mock(),mock.Mock()]
        indices = [k for k in range(self.model._max_spec)]
        indices += indices + indices
        self.assertEqual(indices, self.model._generate_run_indices(ws_list))

    def test_generate_run_indicies(self):
        self.model._max_spec = 12
        ws_list = [mock.Mock()]
        indices = [k for k in range(self.model._max_spec)]
        self.assertEqual(indices, self.model._generate_run_indices(ws_list))

    def test_create_tiled_keys(self):
        self.model._max_spec = 4
        keys = ['Detector: 1', 'Detector: 2', 'Detector: 3', 'Detector: 4']
        self.assertEqual(keys, self.model.create_tiled_keys("unused"))

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


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
