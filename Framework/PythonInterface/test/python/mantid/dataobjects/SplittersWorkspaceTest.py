# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-public-methods
import unittest

from mantid.api import WorkspaceFactory, ISplittersWorkspace, ITableWorkspace
from mantid.dataobjects import SplittersWorkspace, TableWorkspace


class SplittersWorkspaceTest(unittest.TestCase):
    def test_creation(self):
        splitter_ws = SplittersWorkspace()
        self.assertEqual(splitter_ws.id(), "SplittersWorkspace")
        self.assertIsInstance(splitter_ws, ISplittersWorkspace)
        self.assertIsInstance(splitter_ws, SplittersWorkspace)
        self.assertIsInstance(splitter_ws, ITableWorkspace)
        self.assertIsInstance(splitter_ws, TableWorkspace)

    def test_WorkspaceFactory_creation(self):
        splitter_ws = WorkspaceFactory.createTable("SplittersWorkspace")
        self.assertEqual(splitter_ws.id(), "SplittersWorkspace")
        self.assertIsInstance(splitter_ws, ISplittersWorkspace)
        self.assertIsInstance(splitter_ws, SplittersWorkspace)
        self.assertIsInstance(splitter_ws, ITableWorkspace)
        self.assertIsInstance(splitter_ws, TableWorkspace)

    def test_default_columns(self):
        splitter_ws = SplittersWorkspace()
        self.assertEqual(splitter_ws.getColumnNames(), ["start", "stop", "workspacegroup"])
        self.assertEqual(splitter_ws.columnTypes(), ["long64", "long64", "int"])

    def test_getNumberSplitters(self):
        splitter_ws = SplittersWorkspace()
        self.assertEqual(splitter_ws.getNumberSplitters(), 0)
        splitter_ws.addRow([0, 10, 1])
        splitter_ws.addRow([11, 20, 2])
        self.assertEqual(splitter_ws.getNumberSplitters(), 2)


if __name__ == "__main__":
    unittest.main()
