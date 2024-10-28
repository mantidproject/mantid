# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest
from unittest.mock import patch
from matplotlib import use as mpl_use

mpl_use("Agg")
import numpy as np
from workbench.plotting.plothelppages import BASE_PATH, COLORFILL_PAGE, INDEX_PAGE, PLOT1D_PAGE, PLOT3D_PAGE, WATERFALL_PAGE, PlotHelpPages
from matplotlib.pyplot import figure


class PlotHelpPagesTest(unittest.TestCase):
    @staticmethod
    # Generate some data
    def generate_XYZ_data():
        def fun(x, y):
            return x**2 + y

        x = y = np.arange(-3.0, 3.0, 0.05)
        X, Y = np.meshgrid(x, y)
        zs = np.array(fun(np.ravel(X), np.ravel(Y)))
        Z = zs.reshape(X.shape)
        return X, Y, Z

    @patch("workbench.plotting.plothelppages.InterfaceManager.showHelpPage")
    def test_show_help_page_returns_correctly_for_1d_plots(self, mock_show_page):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        ax.plot([0, 0], [1, 1])
        ax.plot([0, 1], [1, 2])

        PlotHelpPages.show_help_page_for_figure(fig)

        mock_show_page.assert_any_call(BASE_PATH + PLOT1D_PAGE)

        ax.set_waterfall(True)

        PlotHelpPages.show_help_page_for_figure(fig)

        mock_show_page.assert_any_call(BASE_PATH + WATERFALL_PAGE)

    @patch("workbench.plotting.plothelppages.InterfaceManager.showHelpPage")
    def test_show_help_page_returns_plotting_index_if_plot_type_unrecongised(self, mock_show_page):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        ax.bar([0, 0], [1, 1])

        PlotHelpPages.show_help_page_for_figure(fig)

        mock_show_page.assert_called_once_with(BASE_PATH + INDEX_PAGE)

    @patch("workbench.plotting.plothelppages.InterfaceManager.showHelpPage")
    def test_show_help_page_correctly_returns_3D_plot_page(self, mock_show_page):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid3d")
        X, Y, Z = self.generate_XYZ_data()
        ax.plot_surface(X, Y, Z)

        PlotHelpPages.show_help_page_for_figure(fig)

        mock_show_page.assert_called_once_with(BASE_PATH + PLOT3D_PAGE)

    @patch("workbench.plotting.plothelppages.InterfaceManager.showHelpPage")
    def test_show_help_page_correctly_returns_colorfill_plot_page(self, mock_show_page):
        fig = figure()
        ax = fig.add_subplot(111, projection="mantid")
        X, Y, Z = self.generate_XYZ_data()
        ax.pcolormesh(X, Y, Z)

        PlotHelpPages.show_help_page_for_figure(fig)

        mock_show_page.assert_called_once_with(BASE_PATH + COLORFILL_PAGE)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
