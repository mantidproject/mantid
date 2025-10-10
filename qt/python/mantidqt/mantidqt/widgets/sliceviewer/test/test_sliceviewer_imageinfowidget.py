# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
from numpy import radians, eye, array
from sys import float_info
import unittest
from unittest.mock import MagicMock, patch
from collections import namedtuple

from mantidqt.widgets.sliceviewer.presenters.imageinfowidget import ImageInfoWidget, ImageInfoTracker
from mantidqt.widgets.sliceviewer.models.transform import NonOrthogonalTransform

CursorInfo = namedtuple("CursorInfo", ("array", "extent", "point"))


class ImageInfoTrackerTest(unittest.TestCase):
    def setUp(self):
        self.image = MagicMock()
        self.nonortho_transform = NonOrthogonalTransform(radians(60))
        self.image_info_widget = MagicMock(ImageInfoWidget)
        self.presenter = MagicMock()
        self.presenter.get_extra_image_info_columns.return_value = {}

    def test_nonortho_transform_not_applied_when_mesh_and_transfrom_false(self):
        del self.image.get_extent  # so image treated as mesh
        tracker = ImageInfoTracker(
            image=self.image, presenter=self.presenter, transform=self.nonortho_transform, do_transform=False, widget=self.image_info_widget
        )

        tracker.on_cursor_at(1.0, 1.0)

        self.image_info_widget.cursorAt.assert_called_with(1.0, 1.0, float_info.max, {})

    def test_nonortho_transform_applied_when_mesh_and_transfrom_true(self):
        del self.image.get_extent  # so image treated as mesh
        tracker = ImageInfoTracker(
            image=self.image, presenter=self.presenter, transform=self.nonortho_transform, do_transform=True, widget=self.image_info_widget
        )

        tracker.on_cursor_at(1.0, 1.0)

        self.image_info_widget.cursorAt.assert_called_with(0.42264973081037405, 1.1547005383792517, float_info.max, {})

    @patch("mantidqt.widgets.sliceviewer.presenters.imageinfowidget.cursor_info")
    def test_nonortho_transform_not_applied_when_not_mesh(self, mock_cursorinfo):
        mock_cursorinfo.return_value = CursorInfo(
            array=2 * eye(2, dtype=float), extent=None, point=(1, 1)
        )  # output simple 2D array and slice indices
        tracker = ImageInfoTracker(
            image=self.image, presenter=self.presenter, transform=self.nonortho_transform, do_transform=True, widget=self.image_info_widget
        )

        tracker.on_cursor_at(1.0, 1.0)

        self.image_info_widget.cursorAt.assert_called_with(1.0, 1.0, 2.0, {})

    @patch("mantidqt.widgets.sliceviewer.presenters.imageinfowidget.cursor_info")
    def test_cursorAt_arguments_correct_when_not_mesh_and_not_transposed(self, mock_cursorinfo):
        underlying_array = array([[1.0, 2.0], [3.0, 4.0]])
        xdata, ydata = 0.0, 1.0  # Data on image x and y axes at cursor position
        xindex, yindex = 0, 1
        mock_cursorinfo.return_value = CursorInfo(array=underlying_array, extent=None, point=(xindex, yindex))
        image = self.image
        image.transpose = False
        tracker = ImageInfoTracker(
            image=self.image, presenter=self.presenter, transform=None, do_transform=False, widget=self.image_info_widget
        )

        tracker.on_cursor_at(xdata, ydata)
        self.image_info_widget.cursorAt.assert_called_with(xdata, ydata, underlying_array[xindex][yindex], {})

    @patch("mantidqt.widgets.sliceviewer.presenters.imageinfowidget.cursor_info")
    def test_cursorAt_arguments_correct_when_not_mesh_and_transposed(self, mock_cursorinfo):
        underlying_array = array([[1.0, 2.0], [3.0, 4.0]])
        xdata, ydata = 0.0, 1.0  # Data on image x and y axes at cursor position
        xindex, yindex = 0, 1
        mock_cursorinfo.return_value = CursorInfo(array=underlying_array, extent=None, point=(xindex, yindex))
        image = self.image
        image.transpose = True
        tracker = ImageInfoTracker(
            image=self.image, presenter=self.presenter, transform=None, do_transform=False, widget=self.image_info_widget
        )

        tracker.on_cursor_at(xdata, ydata)
        # Since the image is transposed, the x and y on the axes with respect to the data is the wrong way around,
        # but the 'signal' at this point in the underlying array is still 2.0
        self.image_info_widget.cursorAt.assert_called_with(ydata, xdata, underlying_array[xindex][yindex], {})

    @patch("mantidqt.widgets.sliceviewer.presenters.imageinfowidget.cursor_info")
    def test_cursorAt_called_with_extra_cols_specified_by_model(self, mock_cursorinfo):
        underlying_array = array([[1.0, 2.0], [3.0, 4.0]])
        xdata, ydata = 0.0, 1.0  # Data on image x and y axes at cursor position
        xindex, yindex = 0, 1
        mock_cursorinfo.return_value = CursorInfo(array=underlying_array, extent=None, point=(xindex, yindex))
        extra_cols = {"H": "1.0", "K": "2.0", "L": "0.0"}
        presenter_with_extra_cols = MagicMock()
        presenter_with_extra_cols.get_extra_image_info_columns.return_value = extra_cols
        image = self.image
        image.transpose = False
        tracker = ImageInfoTracker(
            image=self.image, presenter=presenter_with_extra_cols, transform=None, do_transform=False, widget=self.image_info_widget
        )

        tracker.on_cursor_at(xdata, ydata)
        self.image_info_widget.cursorAt.assert_called_with(xdata, ydata, underlying_array[xindex][yindex], extra_cols)

    @patch("mantidqt.widgets.sliceviewer.presenters.imageinfowidget.cursor_info")
    def test_cursorAt_called_with_extra_cols_specified_by_model_mesh(self, mock_cursorinfo):
        underlying_array = array([[1.0, 2.0], [3.0, 4.0]])
        xdata, ydata = 0.0, 1.0  # Data on image x and y axes at cursor position
        xindex, yindex = 0, 1
        mock_cursorinfo.return_value = CursorInfo(array=underlying_array, extent=None, point=(xindex, yindex))
        extra_cols = {"H": "1.0", "K": "2.0", "L": "0.0"}
        presenter_with_extra_cols = MagicMock()
        presenter_with_extra_cols.get_extra_image_info_columns.return_value = extra_cols
        del self.image.get_extent  # so image treated as mesh
        image = self.image
        image.transpose = False
        tracker = ImageInfoTracker(
            image=self.image, presenter=presenter_with_extra_cols, transform=None, do_transform=False, widget=self.image_info_widget
        )

        tracker.on_cursor_at(xdata, ydata)
        self.image_info_widget.cursorAt.assert_called_with(xdata, ydata, float_info.max, extra_cols)


if __name__ == "__main__":
    unittest.main()
