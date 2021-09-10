# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObservable
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar, MantidStandardNavigationTools, MantidNavigationTool
from qtpy import QtCore, QtWidgets


class PlotToolbar(MantidNavigationToolbar):

    toolitems = (
        MantidStandardNavigationTools.HOME,
        MantidStandardNavigationTools.BACK,
        MantidStandardNavigationTools.FORWARD,
        MantidStandardNavigationTools.SEPARATOR,
        MantidStandardNavigationTools.PAN,
        MantidStandardNavigationTools.ZOOM,
        MantidStandardNavigationTools().SEPARATOR,
        MantidNavigationTool('Show major','Show major gridlines','mdi.grid-large','show_major_gridlines'),
        MantidNavigationTool('Show minor','Show minor gridlines','mdi.grid','show_minor_gridlines' ),
        MantidStandardNavigationTools.SEPARATOR,
        MantidStandardNavigationTools.CONFIGURE,
        MantidStandardNavigationTools.SAVE,
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool('Show/hide legend', 'Toggles the legend on/off', None, 'toggle_legend'),
                 )

    def __init__(self, figure_canvas, parent=None):

        super().__init__(figure_canvas, parent)

        self.is_major_grid_on = False
        self.is_minor_grid_on = False
        self.uncheck_autoscale_notifier = GenericObservable()
        self.enable_autoscale_notifier = GenericObservable()
        self.disable_autoscale_notifier = GenericObservable()
        self.range_changed_notifier = GenericObservable()

        # Adjust icon size or they are too small in PyQt5 by default
        dpi_ratio = QtWidgets.QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QtCore.QSize(24 * dpi_ratio, 24 * dpi_ratio))

    def toggle_legend(self):
        for ax in self.canvas.figure.get_axes():
            if ax.get_legend() is not None:
                ax.get_legend().set_visible(not ax.get_legend().get_visible())
        self.canvas.figure.tight_layout()
        self.canvas.draw()

    def show_major_gridlines(self):
        if self.is_major_grid_on:
            for ax in self.canvas.figure.get_axes():
                ax.grid(False)
            self.is_major_grid_on = False
        else:
            for ax in self.canvas.figure.get_axes():
                ax.grid(True , color='black')
            self.is_major_grid_on = True
        self.canvas.draw()

    def show_minor_gridlines(self):
        if self.is_minor_grid_on:
            for ax in self.canvas.figure.get_axes():
                ax.grid(which='minor')
            self.is_minor_grid_on = False
        else:
            for ax in self.canvas.figure.get_axes():
                ax.minorticks_on()
                ax.grid(which='minor')
            self.is_minor_grid_on = True
        self.canvas.draw()

    def reset_gridline_flags(self):
        self.is_minor_grid_on = False
        self.is_major_grid_on = False

    def zoom(self, *args):
        """Activate zoom to rect mode."""
        if self._active == 'ZOOM':
            self._active = None
            self.enable_autoscale_notifier.notify_subscribers()
            self.range_changed_notifier.notify_subscribers()

        else:
            self.uncheck_autoscale_notifier.notify_subscribers()
            self.disable_autoscale_notifier.notify_subscribers()
            self._active = 'ZOOM'

        if self._idPress is not None:
            self._idPress = self.canvas.mpl_disconnect(self._idPress)
            self.mode = ''

        if self._idRelease is not None:
            self._idRelease = self.canvas.mpl_disconnect(self._idRelease)
            self.mode = ''

        if self._active:
            self._idPress = self.canvas.mpl_connect('button_press_event',
                                                    self.press_zoom)
            self._idRelease = self.canvas.mpl_connect('button_release_event',
                                                      self.release_zoom)
            self.mode = 'zoom rect'
            self.canvas.widgetlock(self)
        else:
            self.canvas.widgetlock.release(self)

        for axes in self.canvas.figure.get_axes():
            axes.set_navigate_mode(self._active)

        self._update_buttons_checked()
        self.set_message(self.mode)

    def pan(self, *args):
        """Activate the pan/zoom tool. pan with left button, zoom with right"""
        # set the pointer icon and button press funcs to the
        # appropriate callbacks

        if self._active == 'PAN':
            self._active = None
            self.enable_autoscale_notifier.notify_subscribers()
            self.range_changed_notifier.notify_subscribers()

        else:
            self.uncheck_autoscale_notifier.notify_subscribers()
            self.disable_autoscale_notifier.notify_subscribers()
            self._active = 'PAN'
        if self._idPress is not None:
            self._idPress = self.canvas.mpl_disconnect(self._idPress)
            self.mode = ''

        if self._idRelease is not None:
            self._idRelease = self.canvas.mpl_disconnect(self._idRelease)
            self.mode = ''

        if self._active:
            self._idPress = self.canvas.mpl_connect(
                'button_press_event', self.press_pan)
            self._idRelease = self.canvas.mpl_connect(
                'button_release_event', self.release_pan)
            self.mode = 'pan/zoom'
            self.canvas.widgetlock(self)
        else:
            self.canvas.widgetlock.release(self)

        for axes in self.canvas.figure.get_axes():
            axes.set_navigate_mode(self._active)

        self._update_buttons_checked()
        self.set_message(self.mode)

    def home(self, *args):
        """Restore the original view."""
        self._nav_stack.home()
        self.set_history_buttons()
        self._update_view()
