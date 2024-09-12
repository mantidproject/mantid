# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
"""Provides a mixin to add zoom on scroll capability to a figure canvas"""


def _noop():
    """Do nothing on calling"""
    pass


class ScrollZoomMixin:
    """
    Mixin class to add zoom on scolling mouse wheel capability to a
    matplotlib axes on a figure canvas
    """

    def enable_zoom_on_scroll(self, axes, base_scale: float = 1.2, redraw: bool = True, toolbar=None, callback=None):
        """
        Connect scroll events so that they zoom in/out of a canvas
        :param axes: A matplotlib.Axes instance that will receive the scroll events
        :param base_scale: Fraction of width between axes and event point limit to increase/decrease zoom level.
                           Default=1.2 as experimentally it seems to give quite smooth scrolling without jumping too much
        :param redraw: If True then force a canvas redraw after zooming, else it is assumed this will be dealt with
                       by an external entity
        :param toolbar: An optional toolbar instance. If present the navstack is updated before the zoom takes place
        :param callback: Optional callback called when zoom completes
        """
        callback = callback if callback is not None else _noop

        def zoom_fun(event):
            if event.inaxes != axes:
                return

            # get the current x and y limits
            cur_xlim = axes.get_xlim()
            cur_ylim = axes.get_ylim()

            xdata = event.xdata  # get event x location
            ydata = event.ydata  # get event y location

            # Get distance from the cursor to the edge of the figure frame
            x_left = xdata - cur_xlim[0]
            x_right = cur_xlim[1] - xdata
            y_top = ydata - cur_ylim[0]
            y_bottom = cur_ylim[1] - ydata

            if event.button == "up":
                # deal with zoom in
                scale_factor = 1 / base_scale
            elif event.button == "down":
                # deal with zoom out
                scale_factor = base_scale
            else:
                # deal with something that should never happen
                scale_factor = 1

            if toolbar:
                toolbar.push_current()
            # set new limits
            axes.set_xlim([xdata - x_left * scale_factor, xdata + x_right * scale_factor])
            axes.set_ylim([ydata - y_top * scale_factor, ydata + y_bottom * scale_factor])
            callback()
            if redraw:
                axes.figure.canvas.draw_idle()  # force re-draw the next time the GUI refreshes

        # attach the call back
        self.scroll_cid = self.mpl_connect("scroll_event", zoom_fun)

    def disable_zoom_on_scroll(self):
        """
        Disconnect from the scoll event if it as been attached. If it has not
        been attached then this is a noop.
        """
        try:
            self.mpl_disconnect(self.cid)
            del self.cid
        except AttributeError:
            pass
