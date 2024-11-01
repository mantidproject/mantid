# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from testhelpers import run_algorithm
from mantid.api import mtd, WorkspaceGroup, MatrixWorkspace, AnalysisDataService, WorkspaceFactory


class WorkspaceGroupTest(unittest.TestCase):
    def create_matrix_workspace_in_ADS(self, name):
        run_algorithm("CreateWorkspace", OutputWorkspace=name, DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF")

    def create_group_via_GroupWorkspace_algorithm(self):
        self.create_matrix_workspace_in_ADS("First")
        self.create_matrix_workspace_in_ADS("Second")
        run_algorithm("GroupWorkspaces", InputWorkspaces="First,Second", OutputWorkspace="grouped")
        return mtd["grouped"]

    def tearDown(self):
        AnalysisDataService.clear()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_WorkspaceGroup_can_be_instantiated(self):
        ws_group = WorkspaceGroup()
        self.assertIsNotNone(ws_group)
        self.assertIsInstance(ws_group, WorkspaceGroup)

    def test_that_instantiated_WorkspaceGroup_is_not_added_to_the_ADS(self):
        _ = WorkspaceGroup()

        self.assertEqual(len(AnalysisDataService.getObjectNames()), 0)

    class _HoldWsGroup:
        def __init__(self):
            self.group = WorkspaceGroup()

        def __enter__(self):
            return self.group

        def __exit__(self, exc_type, exc_val, exc_tb):
            del self.group  # Make our intention explicit for future readers

    def test_that_get_item_scope_works_not_in_ads(self):
        with self._HoldWsGroup() as group:
            # Note we want to create this inline so our ref is through getItem
            group.addWorkspace(WorkspaceFactory.create("Workspace2D", 1, 1, 1))
            ws = group.getItem(0)
            self.assertFalse(ws.name())  # Not in ADS so should be no name
            self.assertEqual(len(AnalysisDataService.getObjectNames()), 0)
        # Now the group should be deleted, this should continue to work now
        self.assertFalse(ws.name())

    def test_that_instantiated_WorkspaceGroup_can_be_added_to_the_ADS(self):
        ws_group = WorkspaceGroup()
        mtd.add("group1", ws_group)

        self.assertEqual(AnalysisDataService.getObjectNames(), ["group1"])
        self.assertIsInstance(mtd["group1"], WorkspaceGroup)

    def test_that_can_add_workspaces_to_WorkspaceGroup_when_in_ADS(self):
        self.create_matrix_workspace_in_ADS("ws1")
        self.create_matrix_workspace_in_ADS("ws2")

        ws_group = WorkspaceGroup()
        mtd.add("group1", ws_group)

        ws_group.add("ws1")
        ws_group.add("ws2")

        self.assertTrue("ws1" in mtd["group1"])
        self.assertTrue("ws2" in mtd["group1"])

    def test_that_can_add_workspaces_to_WorkspaceGroup_when_not_in_ADS(self):
        ws1 = WorkspaceFactory.create("Workspace2D", 2, 2, 2)
        ws2 = WorkspaceFactory.create("Workspace2D", 2, 2, 2)

        ws_group = WorkspaceGroup()

        ws_group.addWorkspace(ws1)
        ws_group.addWorkspace(ws2)

        self.assertEqual(ws_group.size(), 2)

    def test_that_GroupWorkspaces_algorithm_creates_group_of_the_correct_size(self):
        group = self.create_group_via_GroupWorkspace_algorithm()

        self.assertEqual(type(group), WorkspaceGroup)
        self.assertEqual(2, group.size())
        self.assertEqual(2, group.getNumberOfEntries())

    def test_that_python__len__method_works_correctly_on_group(self):
        group = self.create_group_via_GroupWorkspace_algorithm()

        self.assertEqual(len(group), group.getNumberOfEntries())

    def test_that_getName_method_returns_correct_names(self):
        group = self.create_group_via_GroupWorkspace_algorithm()
        names = group.getNames()

        self.assertCountEqual(names, ["First", "Second"])

    def test_that_a_group_is_invalidated_if_ADS_is_cleared_and_RuntimeError_raised(self):
        group = self.create_group_via_GroupWorkspace_algorithm()

        mtd.clear()

        with self.assertRaises(RuntimeError):
            group.getNames()

    def test_that_IndexError_raised_when_attemtping_to_access_an_index_which_doesnt_exist(self):
        group = self.create_group_via_GroupWorkspace_algorithm()

        with self.assertRaises(IndexError):
            group.__getitem__(2)

    def test_that_indexing_the_group_returns_objects_of_the_correct_type(self):
        group = self.create_group_via_GroupWorkspace_algorithm()

        for i in range(2):
            member = group[i]
            self.assertTrue(isinstance(member, MatrixWorkspace))

    def test_that_sortByName_sorts_names_alphabetically_when_using_getNames(self):
        group = self.create_group_via_GroupWorkspace_algorithm()
        group.sortByName()
        names = group.getNames()

        self.assertEqual(names[0], "First")
        self.assertEqual(names[1], "Second")

    def test_SimpleAlgorithm_Accepts_Group_Handle(self):
        from mantid.simpleapi import Scale

        self.create_matrix_workspace_in_ADS("First")
        self.create_matrix_workspace_in_ADS("Second")
        run_algorithm("GroupWorkspaces", InputWorkspaces="First,Second", OutputWorkspace="grouped")
        group = mtd["grouped"]

        try:
            w = Scale(group, 1.5)
            mtd.remove(str(w))
        except Exception as exc:
            self.fail("Algorithm raised an exception with input as WorkspaceGroup: '" + str(exc) + "'")
        mtd.remove(str(group))

    def test_complex_binary_operations_with_group_do_not_leave_temporary_workspaces_in_ADS(self):
        run_algorithm(
            "CreateWorkspace", OutputWorkspace="grouped_1", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF"
        )
        run_algorithm(
            "CreateWorkspace", OutputWorkspace="grouped_2", DataX=[1.0, 2.0, 3.0], DataY=[2.0, 3.0], DataE=[2.0, 3.0], UnitX="TOF"
        )
        run_algorithm("GroupWorkspaces", InputWorkspaces="grouped_1,grouped_2", OutputWorkspace="grouped")

        self.assertTrue("w1" in mtd)
        self.assertTrue("grouped" in mtd)
        self.assertTrue("grouped_1" in mtd)
        self.assertTrue("grouped_2" in mtd)
        self.assertTrue("__python_op_tmp0" not in mtd)
        self.assertTrue("__python_op_tmp0_1" not in mtd)
        self.assertTrue("__python_op_tmp0_2" not in mtd)

        mtd.remove("w1")
        mtd.remove("grouped")
        mtd.remove("grouped_1")
        mtd.remove("grouped_2")

    def test_negative_indices_return_correct_ws_from_group(self):
        group = self.create_group_via_GroupWorkspace_algorithm()
        self.assertEqual(group[-1].name(), "Second")
        self.assertEqual(group[-2].name(), "First")

    def test_out_of_bounds_negative_index_returns_IndexError(self):
        group = self.create_group_via_GroupWorkspace_algorithm()
        with self.assertRaises(IndexError):
            group[-400]

    def test_getItem_negative_int_index_values(self):
        group = self.create_group_via_GroupWorkspace_algorithm()
        self.assertEqual(group.getItem(-1).name(), "Second")
        self.assertEqual(group.getItem(-2).name(), "First")
        with self.assertRaises(IndexError):
            group.getItem(-400)

    def test_isGroup(self):
        group = self.create_group_via_GroupWorkspace_algorithm()
        self.assertEqual(group.isGroup(), True)


if __name__ == "__main__":
    unittest.main()
