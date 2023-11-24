# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
import unittest

import matplotlib

matplotlib.use("AGG")
import matplotlib.pyplot as plt
from numpy import array_equal

# Pulling in the MantidAxes registers the 'mantid' projection
from mantid.simpleapi import CreateWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plotting.figureerrorsmanager import FigureErrorsManager


def plot_things(make_them_errors):
    def function_reference(func):
        def function_parameters(self):
            if not make_them_errors:
                # plot this line with specNum
                self.ax.plot(self.ws2d_histo, specNum=1)
                # and another one with wkspIndex
                self.ax.plot(self.ws2d_histo, wkspIndex=2)
            else:
                self.ax.errorbar(self.ws2d_histo, specNum=1)
                self.ax.errorbar(self.ws2d_histo, wkspIndex=2)
            return func(self)

        return function_parameters

    return function_reference


@start_qapplication
class FigureErrorsManagerTest(unittest.TestCase):
    """
    Test class that covers the interaction of the FigureErrorsManager with plots
    that use the mantid projection and have MantidAxes
    """

    @classmethod
    def setUpClass(cls):
        cls.ws2d_histo = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5, 3, 5],
            DataE=[1, 2, 3, 4, 1, 1],
            NSpec=3,
            Distribution=True,
            UnitX="Wavelength",
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[4, 6, 8],
            OutputWorkspace="ws2d_histo",
        )
        # initialises the QApplication
        super(cls, FigureErrorsManagerTest).setUpClass()

    def setUp(self):
        self.fig, self.ax = plt.subplots(subplot_kw={"projection": "mantid"})

        self.errors_manager = FigureErrorsManager(self.fig.canvas)

    def tearDown(self):
        plt.close("all")
        self.ax.clear()
        del self.fig
        del self.ax
        del self.errors_manager

    @plot_things(make_them_errors=False)
    def test_show_all_errors(self):
        # assert plot does not have errors
        self.assertEqual(0, len(self.ax.containers))

        self.errors_manager.toggle_all_errors(self.ax, make_visible=True)

        # check that the errors have been added
        self.assertEqual(2, len(self.ax.containers))

    @plot_things(make_them_errors=True)
    def test_hide_all_errors(self):
        self.assertEqual(2, len(self.ax.containers))
        self.errors_manager.toggle_all_errors(self.ax, make_visible=False)
        # errors still exist
        self.assertEqual(2, len(self.ax.containers))
        # they are just invisible
        self.assertFalse(self.ax.containers[0][2][0].get_visible())

    @plot_things(make_them_errors=True)
    def test_hide_all_errors_retains_legend_properties(self):
        # create a legend with a title
        self.ax.legend(title="Test")

        self.errors_manager.toggle_all_errors(self.ax, make_visible=False)

        # check that the legend still has a title
        self.assertEqual(self.ax.get_legend().get_title().get_text(), "Test")

    @plot_things(make_them_errors=False)
    def test_show_all_errors_retains_legend_properties(self):
        # create a legend with a title
        self.ax.legend(title="Test")

        self.errors_manager.toggle_all_errors(self.ax, make_visible=True)

        # check that the legend still has a title
        self.assertEqual(self.ax.get_legend().get_title().get_text(), "Test")

    def test_curve_has_all_errorbars_on_replot_after_error_every_increase(self):
        curve = self.ax.errorbar([0, 1, 2, 4], [0, 1, 2, 4], yerr=[0.1, 0.2, 0.3, 0.4])
        new_curve = FigureErrorsManager._replot_mpl_curve(self.ax, curve, {"errorevery": 2})
        self.assertEqual(2, len(new_curve[2][0].get_segments()))
        new_curve = FigureErrorsManager._replot_mpl_curve(self.ax, new_curve, {"errorevery": 1})
        self.assertTrue(hasattr(new_curve, "errorbar_data"))
        self.assertEqual(4, len(new_curve[2][0].get_segments()))

    def test_show_all_errors_on_waterfall_plot_retains_waterfall(self):
        self.ax.plot([0, 1], [0, 1])
        self.ax.plot([0, 1], [0, 1])
        self.ax.set_waterfall(True)

        self.errors_manager.toggle_all_errors(self.ax, make_visible=True)

        self.assertFalse(array_equal(self.ax.get_lines()[0].get_data(), self.ax.get_lines()[1].get_data()))

    def test_hide_all_errors_on_waterfall_plot_retains_waterfall(self):
        self.ax.plot([0, 1], [0, 1])
        self.ax.plot([0, 1], [0, 1])
        self.ax.set_waterfall(True)

        self.errors_manager.toggle_all_errors(self.ax, make_visible=True)
        self.errors_manager.toggle_all_errors(self.ax, make_visible=False)

        self.assertFalse(array_equal(self.ax.get_lines()[0].get_data(), self.ax.get_lines()[1].get_data()))

    def test_creation_args_not_accessed_for_non_workspace_plots(self):
        self.ax.plot([1, 2], [1, 2])

        self.errors_manager.replot_curve(self.ax, self.ax.lines[0], {})
        self.assertEqual(0, len(self.ax.creation_args))


if __name__ == "__main__":
    unittest.main()
