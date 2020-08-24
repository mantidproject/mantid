# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtGui
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QT as NavigationToolbar
from mantidqt.utils.observer_pattern import GenericObservable


class myToolbar(NavigationToolbar):
    # only display the buttons we need
    toolitems = [tool for tool in NavigationToolbar.toolitems if
                 tool[0] in ("Save", "Pan", "Zoom")]

    def __init__(self, *args, **kwargs):
        super(myToolbar, self).__init__(*args, **kwargs)
        self.layout().takeAt(5)  # or more than 1 if you have more buttons
        pm = QtGui.QPixmap()
        ic = QtGui.QIcon(pm)
        # self.add = self.addAction(ic, "Add line")
        self.rm = self.addAction(ic, "Remove line")
        self.rm_subplot = self.addAction(ic, "Remove subplot")
        self.uncheck_autoscale_notifier = GenericObservable()
        self.uncheck_autoscale_notifier = GenericObservable()
        self.enable_autoscale_notifier = GenericObservable()
        self.disable_autoscale_notifier = GenericObservable()

    def setAddConnection(self, slot):
        self.add.triggered.connect(slot)

    def setRmConnection(self, slot):
        self.rm.triggered.connect(slot)

    def setRmSubplotConnection(self, slot):
        self.rm_subplot.triggered.connect(slot)

    def zoom(self, *args):
        """Activate zoom to rect mode."""
        if self._active == 'ZOOM':
            self._active = None
            self.enable_autoscale_notifier.notify_subscribers()
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

        self.set_message(self.mode)

    def pan(self, *args):
        """Activate the pan/zoom tool. pan with left button, zoom with right"""
        # set the pointer icon and button press funcs to the
        # appropriate callbacks

        if self._active == 'PAN':
            self._active = None
            self.enable_autoscale_notifier.notify_subscribers()
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

        self.set_message(self.mode)
