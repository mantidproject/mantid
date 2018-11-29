# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

import unittest
from itertools import permutations

from mantidqt.widgets.tableworkspacedisplay.error_column import ErrorColumn
from mantidqt.widgets.tableworkspacedisplay.marked_columns import MarkedColumns


class ReferenceHolder:
    def __init__(self, test_func, test_list):
        self.func = test_func
        self.list = test_list


class MarkedColumnsTest(unittest.TestCase):

    def test_add_x(self):
        mc = MarkedColumns()
        self.execute_add(mc.add_x, mc.as_x)

    def test_add_y(self):
        mc = MarkedColumns()
        self.execute_add(mc.add_y, mc.as_y)

    def execute_add(self, func_to_add, list_to_check):
        func_to_add(2)
        self.assertEqual(1, len(list_to_check))
        func_to_add(3)
        self.assertEqual(2, len(list_to_check))
        func_to_add(4000000)
        self.assertEqual(3, len(list_to_check))

    def test_add_y_err(self):
        """
        Test adding YErr columns that do not overlap in any way
        """
        mc = MarkedColumns()
        ec = ErrorColumn(2, 4, 0)
        mc.add_y_err(ec)
        self.assertEqual(1, len(mc.as_y_err))
        ec = ErrorColumn(3, 5, 0)
        mc.add_y_err(ec)
        self.assertEqual(2, len(mc.as_y_err))
        ec = ErrorColumn(1, 6, 0)
        mc.add_y_err(ec)
        self.assertEqual(3, len(mc.as_y_err))

    def test_add_x_duplicate_column(self):
        mc = MarkedColumns()
        self.execute_add_duplicate_column(mc.add_x, mc.as_x)

    def test_add_y_duplicate_column(self):
        mc = MarkedColumns()
        self.execute_add_duplicate_column(mc.add_y, mc.as_y)

    def execute_add_duplicate_column(self, func_to_add, list_to_check):
        func_to_add(2)
        self.assertEqual(1, len(list_to_check))
        func_to_add(2)
        self.assertEqual(1, len(list_to_check))
        func_to_add(55)
        self.assertEqual(2, len(list_to_check))
        func_to_add(55)
        self.assertEqual(2, len(list_to_check))

    def test_add_y_err_duplicate_column(self):
        mc = MarkedColumns()
        ec = ErrorColumn(2, 4, 0)

        mc.add_y_err(ec)
        self.assertEqual(1, len(mc.as_y_err))
        mc.add_y_err(ec)
        self.assertEqual(1, len(mc.as_y_err))

        ec2 = ErrorColumn(3, 5, 0)
        mc.add_y_err(ec2)
        self.assertEqual(2, len(mc.as_y_err))
        mc.add_y_err(ec2)
        self.assertEqual(2, len(mc.as_y_err))

    def test_add_already_marked(self):
        mc = MarkedColumns()

        relevant_funcs = [ReferenceHolder(mc.add_x, mc.as_x),
                          ReferenceHolder(mc.add_y, mc.as_y)]
        all_combinations = permutations(relevant_funcs, 2)

        for combination in all_combinations:
            self.execute_add_already_marked(*combination)

    def execute_add_already_marked(self, first, two):
        """
        If trying to mark a column that is already marked -> all other markings must be removed
        :type first: ReferenceHolder
        :type two: ReferenceHolder
        :return:
        """

        # add column in first
        first.func(33)
        self.assertEqual(1, len(first.list))

        # add the same column in the second
        two.func(33)

        # it should have been removed from the first and only present in the second
        self.assertEqual(0, len(first.list))
        self.assertEqual(1, len(two.list))

    def test_add_y_err_duplicate_column_same_source_column(self):
        """
        Test for adding a new YErr column with the same source column
        -> The new YErr must replace the old one
        """
        mc = MarkedColumns()
        ec = ErrorColumn(source_column=2, error_for_column=4, label_index=0)
        mc.add_y_err(ec)
        self.assertEqual(1, len(mc.as_y_err))
        self.assertEqual(2, mc.as_y_err[0].source_column)
        self.assertEqual(4, mc.as_y_err[0].error_for_column)

        # different source column but contains error for the same column
        # adding this one should replace the first one
        ec2 = ErrorColumn(source_column=2, error_for_column=5, label_index=0)
        mc.add_y_err(ec2)
        self.assertEqual(1, len(mc.as_y_err))
        self.assertEqual(2, mc.as_y_err[0].source_column)
        self.assertEqual(5, mc.as_y_err[0].error_for_column)

    def test_add_y_err_duplicate_column_different_reference_col(self):
        """
        Test for adding a new YErr column with a _different_ source column but same reference column
        -> The new YErr must replace the old one
        """
        mc = MarkedColumns()
        ec = ErrorColumn(source_column=2, error_for_column=4, label_index=0)
        mc.add_y_err(ec)
        self.assertEqual(1, len(mc.as_y_err))
        self.assertEqual(2, mc.as_y_err[0].source_column)
        self.assertEqual(4, mc.as_y_err[0].error_for_column)

        # different source column but contains error for the same column
        # adding this one should replace the first one
        ec2 = ErrorColumn(source_column=3, error_for_column=4, label_index=0)
        mc.add_y_err(ec2)
        self.assertEqual(1, len(mc.as_y_err))
        self.assertEqual(3, mc.as_y_err[0].source_column)
        self.assertEqual(4, mc.as_y_err[0].error_for_column)

    def test_changing_y_to_x_removes_associated_yerr_columns(self):
        """
        Test to check if a first column is marked as Y, a second column YErr is associated with it, but then
        the first one is changed to X - the YErr mark should be removed
        """
        mc = MarkedColumns()
        mc.add_y(4)
        ec = ErrorColumn(source_column=2, error_for_column=4, label_index=0)
        mc.add_y_err(ec)

        # check that we have both a Y col and an associated YErr
        self.assertEqual(1, len(mc.as_y))
        self.assertEqual(1, len(mc.as_y_err))

        mc.add_x(4)
        # changing the column to X should have removed it from Y and Yerr
        self.assertEqual(1, len(mc.as_x))
        self.assertEqual(0, len(mc.as_y))
        self.assertEqual(0, len(mc.as_y_err))

    def test_changing_y_to_none_removes_associated_yerr_columns(self):
        """
        Test to check if a first column is marked as Y, a second column YErr is associated with it, but then
        the first one is changed to X - the YErr mark should be removed
        """
        mc = MarkedColumns()
        mc.add_y(4)
        ec = ErrorColumn(source_column=2, error_for_column=4, label_index=0)
        mc.add_y_err(ec)

        # check that we have both a Y col and an associated YErr
        self.assertEqual(1, len(mc.as_y))
        self.assertEqual(1, len(mc.as_y_err))

        mc.remove_column(4)
        # changing the column to NONE should have removed it from X, Y and YErr
        self.assertEqual(0, len(mc.as_x))
        self.assertEqual(0, len(mc.as_y))
        self.assertEqual(0, len(mc.as_y_err))

    def test_remove_column(self):
        mc = MarkedColumns()
        mc.add_y(4)
        mc.add_x(3)
        ec = ErrorColumn(source_column=2, error_for_column=6, label_index=0)
        mc.add_y_err(ec)

        self.assertEqual(1, len(mc.as_x))
        self.assertEqual(1, len(mc.as_y))
        self.assertEqual(1, len(mc.as_y_err))

        mc.remove_column(4)
        self.assertEqual(0, len(mc.as_y))
        self.assertEqual(1, len(mc.as_y_err))
        self.assertEqual(1, len(mc.as_x))

        mc.remove_column(3)
        self.assertEqual(0, len(mc.as_x))
        self.assertEqual(0, len(mc.as_y))
        self.assertEqual(1, len(mc.as_y_err))

        mc.remove_column(2)
        self.assertEqual(0, len(mc.as_x))
        self.assertEqual(0, len(mc.as_y))
        self.assertEqual(0, len(mc.as_y_err))

    def test_build_labels(self):
        # TODO test this edge case: mark all columns Y, remove one that is not the last one!
        # TODO test: mark 3 columns as Y, set the first one to YErr it should have label YErr->Y1
        # TODO test: mark 3 columns as Y, set the middle one to YErr it should have label YErr->Y1
        self.skipTest("Not Impl")

    def test_find_yerr(self):
        self.skipTest("Not Impl")
