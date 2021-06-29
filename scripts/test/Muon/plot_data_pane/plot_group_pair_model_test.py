# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.plot_widget.data_pane.plot_group_pair_model import PlotGroupPairModel
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class PlotGroupPairModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.model = PlotGroupPairModel(context=self.context, name="test")
        self.context.group_pair_context._groups = []
        self.context.group_pair_context._pairs = []
        self.context.group_pair_context._selected_pairs = []

        self.context.group_pair_context.add_group(MuonGroup(group_name="fwd", detector_ids=[1]))
        self.context.group_pair_context.add_group(MuonGroup(group_name="bwd", detector_ids=[2]))
        self.context.group_pair_context.add_group(MuonGroup(group_name="top", detector_ids=[3]))
        self.context.group_pair_context._selected_groups = ["fwd", "bwd", "top"]
        runs = [[62260], [62261]]
        self.context.data_context.current_runs = runs

    def test_create_tiled_keys_returns_correctly_for_tiled_by_run(self):
        keys = self.model.create_tiled_keys("Run")
        self.assertEqual(keys, ['62260', '62261'])

    def test_create_tiled_keys_returns_correctly_for_summed_runs_tiled_by_run(self):
        runs = [[62260, 62261]]
        self.context.data_context.current_runs = runs

        keys = self.model.create_tiled_keys("Run")
        self.assertEqual(keys, ['62260-62261'])

    def test_create_tiled_keys_returns_correctly_for_tiled_by_group(self):
        keys = self.model.create_tiled_keys("Group/Pair")
        self.assertEqual(keys, ["fwd", "bwd", "top"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
