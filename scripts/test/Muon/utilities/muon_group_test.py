# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantid.simpleapi import CreateWorkspace


class MuonGroupTest(unittest.TestCase):
    """
    The MuonGroup object encapsulates the information the describes a group:

    - Its name (string)
    - A list of detector ids (list of ints)
    - A workspace (Optional)

    It is intended to be lightweight, but with some type checking and basic logic, intended
    to prevent obvious misuse. The class is used by the MuonAnalysis interface and so these
    tests should be amended with care.
    """

    def create_group_populated_by_two_workspace(self):
        group = MuonGroup(group_name="group1")
        counts_workspace_22222 = CreateWorkspace([0], [0])
        asymmetry_workspace_22222 = CreateWorkspace([0], [0])
        group.update_workspaces([22222], counts_workspace_22222, asymmetry_workspace_22222, False)
        group.show_raw([22222], 'counts_name_22222', 'asymmetry_name_22222')
        counts_workspace_33333 = CreateWorkspace([0], [0])
        asymmetry_workspace_33333 = CreateWorkspace([0], [0])
        group.update_workspaces([33333], counts_workspace_33333, asymmetry_workspace_33333, False)
        group.show_raw([33333], 'counts_name_33333', 'asymmetry_name_33333')

        return group

    def create_group_populated_by_two_rebinned_workspaces(self):
        group = MuonGroup(group_name="group1")
        counts_workspace_22222 = CreateWorkspace([0], [0])
        asymmetry_workspace_22222 = CreateWorkspace([0], [0])
        group.update_workspaces([22222], counts_workspace_22222, asymmetry_workspace_22222, True)
        group.show_rebin([22222], 'counts_name_22222', 'asymmetry_name_22222')
        counts_workspace_33333 = CreateWorkspace([0], [0])
        asymmetry_workspace_33333 = CreateWorkspace([0], [0])
        group.update_workspaces([33333], counts_workspace_33333, asymmetry_workspace_33333, True)
        group.show_rebin([33333], 'counts_name_33333', 'asymmetry_name_33333')

        return group

    def test_that_cannot_initialize_MuonGroup_without_name(self):
        with self.assertRaises(TypeError):
            MuonGroup()

    def test_that_MuonGroup_name_is_set_correctly(self):
        group = MuonGroup(group_name="group1")

        self.assertEqual(group.name, "group1")

    def test_that_cannot_set_new_name_on_group(self):
        group = MuonGroup(group_name="group1")

        with self.assertRaises(AttributeError):
            group.name = "new_name"
        self.assertEqual(group.name, "group1")

    def test_that_can_set_workspace_if_MuonWorkspace_object(self):
        group = MuonGroup(group_name="group1")
        dataX = [0, 1, 2, 3, 4, 5]
        dataY = [10, 20, 30, 20, 10]
        input_workspace = CreateWorkspace(dataX, dataY)

        self.assertEqual(group.workspace, {})
        group.workspace = MuonWorkspaceWrapper(input_workspace)
        self.assertIsNotNone(group.workspace)

    def test_that_AttributeError_thrown_if_setting_workspace_to_non_MuonWorkspace_object(self):
        group = MuonGroup(group_name="group1")
        self.assertEqual(group.workspace, {})
        
        with self.assertRaises(AttributeError):
            group.workspace = [1, 2, 3]
        self.assertEqual(group.workspace, {})

    def test_that_detector_ids_cannot_be_set_as_string(self):
        group = MuonGroup(group_name="group1")

        with self.assertRaises(AttributeError):
            group.detectors = "1"
            group.detectors = "[1]"
            group.detectors = "1,2"

    def test_that_detectors_set_as_list_of_ints_is_set_correctly(self):
        group = MuonGroup(group_name="group1")

        group.detectors = [1, 2, 3, 4, 5]

        self.assertEqual(group.detectors, [1, 2, 3, 4, 5])

    def test_that_detectors_always_in_ascending_order(self):
        group = MuonGroup(group_name="group1")

        group.detectors = [5, 4, 3, 2, 1]

        self.assertEqual(group.detectors, [1, 2, 3, 4, 5])

    def test_that_duplicated_detectors_are_removed(self):
        group = MuonGroup(group_name="group1")

        group.detectors = [1, 1, 2, 2, 3, 3, 4, 4, 5, 5]

        self.assertEqual(group.detectors, [1, 2, 3, 4, 5])

    def test_that_number_of_detectors_is_correct(self):
        group = MuonGroup(group_name="group1")

        group.detectors = [1, 1, 2, 2, 3, 3, 4, 4, 5, 5]

        self.assertEqual(group.n_detectors, 5)

    def test_that_can_have_group_with_no_detectors(self):
        group = MuonGroup(group_name="group1")

        group.detectors = []

        self.assertEqual(group.detectors, [])
        self.assertEqual(group.n_detectors, 0)

    def test_get_asymmetry_workspace_names_returns_relevant_workspace_names_if_workspace_is_not_hidden(self):
        group = self.create_group_populated_by_two_workspace()

        workspace_list = group.get_asymmetry_workspace_names([[22222]])

        self.assertEqual(workspace_list, ['asymmetry_name_22222'])

    def test_get_asymmetry_workspace_names_returns_nothing_if_workspace_is_hidden(self):
        group = self.create_group_populated_by_two_workspace()
        group._asymmetry_estimate[str([22222])].hide()

        workspace_list = group.get_asymmetry_workspace_names([[22222]])

        self.assertEqual(workspace_list, [])

    def test_get_asymmetry_workspace_names_returns_relevant_workspace_names_if_workspace_is_not_hidden_for_rebinned(self):
        group = self.create_group_populated_by_two_rebinned_workspaces()

        workspace_list = group.get_asymmetry_workspace_names_rebinned([[22222]])

        self.assertEqual(workspace_list, ['asymmetry_name_22222'])

    def test_get_asymmetry_workspace_names_returns_nothing_if_workspace_is_hidden_for_rebinned(self):
        group = self.create_group_populated_by_two_rebinned_workspaces()
        group._asymmetry_estimate_rebin[str([22222])].hide()

        workspace_list = group.get_asymmetry_workspace_names_rebinned([[22222]])

        self.assertEqual(workspace_list, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
