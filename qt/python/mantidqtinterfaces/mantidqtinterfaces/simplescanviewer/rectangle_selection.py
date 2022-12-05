# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import warnings

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

        # matplotlib is bound to deprecate the eventpress attribute with the release of version 3.7, making it private
        # In the meantime, it gives off warnings every time it is accessed.
        # But since this feature does not work correctly at the moment, and we need this fix as a workaround, we just
        # silence the warnings and hope that this bug will be understood and solved by the time this deprecation
        # actually comes into effect.
        # This is not nearly a perennial fix, but it should hold for a year or so.

        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=DeprecationWarning)
            if not self.interactive:
                self.to_draw.set_visible(False)

            # update the eventpress and eventrelease with the resulting extents
            x1, x2, y1, y2 = self.extents
            self.eventpress.xdata = x1
            self.eventpress.ydata = y1
            xy1 = self.ax.transData.transform([x1, y1])
            self.eventpress.x, self.eventpress.y = xy1

            self.eventrelease.xdata = x2
            self.eventrelease.ydata = y2
            xy2 = self.ax.transData.transform([x2, y2])
            self.eventrelease.x, self.eventrelease.y = xy2

            # calculate dimensions of box or line
            if self.spancoords == 'data':
                spanx = abs(self.eventpress.xdata - self.eventrelease.xdata)
                spany = abs(self.eventpress.ydata - self.eventrelease.ydata)
            elif self.spancoords == 'pixels':
                spanx = abs(self.eventpress.x - self.eventrelease.x)
                spany = abs(self.eventpress.y - self.eventrelease.y)
            else:
                raise ValueError('spancoords must be "data" or "pixels"')

            # check if drawn distance (if it exists) is not too small in
            # either x or y-direction
            if (self.drawtype != 'none'
                    and (self.minspanx is not None and spanx < self.minspanx
                         or self.minspany is not None and spany < self.minspany)):
                for artist in self.artists:
                    artist.set_visible(False)
                self.update()
                return

            # call desired function
            self.onselect(self.eventpress, self.eventrelease)
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
