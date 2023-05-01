# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-public-methods
import unittest

from mantid.simpleapi import Load
from mantid.dataobjects import GroupingWorkspace


class GroupingWorkspaceTest(unittest.TestCase):
    def setUp(self):
        self.groupingWorkspace = GroupingWorkspace()

    def test_get_total_groups(self):
        self.assertEqual(0, self.groupingWorkspace.getTotalGroups())

    def test_get_group_IDs(self):
        ids = self.groupingWorkspace.getGroupIDs()
        self.assertEqual(0, len(ids))

    def test_get_group_spetra_IDs(self):
        self.assertEqual(0, len(self.groupingWorkspace.getGroupSpetraIDs(-1)))  # default sws2d values are all 0


if __name__ == "__main__":
    unittest.main()
