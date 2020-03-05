# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

import mantid.simpleapi as simpleapi
from mantid.api import WorkspaceGroup

from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


def create_simple_workspace(data_x, data_y):
    alg = simpleapi.AlgorithmManager.create("CreateWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setLogging(False)
    alg.setProperty("dataX", data_x)
    alg.setProperty("dataY", data_y)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def create_simple_table_workspace():
    alg = simpleapi.AlgorithmManager.create("CreateEmptyTableWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setLogging(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.execute()
    table = alg.getProperty("OutputWorkspace").value
    table.addColumn("int", "col1", 0)
    table.addColumn("int", "col2", 0)
    [table.addRow([i + 1, 2 * i]) for i in range(4)]
    return table


class MuonWorkspaceAddDirectoryTest(unittest.TestCase):
    """
    Test the functionality surrounding adding "directory structures" to the ADS, in other words
    adding nested structures of WorkspaceGroups to help structure the data.
    """

    def setUp(self):
        assert simpleapi.mtd.size() == 0

    def tearDown(self):
        # clear the ADS
        simpleapi.mtd.clear()

    def assert_group_workspace_exists(self, name):
        self.assertTrue(simpleapi.mtd.doesExist(name))
        self.assertEqual(type(simpleapi.mtd.retrieve(name)), WorkspaceGroup)

    def assert_group1_is_inside_group2(self, group1_name, group2_name):
        group2 = simpleapi.mtd.retrieve(group2_name)
        self.assertIn(group1_name, group2.getNames())

    def assert_workspace_in_group(self, workspace_name, group_name):
        group = simpleapi.mtd.retrieve(group_name)
        self.assertIn(workspace_name, group.getNames())

    # ----------------------------------------------------------------------------------------------
    # Test directory structure functionality in MuonWorkspaceWrapper
    # ----------------------------------------------------------------------------------------------

    def test_that_if_workspace_exists_with_same_name_as_group_then_it_is_replaced(self):
        group = simpleapi.CreateWorkspace([1, 2, 3, 4], [10, 10, 10, 10])

        workspace_handle = MuonWorkspaceWrapper('group')
        workspace_handle.show("group/ws1")

        self.assert_group_workspace_exists("group")

    def test_that_workspace_added_correctly_for_single_nested_structure(self):
        workspace = create_simple_workspace(data_x=[1, 2, 3, 4], data_y=[10, 10, 10, 10])
        workspace_handle = MuonWorkspaceWrapper(workspace=workspace)

        workspace_handle.show("group1/ws1")

        self.assert_group_workspace_exists("group1")
        self.assert_workspace_in_group("ws1", "group1")

    def test_that_workspaces_in_existing_folders_are_not_moved_by_directory_manipulation(self):
        workspace1 = create_simple_workspace(data_x=[1, 2, 3, 4], data_y=[10, 10, 10, 10])
        workspace2 = create_simple_workspace(data_x=[1, 2, 3, 4], data_y=[10, 10, 10, 10])
        workspace_handle1 = MuonWorkspaceWrapper(workspace=workspace1)
        workspace_handle2 = MuonWorkspaceWrapper(workspace=workspace2)

        workspace_handle1.show("group2/ws1")
        workspace_handle2.show("group3/ws2")

        self.assert_group_workspace_exists("group2")
        self.assert_group_workspace_exists("group3")
        self.assert_workspace_in_group("ws1", "group2")
        self.assert_workspace_in_group("ws2", "group3")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
