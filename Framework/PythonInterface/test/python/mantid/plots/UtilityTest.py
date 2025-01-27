# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import matplotlib.pyplot as plt

from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateSampleWorkspace
from mantid.plots.utility import col_num, legend_set_draggable, row_num
from matplotlib.legend import Legend
from unittest.mock import create_autospec


class UtilityTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workspace = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction="name=ExpDecay,Lifetime = 20,Height = 200;name=Gaussian,PeakCentre=50, Height=10, Sigma=3",
            XMax=100,
            BinWidth=2,
            OutputWorkspace="__temp",
        )

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.fig, self.axes = plt.subplots(ncols=2, nrows=1, subplot_kw={"projection": "mantid"})

        self.axes[0].plot(self.workspace, specNum=1)
        self.axes[1].errorbar(self.workspace, specNum=1, capsize=2.0)

        # Add an inset axes, which is a good edge case to test
        inset = self.fig.add_axes([0.77, 0.70, 0.18, 0.18], projection="mantid")
        inset.plot(self.workspace, specNum=5, color="blue", label="Log Peak", marker=".")

    def tearDown(self):
        plt.close("all")
        self.fig, self.axes = None, None

    def test_legend_set_draggable(self):
        legend = create_autospec(Legend)
        args = (None, False, "loc")
        legend_set_draggable(legend, *args)

        if hasattr(Legend, "set_draggable"):
            legend.set_draggable.assert_called_with(*args)
        else:
            legend.draggable.assert_called_with(*args)

    def test_row_num_for_a_figure_with_an_inset_axis(self):
        expected_row_nums = [0, 0, None]
        for axis, row_number in zip(self.fig.get_axes(), expected_row_nums):
            self.assertEqual(row_num(axis), row_number)

    def test_col_num_for_a_figure_with_an_inset_axis(self):
        expected_col_nums = [0, 1, None]
        for axis, col_number in zip(self.fig.get_axes(), expected_col_nums):
            self.assertEqual(col_num(axis), col_number)


if __name__ == "__main__":
    unittest.main()
