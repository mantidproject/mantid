# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from matplotlib.widgets import RectangleSelector


class RectangleSelection(RectangleSelector):
    """
    This is a RectangleSelector that just overrides _release.
    """

    def _release(self, event):
        """
        Copyrights go to matplotlib, because this is a straight copy of the _release function of the RectangleSelector.
        HOWEVER, this copied overriding function behaves differently than its original, for reasons I cannot explain.
        Where the basic RectangleSelector stops reporting clicks (i.e. rectangle selectors with no width nor heights)
        after one of those, this version continue, making it possible to switch between selectors in the view.

        So for now, we use this version even if there is no rational reason I can see for it to be needed.
        """

        if not self._interactive:
            self._selection_artist.set_visible(False)

        # update the eventpress and eventrelease with the resulting extents
        x1, x2, y1, y2 = self.extents
        self._eventpress.xdata = x1
        self._eventpress.ydata = y1
        xy1 = self.ax.transData.transform([x1, y1])
        self._eventpress.x, self._eventpress.y = xy1

        self._eventrelease.xdata = x2
        self._eventrelease.ydata = y2
        xy2 = self.ax.transData.transform([x2, y2])
        self._eventrelease.x, self._eventrelease.y = xy2

        # calculate dimensions of box or line
        if self.spancoords == "data":
            spanx = abs(self._eventpress.xdata - self._eventrelease.xdata)
            spany = abs(self._eventpress.ydata - self._eventrelease.ydata)
        elif self.spancoords == "pixels":
            spanx = abs(self._eventpress.x - self._eventrelease.x)
            spany = abs(self._eventpress.y - self._eventrelease.y)
        else:
            raise ValueError('spancoords must be "data" or "pixels"')

        # check if drawn distance (if it exists) is not too small in
        # either x or y-direction
        if self.get_visible() and (
            self.minspanx is not None and spanx < self.minspanx or self.minspany is not None and spany < self.minspany
        ):
            for artist in self.artists:
                artist.set_visible(False)
            self.update()
            return

        # call desired function
        self.onselect(self._eventpress, self._eventrelease)
        self.update()

        return False

    def onmove(self, event):
        """
        Only process event if inside the axes with which the selector was init
        This fixes bug where the x/y of the event originated from the line plot axes not the colorfill axes
        @param event: the event to process
        """
        if event.inaxes is None or self.ax == event.inaxes.axes:
            super(RectangleSelection, self).onmove(event)
