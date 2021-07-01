# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from unittest.mock import patch

import matplotlib

matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt

from mantidqt.utils.qt.testing import start_qapplication
from workbench.plotting.figuremanager import MantidFigureCanvas, FigureManagerWorkbench


@start_qapplication
class ToolBarTest(unittest.TestCase):
    """
    Test that the grids on/off toolbar button has the correct state when creating a plot in various different cases.
    """

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_plot_with_no_grid(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={'projection': 'mantid'})
        axes.plot([-10, 10], [1, 2])
        # Grid button should be OFF because we have not enabled the grid.
        self.assertFalse(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_checked_for_plot_with_grid(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={'projection': 'mantid'})
        axes.plot([-10, 10], [1, 2])
        axes.grid()
        # Grid button should be ON because we enabled the grid.
        self.assertTrue(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_checked_for_plot_with_grid_using_kwargs(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={'projection': 'mantid'})
        axes.plot([-10, 10], [1, 2])
        # Set the grid on using kwargs in tick_params, like the plot script generator.
        axes.tick_params(axis='x', which='major', **{'gridOn': True})
        axes.tick_params(axis='y', which='major', **{'gridOn': True})

        # Grid button should be ON because we enabled the grid on both axes.
        self.assertTrue(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_plot_with_only_x_grid_using_kwargs(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={'projection': 'mantid'})
        axes.plot([-10, 10], [1, 2])
        # Set the grid on using kwargs in tick_params, like the plot script generator.
        axes.tick_params(axis='x', which='major', **{'gridOn': True})

        # Grid button should be OFF because we only enabled the grid on one axis.
        self.assertFalse(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_tiled_plot_with_no_grids(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={'projection': 'mantid'})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
        # None of the subplots have grids, so grid button should be toggled OFF.
        self.assertFalse(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_checked_for_tiled_plot_with_all_grids(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={'projection': 'mantid'})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
            ax.grid()
        # All subplots have grids, so button should be toggled ON.
        self.assertTrue(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_tiled_plot_with_some_grids(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={'projection': 'mantid'})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
        # Only show major grid on 3/4 of the subplots.
        axes[0][0].grid()
        axes[0][1].grid()
        axes[1][0].grid()
        # Grid button should be OFF because not all subplots have grids.
        self.assertFalse(self._is_grid_button_checked(fig))

    @classmethod
    def _is_grid_button_checked(cls, fig):
        """
        Create the figure manager and check whether its toolbar is toggled on or off for the given figure.
        We have to explicity call set_button_visibilty() here, which would otherwise be called within the show()
        function.
        """
        canvas = MantidFigureCanvas(fig)
        fig_manager = FigureManagerWorkbench(canvas, 1)
        # This is only called when show() is called on the figure manager, so we have to manually call it here.
        fig_manager.toolbar.set_buttons_visibility(fig)
        return fig_manager.toolbar._actions['toggle_grid'].isChecked()


if __name__ == '__main__':
    unittest.main()
