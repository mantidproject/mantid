# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from mantid.api import *


class StatisticsOfTableWorkspaceTest(unittest.TestCase):
    def test_basic(self):
        """
        Tests for a table workspace containing all floats.
        """

        table = CreateEmptyTableWorkspace(OutputWorkspace="__StatisticsOfTableWorkspace_Test")

        table.addColumn("int", "a")
        table.addColumn("float", "b")

        table.addRow([1, 3.2])
        table.addRow([2, 3.4])
        table.addRow([3, 3.6])
        table.addRow([4, 3.8])

        stats = StatisticsOfTableWorkspace(InputWorkspace=table)

        self.assertEqual(stats.rowCount(), 5)
        self.assertEqual(stats.columnCount(), 3)

        stat_col = stats.column("Statistic")

        self.assertAlmostEqual(stats.column("a")[stat_col.index("StandardDev")], 1.11803400517)
        self.assertAlmostEqual(stats.column("a")[stat_col.index("Minimum")], 1.0)
        self.assertAlmostEqual(stats.column("a")[stat_col.index("Median")], 2.5)
        self.assertAlmostEqual(stats.column("a")[stat_col.index("Maximum")], 4.0)
        self.assertAlmostEqual(stats.column("a")[stat_col.index("Mean")], 2.5)

    def test_invalid_types(self):
        """
        Tests to ensure that columns of an invalid type are skipped.
        """

        table = CreateEmptyTableWorkspace(OutputWorkspace="__StatisticsOfTableWorkspace_Test")

        table.addColumn("int", "a")
        table.addColumn("str", "b")

        table.addRow([1, "b1"])
        table.addRow([2, "b2"])
        table.addRow([3, "b3"])
        table.addRow([4, "b4"])

        stats = StatisticsOfTableWorkspace(InputWorkspace=table)

        self.assertEqual(stats.rowCount(), 5)
        self.assertEqual(stats.columnCount(), 2)


if __name__ == "__main__":
    unittest.main()
