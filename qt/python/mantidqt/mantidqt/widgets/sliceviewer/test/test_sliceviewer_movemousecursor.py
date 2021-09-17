# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
import unittest
from unittest.mock import MagicMock, patch

from matplotlib.image import AxesImage
from mantidqt.widgets.sliceviewer.cursor import (MoveMouseCursorUp, MoveMouseCursorDown,
                                                 MoveMouseCursorLeft, MoveMouseCursorRight)
from qtpy.QtCore import QPoint


@patch("mantidqt.widgets.sliceviewer.cursor.QCursor")
class MoveMouseCursorTest(unittest.TestCase):
    def setUp(self):
        mock_canvas = MagicMock(_dpi_ratio=1.0)
        mock_figure = MagicMock()
        # matplotlib's coordinate system starts at bottom left & Qt top left
        canvas_height = 100
        mock_figure.bbox.height = canvas_height
        mock_figure.canvas = mock_canvas
        mock_canvas.figure = mock_figure

        mock_axes = MagicMock(figure=mock_figure)
        xmin, xmax, ymin, ymax = -5., 5., -10., 10.
        shape = (10, 20)
        mock_array = MagicMock(shape=(10, 20))
        self.mock_image = MagicMock(spec=AxesImage,
                                    axes=mock_axes,
                                    get_extent=lambda: (xmin, xmax, ymin, ymax),
                                    get_array=lambda: mock_array)
        self.deltax = (xmax - xmin) / shape[1]
        self.deltay = (ymax - ymin) / shape[0]
        # define identity transforms to make tests simpler
        self.mock_axes_transform = mock_axes.transData.transform_point
        self.mock_axes_transform.side_effect = lambda x: x
        self.mock_map_to_global = mock_canvas.mapToGlobal
        self.mock_map_to_global.side_effect = lambda x: x

    def test_move_up_keeps_X_constant(self, mock_cursor_cls):
        cur_pos_data = (-1, 1)
        expected_set_pos = QPoint(-1, 97)
        self._do_move_cursor_test(MoveMouseCursorUp, mock_cursor_cls, cur_pos_data,
                                  expected_set_pos)

    def test_move_down_keeps_X_constant(self, mock_cursor_cls):
        cur_pos_data = (-1, 1)
        expected_set_pos = QPoint(-1, 101)
        self._do_move_cursor_test(MoveMouseCursorDown, mock_cursor_cls, cur_pos_data,
                                  expected_set_pos)

    def test_move_left_keeps_Y_constant(self, mock_cursor_cls):
        cur_pos_data = (-1, 1)
        expected_set_pos = QPoint(-2, 1)
        self._do_move_cursor_test(MoveMouseCursorLeft, mock_cursor_cls, cur_pos_data,
                                  expected_set_pos)

    def test_move_right_keeps_Y_constant(self, mock_cursor_cls):
        cur_pos_data = (-1, 1)
        expected_set_pos = QPoint(0, 1)
        self._do_move_cursor_test(MoveMouseCursorRight, mock_cursor_cls, cur_pos_data,
                                  expected_set_pos)

    def test_move_respects_dpi(self, mock_cursor_cls):
        self.mock_image.axes.figure.canvas._dpi_ratio = 10.0
        cur_pos_data = (-1, 1)
        expected_set_pos = QPoint(-1, 9)
        self._do_move_cursor_test(MoveMouseCursorUp, mock_cursor_cls, cur_pos_data,
                                  expected_set_pos)

    # private api
    def _do_move_cursor_test(self, test_cls, mock_cursor_cls, cur_pos_data, expected_set_pos):
        mover = test_cls(self.mock_image)
        mock_cursor_cls.pos.return_value = QPoint(cur_pos_data[0], cur_pos_data[1])

        mover.move_from(cur_pos_data)

        mock_cursor_cls.setPos.assert_called_once_with(expected_set_pos)


if __name__ == '__main__':
    unittest.main()
