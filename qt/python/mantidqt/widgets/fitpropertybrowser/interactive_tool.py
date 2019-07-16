# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import QObject, Signal, Slot
from qtpy.QtWidgets import QApplication,  QInputDialog

from .markers import PeakMarker, RangeMarker
from .mouse_state_machine import StateMachine


class FitInteractiveTool(QObject):
    """
    Peak editing tool. Peaks can be added by clicking on the plot. Peak parameters can be edited with the mouse.
    """

    fit_range_changed = Signal(list)
    peak_added = Signal(int, float, float, float)
    peak_moved = Signal(int, float, float)
    peak_fwhm_changed = Signal(int, float)
    peak_type_changed = Signal(str)
    add_background_requested = Signal(str)
    add_other_requested = Signal(str)

    default_background = 'LinearBackground'

    def __init__(self, canvas, toolbar_manager, current_peak_type):
        """
        Create an instance of FitInteractiveTool.
        :param canvas: A MPL canvas to draw on.
        :param toolbar_manager: A helper object that checks and manipulates
            the state of the plot toolbar. It is necessary to disable this
            tool's editing when zoom/pan is enabled by the user.
        :param current_peak_type: A name of a peak fit function to create by default.
        """
        super(FitInteractiveTool, self).__init__()
        self.canvas = canvas
        self.toolbar_manager = toolbar_manager
        ax = canvas.figure.get_axes()[0]
        self.ax = ax
        xlim = ax.get_xlim()
        dx = (xlim[1] - xlim[0]) / 20.
        # The fitting range: [StartX, EndX]
        start_x = xlim[0] + dx
        end_x = xlim[1] - dx
        # The interactive range marker drawn on the canvas as vertical lines that represent the fitting range.
        self.fit_range = RangeMarker(canvas, 'green', start_x, end_x, 'XMinMax', '--')
        self.fit_range.range_changed.connect(self.fit_range_changed)

        # A list of interactive peak markers
        self.peak_markers = []
        # A reference to the currently selected peak marker
        self.selected_peak = None
        # A width to set to newly created peaks
        self.fwhm = dx
        # The name of the currently selected peak
        self.current_peak_type = current_peak_type
        # A cache for peak function names to use in the add function dialog
        self.peak_names = []
        # A cache for background function names to use in the add function dialog
        self.background_names = []
        # A cache for names of function that are neither peaks or backgrounds to use in the add function dialog
        self.other_names = []

        # Connect MPL events to callbacks and store connection ids in a cache
        self._cids = []
        self._cids.append(canvas.mpl_connect('draw_event', self.draw_callback))
        self._cids.append(canvas.mpl_connect('motion_notify_event', self.motion_notify_callback))
        self._cids.append(canvas.mpl_connect('button_press_event', self.button_press_callback))
        self._cids.append(canvas.mpl_connect('button_release_event', self.button_release_callback))

        # The mouse state machine that handles responses to the mouse events.
        self.mouse_state = StateMachine(self)

    def disconnect(self):
        """
        Disconnect the tool from everything
        """
        QObject.disconnect(self)
        for cid in self._cids:
            self.canvas.mpl_disconnect(cid)
        self.fit_range.remove()

    def draw_callback(self, event):
        """
        This is called at every canvas draw. Redraw the markers.
        :param event: Unused
        """
        self.fit_range.redraw()
        for pm in self.peak_markers:
            pm.redraw()

    def motion_notify_callback(self, event):
        """
        This is called when the mouse moves across the canvas
        :param event: An event object with information on the current mouse position
        """
        self.mouse_state.motion_notify_callback(event)

    def button_press_callback(self, event):
        """
        This is called when a mouse button is pressed inside the canvas
        :param event: An event object with information on the current mouse position
        """
        self.mouse_state.button_press_callback(event)

    def button_release_callback(self, event):
        """
        This is called when a mouse button is released inside the canvas
        :param event: An event object with information on the current mouse position
        """
        self.mouse_state.button_release_callback(event)

    def move_markers(self, event):
        """
        Move markers that need moving.
        :param event: A MPL mouse event.
        """
        x, y = event.xdata, event.ydata
        if x is None or y is None:
            return

        should_redraw = self.fit_range.mouse_move(x, y)
        for pm in self.peak_markers:
            should_redraw = pm.mouse_move(x, y) or should_redraw
        if should_redraw:
            self.canvas.draw()

    def start_move_markers(self, event):
        """
        Start moving markers under the mouse.
        :param event: A MPL mouse event.
        """
        x = event.xdata
        y = event.ydata
        if x is None or y is None:
            return
        self.fit_range.mouse_move_start(x, y)
        selected_peak = None
        for pm in self.peak_markers:
            pm.mouse_move_start(x, y)
            if pm.is_moving:
                selected_peak = pm
        if selected_peak is not None:
            self.select_peak(selected_peak)
            self.canvas.draw()

    def stop_move_markers(self, event):
        """
        Stop moving all markers.
        """
        self.fit_range.mouse_move_stop()
        for pm in self.peak_markers:
            pm.mouse_move_stop()

    def set_fit_range(self, start_x, end_x):
        """
        Change the fit range when it has been changed in the FitPropertyBrowser.
        :param start_x: New value of StartX
        :param end_x: New value of EndX
        """
        if start_x is not None and end_x is not None:
            self.fit_range.set_range(start_x, end_x)
            self.canvas.draw()

    def _make_peak_id(self):
        """
        Generate a new peak id. Ids of deleted markers can be reused.
        :return: An integer id that is unique among self.peak_markers.
        """
        ids = set([pm.peak_id for pm in self.peak_markers])
        n = 0
        for i in range(len(ids)):
            if i in ids:
                if i > n:
                    n = i
            else:
                return i
        return n + 1

    def add_default_peak(self):
        """
        A QAction callback. Start adding a new peak. The tool will expect the user to click on the canvas to
        where the peak should be placed.
        """
        self.mouse_state.transition_to('add_peak')

    def add_peak_dialog(self):
        """
        A QAction callback. Start a dialog to choose a peak function name. After that the tool will expect the user
        to click on the canvas to where the peak should be placed.
        """
        selected_name = QInputDialog.getItem(self.canvas, 'Fit', 'Select peak function', self.peak_names,
                                             self.peak_names.index(self.current_peak_type), False)
        if selected_name[1]:
            self.peak_type_changed.emit(selected_name[0])
            self.mouse_state.transition_to('add_peak')

    def add_background_dialog(self):
        """
        A QAction callback. Start a dialog to choose a background function name. The new function is added to the
        browser.
        """
        current_index = self.background_names.index(self.default_background)
        if current_index < 0:
            current_index = 0
        selected_name = QInputDialog.getItem(self.canvas, 'Fit', 'Select background function', self.background_names,
                                             current_index, False)
        if selected_name[1]:
            self.add_background_requested.emit(selected_name[0])

    def add_other_dialog(self):
        """
        A QAction callback. Start a dialog to choose a name of a function except a peak or a background. The new
        function is added to the browser.
        """
        selected_name = QInputDialog.getItem(self.canvas, 'Fit', 'Select function', self.other_names, 0, False)
        if selected_name[1]:
            self.add_other_requested.emit(selected_name[0])

    def add_peak_marker(self, x, y_top, y_bottom=0.0, fwhm=None):
        """
        Add a new peak marker. No signal is sent to the fit browser.
        :param x: The peak centre.
        :param y_top: The y coordinate of the top of the peak.
        :param y_bottom: The y coordinate of the bottom of the peak (background level).
        :param fwhm: A full width at half maximum. If None use the value of the FWHM of the last edited peak.
        :return: An instance of PeakMarker.
        """
        if fwhm is None:
            fwhm = self.fwhm
        peak_id = self._make_peak_id()
        peak = PeakMarker(self.canvas, peak_id, x, y_top, y_bottom, fwhm=fwhm)
        peak.peak_moved.connect(self.peak_moved)
        peak.fwhm_changed.connect(self.peak_fwhm_changed_slot)
        self.peak_markers.append(peak)
        return peak

    def add_peak(self, x, y_top, y_bottom=0.0):
        """
        Add a new peak marker and send a signal to the fit browser to add a new peak function.
        :param x: The peak centre.
        :param y_top: The y coordinate of the top of the peak.
        :param y_bottom: The y coordinate of the bottom of the peak (background level).
        """
        peak = self.add_peak_marker(x, y_top, y_bottom)
        self.select_peak(peak)
        self.canvas.draw()
        self.peak_added.emit(peak.peak_id, x, peak.height(), peak.fwhm())

    def update_peak(self, peak_id, centre, height, fwhm):
        """
        Update a peak marker.
        :param peak_id: An id of the marker to update.
        :param centre: A new peak centre.
        :param height: A new peak height.
        :param fwhm: A new peak width.
        """
        for pm in self.peak_markers:
            if pm.peak_id == peak_id:
                pm.update_peak(centre, height, fwhm)
        self.canvas.draw()

    def select_peak(self, peak):
        """
        Make a peak marker selected. Deselect all others.
        :param peak: An instance of PeakMarker to select.
        """
        self.selected_peak = None
        for pm in self.peak_markers:
            if peak == pm:
                pm.select()
                self.selected_peak = peak
            else:
                pm.deselect()

    def _get_default_height(self):
        """
        Calculate the value of the default peak height to set to peaks added by the user to the fit property browser
        directly.
        """
        ylim = self.ax.get_ylim()
        return (ylim[0] + ylim[1]) / 2

    def get_peak_list(self):
        """
        get a list of peak parameters as tuples of (id, centre, height, fwhm).
        """
        plist = []
        for pm in self.peak_markers:
            plist.append((pm.peak_id, pm.centre(), pm.height(), pm.fwhm()))
        return plist

    def update_peak_markers(self, peaks_to_keep, peaks_to_add):
        """
        Update the peak marker list.
        :param peaks_to_keep: A list of ids of the peaks that should be kept. Markers with ids not found in this list
            will be removed.
        :param peaks_to_add: Parameters of peaks to add as a list of tuples (prefix, centre, height, fwhm).
        :return: A tuple of: first item: {map of peak id -> prefix},
                             second item: a list of (prefix, centre, height, fwhm) for those added peaks that had
                                          their parameters changed and need to be updated in the fit browser.
                                          Parameters are changed if the added peak has zero height or width.
        """
        peaks_to_remove = []
        for i, pm in enumerate(self.peak_markers):
            if pm.peak_id not in peaks_to_keep:
                peaks_to_remove.append(i)
        peaks_to_remove.sort(reverse=True)
        for i in peaks_to_remove:
            self.peak_markers[i].remove()
            del self.peak_markers[i]
        peak_ids = {}
        peak_updates = []
        for prefix, c, h, w in peaks_to_add:
            do_updates = False
            if h == 0.0:
                h = self._get_default_height()
                do_updates = True
            if w <= 0:
                w = self.fwhm
                do_updates = True
            pm = self.add_peak_marker(c, h, fwhm=w)
            peak_ids[pm.peak_id] = prefix
            if do_updates:
                peak_updates.append((prefix, c, h, w))
        self.canvas.draw()
        return peak_ids, peak_updates

    @Slot(int, float)
    def peak_fwhm_changed_slot(self, peak_id, fwhm):
        """
        Respond to a peak marker changing its width.
        :param peak_id: Marker's peak id.
        :param fwhm: A new fwhm value.
        """
        self.fwhm = fwhm
        self.peak_fwhm_changed.emit(peak_id, fwhm)

    def get_transform(self):
        """
        Get the MPL transform object used to draw the markers. Used by the unit tests.
        """
        return self.fit_range.patch.get_transform()

    def add_to_menu(self, menu, peak_names, current_peak_type, background_names,
                    other_names):
        """
        Adds the fit tool menu actions to the given menu and returns the menu

        :param menu: A reference to a menu that will accept the actions
        :param peak_names: A list of registered fit function peak names to be offered to choose from by the "Add a peak"
            dialog.
        :param current_peak_type:
        :param background_names: A list of registered background fit functions to be offered to choose from by the
            "Add a background" dialog.
        :param other_names:  A list of other registered fit functions to be offered to choose from by the
            "Add other function" dialog.
        :returns: The menu reference passed in
        """
        self.peak_names = peak_names
        self.current_peak_type = current_peak_type
        self.background_names = background_names
        self.other_names = other_names
        if not self.toolbar_manager.is_tool_active():
            menu.addAction("Add peak", self.add_default_peak)
            menu.addAction("Select peak type", self.add_peak_dialog)
            menu.addAction("Add background", self.add_background_dialog)
            menu.addAction("Add other function", self.add_other_dialog)

        return menu
