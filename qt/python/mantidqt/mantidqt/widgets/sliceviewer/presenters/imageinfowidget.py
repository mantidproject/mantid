# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
# std imports
from typing import Union
import sys
import numpy as np

from mantidqt.utils.qt import import_qt
from matplotlib.collections import QuadMesh
from matplotlib.image import AxesImage
from matplotlib.transforms import Bbox

from .lineplots import CursorTracker, cursor_info
from mantidqt.widgets.sliceviewer.models.transform import NonOrthogonalTransform

# Constants
DBLMAX = sys.float_info.max

ImageInfoWidget = import_qt(".._common", "mantidqt.widgets", "ImageInfoWidget")
ImageInfoWidgetMini = import_qt(".._common", "mantidqt.widgets", "ImageInfoWidgetMini")


class ImageInfoTracker(CursorTracker):
    def __init__(
        self,
        image: Union[AxesImage, QuadMesh],
        presenter,
        transform: NonOrthogonalTransform,
        do_transform: bool,
        widget: ImageInfoWidget,
        cursor_transform: tuple = None,
    ):
        """
        Update the image that the widget refers too.
        :param: An AxesImage or Mesh instance to track
        :param: transpose_xy: If true the cursor position should be transposed
                before sending to the table update
        :param do_transform: Flag to perform transform for QuadMesh images
        :param widget: ImageInfoWidget instance
        :param cursor_transform: Full axes limits to use for mouse coord transform to use instead of image extents
        """
        super().__init__(image_axes=image.axes, autoconnect=False)
        self._image = image
        self.transform = transform
        self.do_transform = do_transform
        self._widget = widget
        self._cursor_transform = None
        if cursor_transform is not None:
            self._cursor_transform = Bbox(
                [[cursor_transform[1][0], cursor_transform[0][0]], [cursor_transform[1][1], cursor_transform[0][1]]]
            )

        if hasattr(image, "get_extent"):
            self.on_cursor_at = self._on_cursor_at_axesimage
        else:
            self.on_cursor_at = self._on_cursor_at_mesh
        self._presenter = presenter

    def on_cursor_outside_axes(self):
        """Update the image table given the mouse has moved out of the image axes"""
        extra_cols = self._presenter.get_extra_image_info_columns(DBLMAX, DBLMAX)
        self._widget.cursorAt(DBLMAX, DBLMAX, DBLMAX, extra_cols)

    # private api

    def _on_cursor_at_axesimage(self, xdata: float, ydata: float):
        """
        Update the image table for the given coordinates given an AxesImage
        object.
        :param xdata: X coordinate of cursor in data space
        :param ydata: Y coordinate of cursor in data space
        """
        if self._image is None:
            return
        cinfo = cursor_info(self._image, xdata, ydata, full_bbox=self._cursor_transform)
        if cinfo is not None:
            arr = cinfo.array
            i, j = cinfo.point
            if (0 <= i < arr.shape[0]) and (0 <= j < arr.shape[1]) and not np.ma.is_masked(arr[i, j]):
                extra_cols = self._presenter.get_extra_image_info_columns(xdata, ydata)
                if (not self._cursor_transform) and self._image.transpose:
                    # Only do this for MatrixWorkspace where ImageInfoWidget requires x to be TOF, y to be spectrum #
                    xdata, ydata = ydata, xdata
                self._widget.cursorAt(xdata, ydata, arr[i, j], extra_cols)

    def _on_cursor_at_mesh(self, xdata: float, ydata: float):
        """
        Update the image table for the given coordinates for a mesh object.
        This simply updates the position coordinates in the same fashion as
        the standard matplotlib system as looking up the signal is not yet
        supported.
        :param xdata: X coordinate of cursor in data space
        :param ydata: Y coordinate of cursor in data space
        """
        if self._image is None:
            return
        if self.do_transform:
            xdata, ydata = self.transform.inv_tr(xdata, ydata)
        extra_cols = self._presenter.get_extra_image_info_columns(xdata, ydata)
        self._widget.cursorAt(xdata, ydata, DBLMAX, extra_cols)
