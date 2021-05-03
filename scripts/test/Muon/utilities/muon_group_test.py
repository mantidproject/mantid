# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.muon_group import MuonGroup, MuonRun
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantid.simpleapi import CreateWorkspace
from Muon.GUI.Common.test_helpers.general_test_helpers import (create_group_populated_by_two_workspace,
                                                               create_group_populated_by_two_rebinned_workspaces,
                                                               create_group_populated_by_two_binned_and_two_unbinned_workspaces)


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
    def test_that_cannot_initialize_MuonGroup_without_name(self):
        with self.assertRaises(TypeError):
            MuonGroup()

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
        group = create_group_populated_by_two_workspace()

        workspace_list = group.get_asymmetry_workspace_names([[22222]])

        self.assertEqual(workspace_list, ['asymmetry_name_22222'])

    def test_get_asymmetry_workspace_names_returns_nothing_if_workspace_is_hidden(self):
        group = create_group_populated_by_two_workspace()
        group._asymmetry_estimate[MuonRun([22222])].hide()

        workspace_list = group.get_asymmetry_workspace_names([[22222]])

        self.assertEqual(workspace_list, [])

    def test_get_asymmetry_workspace_names_returns_relevant_workspace_names_if_workspace_is_not_hidden_for_rebinned(self):
        group = create_group_populated_by_two_rebinned_workspaces()

        workspace_list = group.get_asymmetry_workspace_names_rebinned([[22222]])

        self.assertEqual(workspace_list, ['asymmetry_name_22222_rebin'])

    def test_get_asymmetry_workspace_names_returns_nothing_if_workspace_is_hidden_for_rebinned(self):
        group = create_group_populated_by_two_rebinned_workspaces()
        group._asymmetry_estimate_rebin[MuonRun([22222])].hide()

        workspace_list = group.get_asymmetry_workspace_names_rebinned([[22222]])

        self.assertEqual(workspace_list, [])

    def test_unbinned_asymmetry_name_returns_binned_name(self):
        group = create_group_populated_by_two_binned_and_two_unbinned_workspaces()

        rebinned_workspace_name = group.get_rebined_or_unbinned_version_of_workspace_if_it_exists('asymmetry_name_33333')

        self.assertEqual(rebinned_workspace_name, 'asymmetry_name_33333_rebin')

    def test_unbinned_counts_workspace_returns_binned_name(self):
        group = create_group_populated_by_two_binned_and_two_unbinned_workspaces()

        rebinned_workspace_name = group.get_rebined_or_unbinned_version_of_workspace_if_it_exists('counts_name_33333')

        self.assertEqual(rebinned_workspace_name, 'counts_name_33333_rebin')

    def test_binned_counts_workspace_returns_unbinned_name(self):
        group = create_group_populated_by_two_binned_and_two_unbinned_workspaces()

        rebinned_workspace_name = group.get_rebined_or_unbinned_version_of_workspace_if_it_exists('counts_name_33333_rebin')

        self.assertEqual(rebinned_workspace_name, 'counts_name_33333')

    def test_binned_asymmetry_name_returns_unbinned_name(self):
        group = create_group_populated_by_two_binned_and_two_unbinned_workspaces()

        rebinned_workspace_name = group.get_rebined_or_unbinned_version_of_workspace_if_it_exists('asymmetry_name_33333_rebin')

        self.assertEqual(rebinned_workspace_name, 'asymmetry_name_33333')

    def test_nothing_returned_if_workspace_does_not_exist(self):
        group = create_group_populated_by_two_binned_and_two_unbinned_workspaces()

        rebinned_workspace_name = group.get_rebined_or_unbinned_version_of_workspace_if_it_exists('asymmetry_name_43333_rebin')

        self.assertEqual(rebinned_workspace_name, None)

    def test_nothing_returned_equivalent_workspace_does_not_exist(self):
        group = create_group_populated_by_two_workspace()

        rebinned_workspace_name = group.get_rebined_or_unbinned_version_of_workspace_if_it_exists('asymmetry_name_33333')

        self.assertEqual(rebinned_workspace_name, None)

    def test_that_default_period_for_group_set_correctly(self):
        group = MuonGroup(group_name="group1")

        self.assertEqual(group.periods, [1])

    def test_get_counts_workspace_for_run_returns_workspace_name_if_it_exists(self):
        group = create_group_populated_by_two_binned_and_two_unbinned_workspaces()

        counts_workspace_name = group.get_counts_workspace_for_run([22222], False)
        rebinned_counts_workspace_name = group.get_counts_workspace_for_run([22222], True)

        self.assertEqual(counts_workspace_name, 'counts_name_22222')
        self.assertEqual(rebinned_counts_workspace_name, 'counts_name_22222_rebin')

    def test_get_counts_workspace_for_run_throws_a_key_error_if_workspace_not_found(self):
        group = create_group_populated_by_two_binned_and_two_unbinned_workspaces()

        with self.assertRaises(KeyError):
            group.get_counts_workspace_for_run([222223], True)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
