# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
import unittest
from unittest.mock import MagicMock, call, patch

# 3rd party imports
from mantidqt.widgets.sliceviewer.lineplots import LinePlots, PixelLinePlot
from mantidqt.utils.testing.compare import ArraysEqual

from matplotlib.figure import SubplotParams
import numpy as np


class LinePlotsTest(unittest.TestCase):
    def setUp(self):
        self.image_axes = _create_mock_axes()
        self.axx, self.axy = MagicMock(), MagicMock()
        self.image_axes.figure.add_subplot.side_effect = [self.axx, self.axy]
        self.mock_colorbar = MagicMock(cmin_value=1.0, cmax_value=50.)
        self.mock_colorbar.get_colorbar_scale = MagicMock()
        self.mock_colorbar.get_colorbar_scale.return_value = [1.0, {}]

    @patch('mantidqt.widgets.sliceviewer.lineplots.GridSpec')
    def test_construction_adds_line_plots_to_axes(self, mock_gridspec):
        gs = mock_gridspec()
        mock_gridspec.reset_mock()
        LinePlots(self.image_axes, self.mock_colorbar)

        fig = self.image_axes.figure
        self.assertEqual(2, fig.add_subplot.call_count)
        self.assertEqual(1, mock_gridspec.call_count)
        # use spaces at 0, 1 & 3 in grid
        gs.__getitem__.assert_has_calls((call(0), call(1), call(3)), any_order=True)
        self.assertTrue('sharey' in fig.add_subplot.call_args_list[0][1] or 'sharey' in fig.add_subplot.call_args_list[1][1])
        self.assertTrue('sharex' in fig.add_subplot.call_args_list[0][1] or 'sharex' in fig.add_subplot.call_args_list[1][1])

    def test_delete_plot_lines_handles_empty_plots(self):
        plotter = LinePlots(self.image_axes, self.mock_colorbar)

        plotter.delete_line_plot_lines()

    def test_delete_plot_lines_with_plots_present(self):
        plotter = LinePlots(self.image_axes, self.mock_colorbar)
        xfig, yfig = MagicMock(), MagicMock()
        self.axx.plot.side_effect = [[xfig]]
        self.axy.plot.side_effect = [[yfig]]
        x, y = np.arange(10.), np.arange(10.)
        plotter.plot_x_line(x, y)
        plotter.plot_y_line(x, y)

        plotter.delete_line_plot_lines()

        xfig.remove.assert_called_once()
        yfig.remove.assert_called_once()

    def test_plot_with_no_line_present_creates_line_artist(self):
        plotter = LinePlots(self.image_axes, self.mock_colorbar)
        self.axx.set_xlabel.reset_mock()
        x, y = np.arange(10.), np.arange(10.) * 2

        plotter.plot_x_line(x, y)
        self.axx.plot.assert_called_once_with(x, y, scalex=False)
        self.axx.set_xlabel.assert_called_once_with(self.image_axes.get_xlabel())

        self.axy.set_ylabel.reset_mock()
        self.axy.set_xlim.reset_mock()
        plotter.plot_y_line(x, y)
        self.axy.plot.assert_called_once_with(y, x, scaley=False)
        self.axy.set_ylabel.assert_called_once_with(self.image_axes.get_ylabel())

    def test_plot_lines_are_set_to_width_of_below_default(self):
        plotter = LinePlots(self.image_axes, self.mock_colorbar)
        self.axx.set_xlabel.reset_mock()
        x, y = np.arange(10.), np.arange(10.) * 2

        plotter.plot_x_line(x, y)
        self.axx.plot.assert_called_once_with(x, y, scalex=False)
        self.assert_that_a_call_has_been_made_to_this_object_containing_linewidth_and_half(self.axx.plot)

        self.axy.set_ylabel.reset_mock()
        self.axy.set_xlim.reset_mock()
        plotter.plot_y_line(x, y)
        self.axy.plot.assert_called_once_with(y, x, scaley=False)
        self.assert_that_a_call_has_been_made_to_this_object_containing_linewidth_and_half(self.axy.plot)

    def assert_that_a_call_has_been_made_to_this_object_containing_linewidth_and_half(self, obj):
        one_of_the_calls_is_linewidth = False
        for mock_call in obj.mock_calls:
            if mock_call[0] == "().__getitem__().set_linewidth" or mock_call[1] == 0.5:
                one_of_the_calls_is_linewidth = True
        self.assertTrue(one_of_the_calls_is_linewidth)

    def test_plot_with_line_present_sets_data(self):
        plotter = LinePlots(self.image_axes, self.mock_colorbar)
        x, y = np.arange(10.), np.arange(10.) * 2
        plotter.plot_x_line(x, y)
        plotter.plot_y_line(x, y)
        self.axx.reset_mock()
        self.axy.reset_mock()

        plotter.plot_x_line(x, y)
        plotter.plot_y_line(x, y)

        plotter._xfig.set_data.assert_called_once_with(x, y)
        plotter._yfig.set_data.assert_called_once_with(y, x)
        self.axx.plot.assert_not_called()
        self.axy.plot.assert_not_called()


class PixelLinePlotTest(unittest.TestCase):
    def test_cursor_at_generates_xy_plots(self):
        image_axes = _create_mock_axes()
        mock_image = MagicMock()
        mock_image.get_extent.return_value = (-1, 1, -3, 3)
        signal = np.arange(25.).reshape(5, 5)
        mock_image.get_array.return_value = signal
        image_axes.images = [mock_image]
        plotter = MagicMock(image_axes=image_axes, image=mock_image)
        pixel_plots = PixelLinePlot(plotter)

        pixel_plots.on_cursor_at(0.0, 1.0)

        plotter.plot_x_line.assert_called_once_with(ArraysEqual(np.linspace(-1, 1, 5)),
                                                    ArraysEqual(signal[3, :]))
        plotter.plot_y_line.assert_called_once_with(ArraysEqual(np.linspace(-3, 3, 5)),
                                                    ArraysEqual(signal[:, 2]))

    def test_cursor_outside_axes_deletes_plot_lines(self):
        plotter = MagicMock()
        pixel_plots = PixelLinePlot(plotter)

        pixel_plots.on_cursor_outside_axes()

        plotter.delete_line_plot_lines.assert_called_once()


def _create_mock_axes():
    image_axes = MagicMock()
    image_axes.figure.subplotpars = SubplotParams(0.125, 0.11, 0.9, 0.88, 0.2, 0.2)
    image_axes.get_xlim.return_value = (-1, 1)
    image_axes.get_ylim.return_value = (-3, 3)
    image_axes.get_xlabel.return_value = 'x'
    image_axes.get_ylabel.return_value = 'y'
    return image_axes


if __name__ == '__main__':
    unittest.main()
