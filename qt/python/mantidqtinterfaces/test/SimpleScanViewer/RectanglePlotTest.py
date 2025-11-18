# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
import sys
import numpy as np

from qtpy.QtWidgets import QApplication
from matplotlib.widgets import Rectangle

from mantid.simpleapi import config
from mantid.api import mtd

from mantidqtinterfaces.simplescanviewer.rectangle_plot import MultipleRectangleSelectionLinePlot, UserInteraction
from mantidqtinterfaces.simplescanviewer.rectangle_controller import RectanglesManager

app = QApplication(sys.argv)


class RectanglePlotTest(unittest.TestCase):
    def setUp(self) -> None:
        self.facility = config["default.facility"]
        self.instrument = config["default.instrument"]
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D16"

        plotter = mock.MagicMock()
        plotter.image_axes._get_aspect_ratio.return_value = 1
        exporter = mock.MagicMock()
        exporter.rectangles_manager = RectanglesManager()

        self.rectangle_plot = MultipleRectangleSelectionLinePlot(plotter, exporter)

    def tearDown(self) -> None:
        config["default.facility"] = self.facility
        config["default.instrument"] = self.instrument
        mtd.clear()

    @staticmethod
    def create_data_array(width: int = 500, height: int = 500, peaks: list = (), sigma: float = 10):
        arr = np.fromfunction(lambda x, y: sum([np.exp(-((x - px) ** 2 + (y - py) ** 2) / sigma) for px, py in peaks]), (width, height))

        return arr

    def test_determine_behaviour(self):
        click_event = mock.Mock()
        release_event = mock.Mock()

        click_event.xdata = 0
        release_event.xdata = 0
        click_event.ydata = 1
        release_event.ydata = 1

        self.assertEqual(self.rectangle_plot._determine_behaviour(click_event, release_event), UserInteraction.RECTANGLE_SELECTED)

        click_event.xdata = 0
        click_event.ydata = 0
        release_event.xdata = 1
        release_event.ydata = 1

        self.assertEqual(self.rectangle_plot._determine_behaviour(click_event, release_event), UserInteraction.RECTANGLE_CREATED)

        rect = Rectangle((0, 0), 1, 1)
        self.rectangle_plot._manager.add_rectangle(rect)

        click_event.xdata = 1
        click_event.ydata = 1
        release_event.xdata = 2
        release_event.ydata = 2

        self.assertEqual(self.rectangle_plot._determine_behaviour(click_event, release_event), UserInteraction.RECTANGLE_RESHAPED)

        click_event.xdata = 0.5
        click_event.ydata = 0.5
        release_event.xdata = 1.5
        release_event.ydata = 1.5

        self.assertEqual(self.rectangle_plot._determine_behaviour(click_event, release_event), UserInteraction.RECTANGLE_MOVED)

        click_event.xdata = 10
        click_event.ydata = 10
        release_event.xdata = 15
        release_event.ydata = 15

        self.assertEqual(self.rectangle_plot._determine_behaviour(click_event, release_event), UserInteraction.RECTANGLE_CREATED)

    def test_select_rectangle(self):
        self.rectangle_plot._selector = mock.Mock()

        rect1 = Rectangle((0, 0), 1, 1)
        self.rectangle_plot._manager.add_rectangle(rect1)
        rect2 = Rectangle((10, 10), 2, 2)
        self.rectangle_plot._manager.add_rectangle(rect2)

        self.assertEqual(self.rectangle_plot.current_rectangle, rect2)

        self.rectangle_plot._select_rectangle((0.5, 0.5))
        self.assertEqual(self.rectangle_plot.current_rectangle, rect1)
        self.assertEqual(self.rectangle_plot._selector.extents, (0, 1, 0, 1))

    def test_snap_to_edges(self):
        self.rectangle_plot._selector = mock.MagicMock()

        self.rectangle_plot.exporter.get_axes = lambda: (np.linspace(0, 100, 101), np.linspace(0, 100, 101))
        self.rectangle_plot.plotter.image.get_extent = lambda: (0, 100, 0, 100)

        point_1 = (0, 0)
        point_2 = (10, 10)

        corner, width, height = self.rectangle_plot._snap_to_edges(point_1, point_2)
        self.assertEqual(self.rectangle_plot._selector.extents, (corner[0], corner[0] + width, corner[1], corner[1] + height))
        self.assertEqual(corner, (0, 0))
        self.assertEqual(width, 9.5)
        self.assertEqual(height, 9.5)

        point_1 = (5.6, 5.6)
        point_2 = (10.2, 10.2)

        corner, width, height = self.rectangle_plot._snap_to_edges(point_1, point_2)
        self.assertEqual(self.rectangle_plot._selector.extents, (corner[0], corner[0] + width, corner[1], corner[1] + height))
        self.assertEqual(corner, (5.5, 5.5))
        self.assertEqual(width, 5)
        self.assertEqual(height, 5)

        point_1 = (0.2, 0.2)
        point_2 = (10.2, 10.2)

        corner, width, height = self.rectangle_plot._snap_to_edges(point_1, point_2)
        self.assertEqual(self.rectangle_plot._selector.extents, (corner[0], corner[0] + width, corner[1], corner[1] + height))
        self.assertEqual(corner, (0, 0))
        self.assertEqual(width, 10.5)
        self.assertEqual(height, 10.5)

        point_1 = (10.1, 10.1)
        point_2 = (10.2, 10.2)

        corner, width, height = self.rectangle_plot._snap_to_edges(point_1, point_2)
        self.assertEqual(self.rectangle_plot._selector.extents, (corner[0], corner[0] + width, corner[1], corner[1] + height))
        self.assertEqual(corner, (9.5, 9.5))
        self.assertEqual(width, 1)
        self.assertEqual(height, 1)

        point_1 = (10.4, 10.4)
        point_2 = (10.7, 10.7)

        corner, width, height = self.rectangle_plot._snap_to_edges(point_1, point_2)
        self.assertEqual(self.rectangle_plot._selector.extents, (corner[0], corner[0] + width, corner[1], corner[1] + height))
        self.assertEqual(corner, (10.5, 10.5))
        self.assertEqual(width, 1)
        self.assertEqual(height, 1)

        point_1 = (0.1, 0.1)
        point_2 = (0.2, 0.2)

        corner, width, height = self.rectangle_plot._snap_to_edges(point_1, point_2)
        self.assertEqual(self.rectangle_plot._selector.extents, (corner[0], corner[0] + width, corner[1], corner[1] + height))
        self.assertEqual(corner, (0, 0))
        self.assertEqual(width, 0.5)
        self.assertEqual(height, 0.5)

        # test with the anomaly where the borders are larger that they should be
        self.rectangle_plot.plotter.image.get_extent = lambda: (-0.5, 100.5, -0.5, 100.5)
        point_1 = (0.1, 0.1)
        point_2 = (0.2, 0.2)

        corner, width, height = self.rectangle_plot._snap_to_edges(point_1, point_2)
        self.assertEqual(self.rectangle_plot._selector.extents, (corner[0], corner[0] + width, corner[1], corner[1] + height))
        self.assertEqual(corner, (-0.5, -0.5))
        self.assertEqual(width, 1)
        self.assertEqual(height, 1)

    def test_move_selected_rect(self):
        self.rectangle_plot._selector = mock.MagicMock()
        trigger_check = mock.Mock()
        self.rectangle_plot._update_plot_values = mock.MagicMock()

        self.rectangle_plot.signals.sig_current_updated.connect(trigger_check)
        self.rectangle_plot._find_peak = lambda x: x
        self.rectangle_plot._show_peak = lambda x, y: x

        rect1 = Rectangle((0, 0), 1, 1)
        self.rectangle_plot._manager.add_rectangle(rect1)
        self.rectangle_plot._move_selected_rectangle((1, 1), 1, 1)

        trigger_check.assert_called_once()
        self.assertEqual(self.rectangle_plot._update_plot_values.call_count, 5)
        self.assertEqual(self.rectangle_plot.current_rectangle.get_x(), 1)
        self.assertEqual(self.rectangle_plot.current_rectangle.get_y(), 1)
        self.assertEqual(self.rectangle_plot.current_rectangle.get_height(), 1)
        self.assertEqual(self.rectangle_plot.current_rectangle.get_width(), 1)

    def test_update_plot_value(self):
        self.rectangle_plot._compute_plot_axes = lambda: ((0, 0), (0, 0))
        self.rectangle_plot._update_plot_values()

        self.rectangle_plot.plotter.plot_x_line.assert_called_once()
        self.rectangle_plot.plotter.plot_x_line.assert_called_once()

        self.rectangle_plot.plotter.update_line_plot_limits.assert_called_once()
        self.rectangle_plot.plotter.redraw.assert_called_once()

    def test_compute_plot_axes(self):
        self.rectangle_plot.plotter.image.get_extent = lambda: (0, 100, 0, 100)

        # create a data array with two peaks
        arr = self.create_data_array(100, 100, ((10, 10), (70, 70)), 5)
        self.rectangle_plot.plotter.image.get_array = lambda: arr

        # simple case of a single rectangle around one peak
        rect = Rectangle((5, 5), 10, 10)
        self.rectangle_plot._manager.add_rectangle(rect)
        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self.rectangle_plot._compute_plot_axes()

        self.assertEqual(x_line_x_axis.shape, (arr.shape[1],))
        self.assertEqual(x_line_y_values.shape, (arr.shape[1],))
        self.assertEqual(y_line_x_axis.shape, (arr.shape[0],))
        self.assertEqual(y_line_y_values.shape, (arr.shape[0],))
        self.assertAlmostEqual(x_line_y_values[10], 3.955, 3)
        self.assertAlmostEqual(y_line_y_values[10], 3.955, 3)
        self.assertTrue(x_line_y_values.mask[4])
        self.assertTrue(y_line_y_values.mask[4])

        # check that overlapping rectangles are not counted twice
        rect = Rectangle((0, 0), 15, 15)
        self.rectangle_plot._manager.add_rectangle(rect)
        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self.rectangle_plot._compute_plot_axes()

        self.assertEqual(x_line_x_axis.shape, (arr.shape[1],))
        self.assertEqual(x_line_y_values.shape, (arr.shape[1],))
        self.assertEqual(y_line_x_axis.shape, (arr.shape[0],))
        self.assertEqual(y_line_y_values.shape, (arr.shape[0],))
        self.assertAlmostEqual(x_line_y_values[10], 3.956, 3)
        self.assertAlmostEqual(y_line_y_values[10], 3.956, 3)
        self.assertFalse(x_line_y_values.mask[5])
        self.assertFalse(y_line_y_values.mask[5])

        # check that non contiguous roi also works
        rect = Rectangle((60, 60), 20, 20)
        self.rectangle_plot._manager.add_rectangle(rect)
        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self.rectangle_plot._compute_plot_axes()

        self.assertEqual(x_line_x_axis.shape, (arr.shape[1],))
        self.assertEqual(x_line_y_values.shape, (arr.shape[1],))
        self.assertEqual(y_line_x_axis.shape, (arr.shape[0],))
        self.assertEqual(y_line_y_values.shape, (arr.shape[0],))
        self.assertAlmostEqual(x_line_y_values[70], 3.963, 3)
        self.assertAlmostEqual(y_line_y_values[70], 3.963, 3)
        self.assertTrue(x_line_y_values.mask[15])
        self.assertTrue(y_line_y_values.mask[15])

        # integrate over the entire data
        rect = Rectangle((0, 0), 100, 100)
        self.rectangle_plot._manager.add_rectangle(rect)
        (x_line_x_axis, x_line_y_values), (y_line_x_axis, y_line_y_values) = self.rectangle_plot._compute_plot_axes()

        self.assertEqual(x_line_x_axis.shape, (arr.shape[1],))
        self.assertEqual(x_line_y_values.shape, (arr.shape[1],))
        self.assertEqual(y_line_x_axis.shape, (arr.shape[0],))
        self.assertEqual(y_line_y_values.shape, (arr.shape[0],))
        self.assertAlmostEqual(max(x_line_y_values), 3.963, 3)
        self.assertAlmostEqual(max(y_line_y_values), 3.963, 3)
        self.assertAlmostEqual(x_line_y_values[10], 3.963, 3)
        self.assertAlmostEqual(x_line_y_values[70], 3.963, 3)
        self.assertAlmostEqual(y_line_y_values[10], 3.963, 3)
        self.assertAlmostEqual(y_line_y_values[70], 3.963, 3)

    def test_extract_projections(self):
        self.rectangle_plot.plotter.image.get_extent = lambda: (0, 100, 0, 100)

        # create a data array with two peaks
        arr = self.create_data_array(100, 100, ((10, 10), (70, 70)), 5)
        self.rectangle_plot.plotter.image.get_array = lambda: arr

        # simple case of a single rectangle around one peak
        rect = Rectangle((5, 5), 10, 10)
        self.rectangle_plot._manager.add_rectangle(rect)

        self.rectangle_plot._extract_projections()

        self.assertTrue(mtd.doesExist("x_cut"))
        self.assertTrue(mtd.doesExist("x_cut_0"))
        self.assertTrue(mtd.doesExist("y_cut"))
        self.assertTrue(mtd.doesExist("y_cut_0"))

        x_cut = mtd["x_cut_0"]
        data = x_cut.dataY(0)
        self.assertEqual(data.shape[0], 10)
        self.assertAlmostEqual(data[5], 3.955, 3)

        y_cut = mtd["y_cut_0"]
        data = y_cut.dataY(0)
        self.assertEqual(data.shape[0], 10)
        self.assertAlmostEqual(data[5], 3.955, 3)

        mtd.clear()

        self.rectangle_plot._extract_projections(extract_x=True, extract_y=False)
        self.assertTrue(mtd.doesExist("x_cut"))
        self.assertTrue(mtd.doesExist("x_cut_0"))
        self.assertFalse(mtd.doesExist("y_cut"))
        self.assertFalse(mtd.doesExist("y_cut_0"))

        mtd.clear()

        self.rectangle_plot._extract_projections(extract_x=False, extract_y=True)
        self.assertFalse(mtd.doesExist("x_cut"))
        self.assertFalse(mtd.doesExist("x_cut_0"))
        self.assertTrue(mtd.doesExist("y_cut"))
        self.assertTrue(mtd.doesExist("y_cut_0"))

        mtd.clear()

        self.rectangle_plot._extract_projections(extract_x=False, extract_y=False)
        self.assertFalse(mtd.doesExist("x_cut"))
        self.assertFalse(mtd.doesExist("x_cut_0"))
        self.assertFalse(mtd.doesExist("y_cut"))
        self.assertFalse(mtd.doesExist("y_cut_0"))

        mtd.clear()

    def test_place_interpolate_linear(self):
        self.rectangle_plot.plotter.image.get_extent = lambda: (0, 100, 0, 100)
        self.rectangle_plot._draw_rectangle = mock.MagicMock()

        # first check with only one rectangle
        rect = Rectangle((10, 10), 10, 10)
        self.rectangle_plot._manager.add_rectangle(rect)
        self.rectangle_plot._find_peak = lambda x: (15, 15)

        self.rectangle_plot._place_interpolate_linear()

        calls = [mock.call((10 + 15 * i, 10 + 15 * i), 10, 10) for i in range(1, 5)]
        self.assertEqual(self.rectangle_plot._draw_rectangle.call_count, 5)
        self.rectangle_plot._draw_rectangle.assert_has_calls(calls)

        # reset the calls to the function
        self.rectangle_plot._draw_rectangle = mock.MagicMock()

        # a second rectangle is placed, so the new ones are placed in reference to them
        rect_2 = Rectangle((30, 20), 10, 10)
        self.rectangle_plot._manager.add_rectangle(rect_2)

        self.rectangle_plot._place_interpolate_linear()

        calls = [mock.call((50, 30), 10, 10), mock.call((70, 40), 10, 10), mock.call((90, 50), 10, 10)]
        self.assertEqual(self.rectangle_plot._draw_rectangle.call_count, 3)
        self.rectangle_plot._draw_rectangle.assert_has_calls(calls)

    def test_rectangle_fit(self):
        self.rectangle_plot.plotter.image.get_extent = mock.Mock()
        self.rectangle_plot.plotter.image.get_extent.return_value = (0, 100, -100, 0)

        self.assertTrue(self.rectangle_plot.rectangle_fit_on_image((10, -90), 10, 10))
        self.assertTrue(self.rectangle_plot.rectangle_fit_on_image((50, -50), 100, 100))

        self.assertFalse(self.rectangle_plot.rectangle_fit_on_image((-10, -20), 10, 10))
        self.assertFalse(self.rectangle_plot.rectangle_fit_on_image((10, 0), 10, 10))

    def test_extract_peaks(self):
        rect1 = Rectangle((0, 0), 10, 10)
        rect2 = Rectangle((30, 30), 20, 20)

        self.rectangle_plot._manager.add_rectangle(rect1)
        self.rectangle_plot._manager.add_rectangle(rect2)

        peaks = {rect1: (5, 5, 5, 5), rect2: (40, 40, 10, 10)}

        self.rectangle_plot._find_peak = mock.Mock()
        self.rectangle_plot._find_peak.side_effect = lambda x: peaks[x] if x in peaks else None

        self.rectangle_plot._extract_peaks()

        self.assertTrue(mtd.doesExist("peaks"))
        peak_ws = mtd["peaks"]
        self.assertEqual(peak_ws.rowCount(), 2)
        self.assertEqual(peak_ws.columnCount(), 5)
        self.assertEqual(peak_ws.cell(0, 0), 0)
        self.assertEqual(peak_ws.cell(1, 1), 40)
        self.assertEqual(peak_ws.cell(0, 2), 5)
        self.assertEqual(peak_ws.cell(0, 3), 5)
        self.assertEqual(peak_ws.cell(0, 4), 5)

    def test_find_peaks(self):
        extent = (0, 500, 0, 500)
        arr = self.create_data_array(width=500, height=500, peaks=((70, 70), (200, 200), (210, 210)), sigma=15)

        self.rectangle_plot.plotter.image.get_extent = mock.Mock()
        self.rectangle_plot.plotter.image.get_extent.return_value = extent

        self.rectangle_plot.plotter.image.get_array = mock.Mock()
        self.rectangle_plot.plotter.image.get_array.return_value = arr

        # only the center of the peak
        rect = Rectangle((70, 70), 1, 1, facecolor="none", edgecolor="black")
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], 70.0, 5)
        self.assertAlmostEqual(peak[1], 70.0, 5)

        # a couple of pixels
        rect = Rectangle((69, 69), 3, 3)
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], 70.5, 5)
        self.assertAlmostEqual(peak[1], 70.5, 5)

        # a wider rectangle
        rect = Rectangle((50, 50), 40, 40)
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], 71.567, 2)
        self.assertAlmostEqual(peak[1], 71.567, 2)

        # off centered rectangle - this time the peak is not just in the middle of the rectangle
        rect = Rectangle((205, 205), 290, 290)
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], (205 + 290 / 2), 3)
        self.assertAlmostEqual(peak[1], (205 + 290 / 2), 3)

        rect = Rectangle((40, 50), 40, 30)
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], 71.71, 2)
        self.assertAlmostEqual(peak[1], 71.44, 2)

        # far enough from the peak that there is no interference from them, so it is an empty rectangle
        rect = Rectangle((450, 450), 10, 10)
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], 455.0, 5)
        self.assertAlmostEqual(peak[1], 455.0, 5)

        # two peaks at once, finds the middle
        rect = Rectangle((150, 150), 100, 100)
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], 203.945, 2)
        self.assertAlmostEqual(peak[1], 203.945, 2)

        # rectangle completely off screen
        rect = Rectangle((-200, -200), 10, 10)
        peak = self.rectangle_plot._find_peak(rect)
        self.assertAlmostEqual(peak[0], 0.0, 5)
        self.assertAlmostEqual(peak[1], 0.0, 5)

    def test_show_peak(self):
        rect = Rectangle((0, 0), 1, 1)
        peak = (0.5, 0.5)

        self.rectangle_plot._manager.add_rectangle(rect)

        self.assertEqual(self.rectangle_plot._manager.rectangles[0][0].peak_plot, None)
        self.rectangle_plot._show_peak(rect, peak)
        self.assertNotEqual(self.rectangle_plot._manager.rectangles[0][0].peak_plot, None)

    def test_handle_key(self):
        mock_extract_projection = mock.Mock()
        self.rectangle_plot._extract_projections = mock_extract_projection
        mock_place_interpolated = mock.Mock()
        self.rectangle_plot._place_interpolated_rectangles = mock_place_interpolated
        mock_extract_peaks = mock.Mock()
        self.rectangle_plot._extract_peaks = mock_extract_peaks
        mock_delete = mock.Mock()
        self.rectangle_plot._delete_current = mock_delete

        self.rectangle_plot.handle_key("g")
        mock_delete.assert_not_called()
        mock_extract_peaks.assert_not_called()
        mock_place_interpolated.assert_not_called()
        mock_extract_projection.assert_not_called()

        self.rectangle_plot.handle_key("f")
        mock_delete.assert_not_called()
        mock_extract_peaks.assert_not_called()
        mock_extract_projection.assert_not_called()
        mock_place_interpolated.assert_called_once()

        mock_place_interpolated.reset_mock()

        self.rectangle_plot.handle_key("p")
        mock_delete.assert_not_called()
        mock_place_interpolated.assert_not_called()
        mock_extract_projection.assert_not_called()
        mock_extract_peaks.assert_called_once()

        mock_extract_peaks.reset_mock()

        self.rectangle_plot.handle_key("delete")
        mock_place_interpolated.assert_not_called()
        mock_extract_projection.assert_not_called()
        mock_extract_peaks.assert_not_called()
        mock_delete.assert_called_once()

        mock_delete.reset_mock()

        self.rectangle_plot.handle_key("c")
        mock_delete.assert_not_called()
        mock_place_interpolated.assert_not_called()
        mock_extract_peaks.assert_not_called()
        mock_extract_projection.assert_called_once()
        mock_extract_projection.assert_has_calls([mock.call(extract_x=True, extract_y=True)])


if __name__ == "__main__":
    unittest.main()
