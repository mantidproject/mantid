from collections import namedtuple
from functools import lru_cache
from typing import Optional

from matplotlib.image import AxesImage
from matplotlib.transforms import Bbox, BboxTransform
import numpy as np

# Data type to store information related to a cursor over an image
CursorInfo = namedtuple("CursorInfo", ("array", "extent", "point", "data"))


@lru_cache(maxsize=32)
def cursor_info(image: AxesImage, xdata: float, ydata: float, full_bbox: Bbox = None) -> Optional[CursorInfo]:
    """Return information on the image for the given position in
    data coordinates.
    :param image: An instance of an image type
    :param xdata: X data coordinate of cursor
    :param ydata: Y data coordinate of cursor
    :param full_bbox: Bbox of full workspace dimension to use for transforming mouse position
    :return: None if point is not valid on the image else return CursorInfo type
    """
    extent = image.get_extent()
    xmin, xmax, ymin, ymax = extent
    arr = image.get_array()
    data_extent = Bbox([[ymin, xmin], [ymax, xmax]])
    array_extent = Bbox([[0, 0], arr.shape[:2]])
    if full_bbox is None:
        trans = BboxTransform(boxin=data_extent, boxout=array_extent)
    else:
        # If the view is zoomed in and the slice is changed, then the image extents
        # and data extents change. This causes the cursor to be transformed to the
        # wrong point for certain MDH workspaces (since it cannot be dynamically rebinned).
        # This will use the full WS data dimensions to do the transformation
        trans = BboxTransform(boxin=full_bbox, boxout=array_extent)
    point = trans.transform_point([ydata, xdata])
    if any(np.isnan(point)):
        return None

    point = point.astype(int)
    if 0 <= point[0] <= arr.shape[0] and 0 <= point[1] <= arr.shape[1]:
        return CursorInfo(array=arr, extent=extent, point=point, data=(xdata, ydata))
    else:
        return None


def make_selector_class(base):
    def in_axes_event(self, event):
        """
        Only process event if inside the axes with which the selector was init
        This fixes bug where the x/y of the event originated from the line plot axes not the colorfill axes
        """
        return event.inaxes is None or self.ax == event.inaxes.axes

    def invalid_first_event(self, event):
        """
        Do not process the first event if there is no x/ydata.
        This fixes bug where the mpl tries to access the previous event given the lack of x/yata, which is None
        """
        return (not event.xdata or not event.ydata) and not self._prev_event

    def onmove(self, event):
        if in_axes_event(self, event) and not invalid_first_event(self, event):
            super(SelectorMtd, self).onmove(event)

    SelectorMtd = type("SelectorMtd", (base,), {})
    SelectorMtd.onmove = onmove
    return SelectorMtd
