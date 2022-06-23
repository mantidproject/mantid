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

        # TODO half of this is bound to be deprecated soon, fix that (and stop its warning screaming)

        if not self.interactive:
            self.to_draw.set_visible(False)

        # update the eventpress and eventrelease with the resulting extents
        x1, x2, y1, y2 = self.extents
        self.eventpress.xdata = x1
        self.eventpress.ydata = y1
        xy1 = self.ax.transData.transform_point([x1, y1])
        self.eventpress.x, self.eventpress.y = xy1

        self.eventrelease.xdata = x2
        self.eventrelease.ydata = y2
        xy2 = self.ax.transData.transform_point([x2, y2])
        self.eventrelease.x, self.eventrelease.y = xy2

        if self.spancoords == 'data':
            xmin, ymin = self.eventpress.xdata, self.eventpress.ydata
            xmax, ymax = self.eventrelease.xdata, self.eventrelease.ydata
            # calculate dimensions of box or line get values in the right
            # order
        elif self.spancoords == 'pixels':
            xmin, ymin = self.eventpress.x, self.eventpress.y
            xmax, ymax = self.eventrelease.x, self.eventrelease.y
        else:
            raise ValueError('spancoords must be "data" or "pixels"')

        if xmin > xmax:
            xmin, xmax = xmax, xmin
        if ymin > ymax:
            ymin, ymax = ymax, ymin

        spanx = xmax - xmin
        spany = ymax - ymin

        xproblems = self.minspanx is not None and spanx < self.minspanx
        yproblems = self.minspany is not None and spany < self.minspany

        # check if drawn distance (if it exists) is not too small in
        # either x or y-direction
        if self.drawtype != 'none' and (xproblems or yproblems):
            for artist in self.artists:
                artist.set_visible(False)
            self.update()
            return

        # call desired function
        self.onselect(self.eventpress, self.eventrelease)
        self.update()

        return False
