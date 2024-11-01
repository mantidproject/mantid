# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from plugins.algorithms.PeakMatching import PeakMatching as _PeakMatching
import unittest
from unittest import mock
from mantid.simpleapi import DeleteWorkspace, CreateEmptyTableWorkspace, PeakMatching, CreateWorkspace
from mantid.api import mtd


class PeakMatchingTest(unittest.TestCase):
    def setUp(self):
        self.peak_data = {
            "Primary energy": {"Ag": {"K(4->1)": 3177.7, "L(4->2)": 900.7, "M(4->3)": 304.7, "6->5": 141.0}},
            "Secondary energy": {"Ag": {"K(2->1)": 3140.6, "L(8->2)": 1347.8, "M(10->3)": 567.0, "8->6": 122.2}},
            "All energies": {
                "Ag": {
                    "K(4->1)": 3177.7,
                    "L(4->2)": 900.7,
                    "M(4->3)": 304.7,
                    "6->5": 141.0,
                    "K(2->1)": 3140.6,
                    "L(8->2)": 1347.8,
                    "M(10->3)": 567.0,
                    "8->6": 122.2,
                }
            },
        }

        self.input_peaks = [(900, 0.8), (306, 0.8), (567, 0.8), (3, 0.8)]
        self.input_table = CreateEmptyTableWorkspace(OutputWorkSpace="input")
        self.input_table.addColumn("double", "centre")
        self.input_table.addColumn("double", "sigma")
        for row in self.input_peaks:
            self.input_table.addRow(row)
        self.alg = _PeakMatching()

    def tearDown(self):
        self.delete_if_present("input")
        self.delete_if_present("primary_matches")
        self.delete_if_present("secondary_matches")
        self.delete_if_present("all_matches")
        self.delete_if_present("all_matches_sorted_by_energy")
        self.delete_if_present("element_likelihood")
        self.delete_if_present("Test")
        self.alg = None

    def assertPeaksMatch(self, table, peaks):
        assert len(peaks) == table.columnCount()
        for column in peaks:
            column_data = table.column(column)
            assert len(peaks[column]) == len(column_data)
            for i in range(len(peaks[column])):
                if isinstance(peaks[column][i], float):
                    self.assertAlmostEqual(peaks[column][i], column_data[i], delta=0.01)
                else:
                    self.assertEqual(peaks[column][i], column_data[i])

    @staticmethod
    def delete_if_present(workspace):
        if workspace in mtd:
            DeleteWorkspace(workspace)

    def test_get_input_peaks(self):
        table_data = self.alg.get_input_peaks(self.input_table.name(), "centre", "sigma")
        self.assertEqual(table_data, self.input_peaks)
        self.delete_if_present("Test")

    @mock.patch("plugins.algorithms.PeakMatching.PeakMatching.get_default_peak_data")
    def test_process_peak_data(self, mock_get_default_peak_data):
        """
        This test checks if process_peak_data method returns the correct dictionary which is self.peak_data,
        the process_peak_data calls the get_default_peak_data method when an empty string is passed to it ,
        the method is mocked as it needs to open a file and has been mocked to return a snippet of
        the data in the default json file.
        """
        mock_get_default_peak_data.return_value = dict(
            {
                "Ag": {
                    "Z": 47,
                    "A": 107.87,
                    "Primary": {"K(4->1)": 3177.7, "L(4->2)": 900.7, "M(4->3)": 304.7, "6->5": 141},
                    "Secondary": {"K(2->1)": 3140.6, "L(8->2)": 1347.8, "M(10->3)": 567, "8->6": 122.2},
                    "Gammas": {"72Ge(n,n')72Ge": 691, "73Ge(n,g)74Ge": None, "74Ge(n,n')74Ge": 595.7},
                }
            }
        )
        data = self.alg.process_peak_data("")

        self.assertEqual(self.peak_data, data)

    def test_get_matches(self):
        all_data = self.alg.get_matches(self.peak_data, self.input_peaks)

        primary_matches = [
            {"energy": 900.7, "peak_centre": 900, "error": 0.8, "element": "Ag", "diff": 0.7, "transition": "L(4->2)", "Rating": 3},
            {"energy": 304.7, "peak_centre": 306, "error": 1.6, "element": "Ag", "diff": 1.3, "transition": "M(4->3)", "Rating": 2},
        ]

        all_matches = [
            {"energy": 567, "peak_centre": 567, "error": 0, "element": "Ag", "diff": 0, "transition": "M(10->3)", "Rating": 4},
            {"energy": 900.7, "peak_centre": 900, "error": 0.8, "element": "Ag", "diff": 0.7, "transition": "L(4->2)", "Rating": 3},
            {"energy": 304.7, "peak_centre": 306, "error": 1.6, "element": "Ag", "diff": 1.3, "transition": "M(4->3)", "Rating": 2},
        ]

        secondary_matches = [
            {"energy": 567, "peak_centre": 567, "error": 0, "element": "Ag", "diff": 0, "transition": "M(10->3)", "Rating": 4}
        ]

        all_matches = [primary_matches, secondary_matches, all_matches]
        for i, data in enumerate(all_data):
            matches = all_matches[i]
            if len(data) != len(matches):
                raise AssertionError
            for j, row in enumerate(data):
                for column, value in row.items():
                    if isinstance(value, int) or isinstance(value, float):
                        self.assertAlmostEqual(matches[j][column], value)
                    else:
                        self.assertEqual(matches[j][column], value)

    @mock.patch("plugins.algorithms.PeakMatching.PeakMatching.make_peak_table")
    @mock.patch("plugins.algorithms.PeakMatching.PeakMatching.make_count_table")
    @mock.patch("plugins.algorithms.PeakMatching.PeakMatching.setProperty")
    @mock.patch("plugins.algorithms.PeakMatching.PeakMatching.setPropertyValue")
    def test_output_data(self, mock_set_value, mock_set_property, mock_make_count_table, mock_make_peak_table):
        self.alg.output_data(1, 2, 3, ["prim", "secon", "all", "sort", "count"] * 5)
        call = [
            mock.call("prim", 1),
            mock.call("secon", 2),
            mock.call("all", 3),
            mock.call("sort", 3, True, "energy"),
            mock.call("count", 3),
        ]
        mock_make_peak_table.assert_has_calls(call[:-1])
        mock_make_count_table.assert_has_calls(call[-1:])
        self.assertEqual(mock_make_peak_table.call_count, 4)
        self.assertEqual(mock_make_count_table.call_count, 1)
        self.assertEqual(mock_set_value.call_count, 5)
        self.assertEqual(mock_set_property.call_count, 5)

    @mock.patch("plugins.algorithms.PeakMatching.PeakMatching.get_default_peak_data")
    def test_algorithm_with_valid_inputs(self, mock_get_default_peak_data):
        mock_get_default_peak_data.return_value = dict(
            {
                "Ag": {
                    "Z": 47,
                    "A": 107.87,
                    "Primary": {"K(4->1)": 3177.7, "L(4->2)": 900.7, "M(4->3)": 304.7, "6->5": 141},
                    "Secondary": {"K(2->1)": 3140.6, "L(8->2)": 1347.8, "M(10->3)": 567, "8->6": 122.2},
                    "Gammas": {"72Ge(n,n')72Ge": 691, "73Ge(n,g)74Ge": None, "74Ge(n,n')74Ge": 595.7},
                }
            }
        )

        PeakMatching(PeakTable=self.input_table)
        prim = mtd["primary_matches"]
        secon = mtd["secondary_matches"]
        all_matches = mtd["all_matches"]
        sort = mtd["all_matches_sorted_by_energy"]
        count = mtd["element_likelihood"]

        correct_prim = {
            "Peak centre": [900.0, 306.0],
            "Database Energy": [900.7, 304.7],
            "Element": ["Ag", "Ag"],
            "Transition": ["L(4->2)", "M(4->3)"],
            "Error": [0.8, 1.6],
            "Difference": [0.7, 1.3],
        }

        correct_secon = {
            "Peak centre": [567.0],
            "Database Energy": [567.0],
            "Element": ["Ag"],
            "Transition": ["M(10->3)"],
            "Error": [0.0],
            "Difference": [0.0],
        }

        correct_all = {
            "Peak centre": [567.0, 900.0, 306.0],
            "Database Energy": [567.0, 900.7, 304.7],
            "Element": ["Ag", "Ag", "Ag"],
            "Transition": ["M(10->3)", "L(4->2)", "M(4->3)"],
            "Error": [0.0, 0.8, 1.6],
            "Difference": [0.0, 0.7, 1.3],
        }

        correct_sort = {
            "Peak centre": [306.0, 567.0, 900.0],
            "Database Energy": [304.7, 567.0, 900.7],
            "Element": ["Ag", "Ag", "Ag"],
            "Transition": ["M(4->3)", "M(10->3)", "L(4->2)"],
            "Error": [1.6, 0.0, 0.8],
            "Difference": [1.3, 0.0, 0.7],
        }

        correct_count = {"Element": ["Ag"], "Likelihood(arbitrary units)": [9]}

        self.assertPeaksMatch(prim, correct_prim)
        self.assertPeaksMatch(secon, correct_secon)
        self.assertPeaksMatch(all_matches, correct_all)
        self.assertPeaksMatch(sort, correct_sort)
        self.assertPeaksMatch(count, correct_count)

        self.delete_if_present("primary-matches")
        self.delete_if_present("secondary_matches")
        self.delete_if_present("all_matches")
        self.delete_if_present("all_matches_sorted_by_energy")
        self.delete_if_present("element_likelihood")

    def test_algorithm_with_invalid_arguments(self):
        test_workspace = CreateWorkspace(DataX=[1], DataY=[1], OutputWorkspace="Test")

        with self.assertRaises(ValueError):
            PeakMatching(PeakTable="Spam")

        with self.assertRaises(ValueError):
            PeakMatching(PeakTable=test_workspace)

        with self.assertRaises(TypeError):
            PeakMatching(PeakTable=self.input_table, PeakCentreColumn=1)

        with self.assertRaises(TypeError):
            PeakMatching(PeakTable=self.input_table, SigmaColumn=1.7)

    @mock.patch("plugins.algorithms.PeakMatching.PeakMatching.get_default_peak_data")
    def test_algorithm_correctly_names_tables(self, mock_get_default_peak_data):
        mock_get_default_peak_data.return_value = dict(
            {
                "Ag": {
                    "Z": 47,
                    "A": 107.87,
                    "Primary": {"K(4->1)": 3177.7, "L(4->2)": 900.7, "M(4->3)": 304.7, "6->5": 141},
                    "Secondary": {"K(2->1)": 3140.6, "L(8->2)": 1347.8, "M(10->3)": 567, "8->6": 122.2},
                    "Gammas": {"72Ge(n,n')72Ge": 691, "73Ge(n,g)74Ge": None, "74Ge(n,n')74Ge": 595.7},
                }
            }
        )

        PeakMatching(
            PeakTable=self.input_table,
            AllPeaks="rename_all",
            PrimaryPeaks="rename_prim",
            SecondaryPeaks="rename_secon",
            SortedByEnergy="rename_sort",
            ElementLikelihood="rename_count",
        )
        prim = mtd["rename_prim"]
        print(prim.toDict())
        secon = mtd["rename_secon"]
        all_matches = mtd["rename_all"]
        sort = mtd["rename_sort"]
        count = mtd["rename_count"]

        correct_prim = {
            "Peak centre": [900.0, 306.0],
            "Database Energy": [900.7, 304.7],
            "Element": ["Ag", "Ag"],
            "Transition": ["L(4->2)", "M(4->3)"],
            "Error": [0.8, 1.6],
            "Difference": [0.7, 1.3],
        }

        correct_secon = {
            "Peak centre": [567.0],
            "Database Energy": [567.0],
            "Element": ["Ag"],
            "Transition": ["M(10->3)"],
            "Error": [0.0],
            "Difference": [0.0],
        }

        correct_all = {
            "Peak centre": [567.0, 900.0, 306.0],
            "Database Energy": [567.0, 900.7, 304.7],
            "Element": ["Ag", "Ag", "Ag"],
            "Transition": ["M(10->3)", "L(4->2)", "M(4->3)"],
            "Error": [0.0, 0.8, 1.6],
            "Difference": [0.0, 0.7, 1.3],
        }

        correct_sort = {
            "Peak centre": [306.0, 567.0, 900.0],
            "Database Energy": [304.7, 567.0, 900.7],
            "Element": ["Ag", "Ag", "Ag"],
            "Transition": ["M(4->3)", "M(10->3)", "L(4->2)"],
            "Error": [1.6, 0.0, 0.8],
            "Difference": [1.3, 0.0, 0.7],
        }

        correct_count = {"Element": ["Ag"], "Likelihood(arbitrary units)": [9]}

        self.assertPeaksMatch(prim, correct_prim)
        self.assertPeaksMatch(secon, correct_secon)
        self.assertPeaksMatch(all_matches, correct_all)
        self.assertPeaksMatch(sort, correct_sort)
        self.assertPeaksMatch(count, correct_count)

        self.delete_if_present("rename_all")
        self.delete_if_present("rename_prim")
        self.delete_if_present("rename_secon")
        self.delete_if_present("rename_sort")
        self.delete_if_present("rename_count")


if __name__ == "__main__":
    unittest.main()
