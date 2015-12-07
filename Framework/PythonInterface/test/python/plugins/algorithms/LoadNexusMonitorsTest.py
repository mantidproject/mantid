import unittest
from mantid.simpleapi import *
from mantid.api import *
from mantid.dataobjects import Workspace2D

class LoadNexusMonitorsTest(unittest.TestCase):
    def test_multiperiod_output(self):
        """
        Test for correct output when loading a multiperiod nexus workspace.
        """
        # OutputWorkspace name inferred from LHS
        group_a = LoadNexusMonitors(Filename="LARMOR00003368.nxs")

        # Output should be a group workspace of 4 workspaces with given names
        self.assertEqual(type(group_a), WorkspaceGroup)
        self.assertEqual(len(group_a), 4)
        self.assertEqual(group_a[0].name(), "group_a_1")
        self.assertEqual(group_a[1].name(), "group_a_2")
        self.assertEqual(group_a[2].name(), "group_a_3")
        self.assertEqual(group_a[3].name(), "group_a_4")

        # OutputWorkspace specified explicitly
        LoadNexusMonitors(Filename="LARMOR00003368.nxs", OutputWorkspace="group_b")
        group_b = mtd["group_b"]

        # Output should still work the same way
        self.assertEqual(type(group_b), WorkspaceGroup)
        self.assertEqual(len(group_b), 4)
        self.assertEqual(group_b[0].name(), "group_b_1")
        self.assertEqual(group_b[1].name(), "group_b_2")
        self.assertEqual(group_b[2].name(), "group_b_3")
        self.assertEqual(group_b[3].name(), "group_b_4")

    def test_multiperiod_old_behaviour(self):
        """
        Initial version of algorithm had a bug that produced incorrect output.
        """
        # OutputWorkspace name inferred from LHS
        group_a = LoadNexusMonitors(Filename="LARMOR00003368.nxs", Version=1)

        # Bug caused a tuple of the group workspace and its children to be returned
        self.assertEqual(type(group_a), tuple)
        self.assertEqual(len(group_a), 5)

        self.assertEqual(type(group_a[0]), WorkspaceGroup)
        self.assertEqual(type(group_a[1]), Workspace2D)
        self.assertEqual(type(group_a[2]), Workspace2D)
        self.assertEqual(type(group_a[3]), Workspace2D)
        self.assertEqual(type(group_a[4]), Workspace2D)

        # OutputWorkspace name was also missing from child workspace names
        self.assertEqual(group_a[0][0].name(), "_1")
        self.assertEqual(group_a[0][1].name(), "_2")
        self.assertEqual(group_a[0][2].name(), "_3")
        self.assertEqual(group_a[0][3].name(), "_4")

        # OutputWorkspace specified explicitly
        LoadNexusMonitors(Filename="LARMOR00003368.nxs", OutputWorkspace="group_b", Version=1)
        group_b = mtd["group_b"]

        # Since group_b is created by grabbing the group workspace from ADS,
        # you get a more reasonable result this way
        self.assertEqual(type(group_b), WorkspaceGroup)
        self.assertEqual(len(group_b), 4)

        # But the names are still wrong
        self.assertEqual(group_b[0].name(), "_1")
        self.assertEqual(group_b[1].name(), "_2")
        self.assertEqual(group_b[2].name(), "_3")
        self.assertEqual(group_b[3].name(), "_4")

if __name__ == '__main__':
    unittest.main()
