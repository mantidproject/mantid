# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class BasePaneModelTest(unittest.TestCase):
    def setUp(self):
        self.model = BasePaneModel(context=setup_context(False))

    def test_get_workspace_to_plot(self):
        self.assertEqual(self.model.get_workspaces_to_plot(True, True), [])

    def test_get_workspace_list_and_indices_to_plot_returns_correctly(self):
        workspaces, indices = self.model.get_workspace_list_and_indices_to_plot(True, "Asymmetry")

        self.assertEqual(workspaces, [])
        self.assertEqual(indices, [])

    def test_get_workspaces_to_remove(self):
        workspaces = self.model.get_workspaces_to_remove("fwd", True, "Asymmetry")

        self.assertEqual(workspaces, [])

    def test_create_tiled_keys_returns_correctly_for_tiled_by_group(self):
        keys = self.model.create_tiled_keys("something")

        self.assertEqual(keys, [])


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
