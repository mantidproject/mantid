# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.

from mantidqt.icons import get_icon
from qtpy.QtCore import Signal, Qt, QSize
from qtpy.QtWidgets import QLabel, QSizePolicy, QToolBar, QFileDialog, QMessageBox
from matplotlib.backend_bases import NavigationToolbar2
from matplotlib import backend_tools
import matplotlib
import os


class ToolItemText:
    HOME = 'Home'
    PAN = 'Pan'
    ZOOM = 'Zoom'
    GRID = 'Grid'
    LINEPLOTS = 'LinePlots'
    REGIONSELECTION = 'RegionSelection'
    OVERLAY_PEAKS = 'OverlayPeaks'
    NONORTHOGONAL_AXES = 'NonOrthogonalAxes'
    SAVE = 'Save'


# Available cursor types
cursord = {
    backend_tools.cursors.MOVE: Qt.SizeAllCursor,
    backend_tools.cursors.HAND: Qt.PointingHandCursor,
    backend_tools.cursors.POINTER: Qt.ArrowCursor,
    backend_tools.cursors.SELECT_REGION: Qt.CrossCursor,
    backend_tools.cursors.WAIT: Qt.WaitCursor,
    }


class SliceViewerNavigationToolbar(NavigationToolbar2, QToolBar):

    gridClicked = Signal(bool)
    homeClicked = Signal()
    linePlotsClicked = Signal(bool)
    regionSelectionClicked = Signal(bool)
    nonOrthogonalClicked = Signal(bool)
    peaksOverlayClicked = Signal(bool)
    zoomPanClicked = Signal(bool)
    zoomPanFinished = Signal()

    sliceviewer_toolitems = (
        (ToolItemText.HOME, 'Reset original view', 'mdi.home', 'homeClicked', None),
        (ToolItemText.PAN, 'Pan axes with left mouse, zoom with right', 'mdi.arrow-all', 'pan',
         False),
        (ToolItemText.ZOOM, 'Zoom to rectangle', 'mdi.magnify', 'zoom', False),
        (None, None, None, None, None),
        (ToolItemText.GRID, 'Toggle grid on/off', 'mdi.grid', 'gridClicked', False),
        (None, None, None, None, None),
        (ToolItemText.LINEPLOTS, 'Toggle lineplots on/off', 'mdi.chart-bell-curve',
         'linePlotsClicked', False),
        (ToolItemText.REGIONSELECTION, 'Toggle region selection on/off', 'mdi.vector-rectangle',
         'regionSelectionClicked', False),
        (None, None, None, None, None),
        (ToolItemText.OVERLAY_PEAKS, 'Add peaks overlays on/off', 'mdi.chart-bubble',
         'peaksOverlayClicked', None),
        (ToolItemText.NONORTHOGONAL_AXES, 'Toggle nonorthogonal axes on/off', 'mdi.axis',
         'nonOrthogonalClicked', False),
        (None, None, None, None, None),
        (ToolItemText.SAVE, 'Save the figure', 'mdi.content-save', 'save_figure', None)
    )

    def __init__(self, canvas, parent, coordinates=True):
        """coordinates: should we show the coordinates on the right?"""
        QToolBar.__init__(self, parent)
        self.setAllowedAreas(
            Qt.TopToolBarArea | Qt.BottomToolBarArea)

        self.coordinates = coordinates
        self._actions = {}  # mapping of toolitem method names to QActions.

        for text, tooltip_text, fa_icon, callback, checked in self.sliceviewer_toolitems:
            if text is None:
                self.addSeparator()
            else:
                if fa_icon:
                    a = self.addAction(get_icon(fa_icon), text, getattr(self, callback))
                else:
                    a = self.addAction(text, getattr(self, callback))
                self._actions[callback] = a
                if checked is not None:
                    a.setCheckable(True)
                    a.setChecked(checked)
                if tooltip_text is not None:
                    a.setToolTip(tooltip_text)

        # Add the (x, y) location widget at the right side of the toolbar
        # The stretch factor is 1 which means any resizing of the toolbar
        # will resize this label instead of the buttons.
        if self.coordinates:
            self.locLabel = QLabel("", self)
            self.locLabel.setAlignment(
                Qt.AlignRight | Qt.AlignVCenter)
            self.locLabel.setSizePolicy(
                QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Ignored))
            labelAction = self.addWidget(self.locLabel)
            labelAction.setVisible(True)

        # Adjust icon size or they are too small in PyQt5 by default
        self.setIconSize(QSize(24, 24))

        # Location of a press event
        self._pressed_xy = None

        NavigationToolbar2.__init__(self, canvas)

    def _init_toolbar(self):
        # Empty init_toolbar method kept for backwards compatability
        pass

    def _get_mode(self):
        if hasattr(self, 'name'):
            return self.mode.name
        else:
            return self.mode

    def _update_buttons_checked(self):
        # sync button checkstates to match active mode
        if 'pan' in self._actions:
            self._actions['pan'].setChecked(self._get_mode() == 'PAN' or self._get_mode() == 'pan/zoom'  )
        if 'zoom' in self._actions:
            self._actions['zoom'].setChecked(self._get_mode()  == 'ZOOM' or self._get_mode() == 'zoom rect' )

    def set_cursor(self, cursor):
        self.canvas.setCursor(cursord[cursor])

    def zoom(self, *args):
        super().zoom(*args)
        self._update_buttons_checked()
        self.zoomPanClicked.emit(bool(self.mode))

    def pan(self, *args):
        super().pan(*args)
        self._update_buttons_checked()
        self.zoomPanClicked.emit(bool(self.mode))

    def save_figure(self, *args):
        filetypes = self.canvas.get_supported_filetypes_grouped()
        sorted_filetypes = sorted(filetypes.items())
        default_filetype = self.canvas.get_default_filetype()

        startpath = os.path.expanduser(
            matplotlib.rcParams['savefig.directory'])
        start = os.path.join(startpath, self.canvas.get_default_filename())
        filters = []
        selectedFilter = None
        for name, exts in sorted_filetypes:
            exts_list = " ".join(['*.%s' % ext for ext in exts])
            filter = '%s (%s)' % (name, exts_list)
            if default_filetype in exts:
                selectedFilter = filter
            filters.append(filter)
        filters = ';;'.join(filters)

        fname, filter = QFileDialog.getSaveFileName(
            self.canvas.parent(), "Choose a filename to save to", start,
            filters, selectedFilter)
        if fname:
            # Save dir for next time, unless empty str (i.e., use cwd).
            if startpath != "":
                matplotlib.rcParams['savefig.directory'] = (
                    os.path.dirname(fname))
            try:
                self.canvas.figure.savefig(fname)
            except Exception as e:
                QMessageBox.critical(
                    self, "Error saving file", str(e),
                    QMessageBox.Ok, QMessageBox.NoButton)

    def _press_pan_zoom_event(self, event):
        """
        Called by matplotlib after a press event has been handled. Stores the location
        of the event.
        """
        self._pressed_xy = event.x, event.y

    def press_pan(self, event):
        super().press_pan(event)
        self._press_pan_zoom_event(event)

    def press_zoom(self, event):
        super().press_zoom(event)
        self._press_pan_zoom_event(event)

    def _release_pan_zoom_event(self, event):
        """
        Called when a zoom/pan event has completed. Mouse must move more than 5 pixels
        to be consider a pan/zoom ending
        """
        if self._pressed_xy is None:
            # this "should" never happen as the mouse press callback should have been
            # called first
            return

        x, y = event.x, event.y
        lastx, lasty = self._pressed_xy
        if ((abs(x - lastx) < 5) or (abs(y - lasty) < 5)):
            return
        self.zoomPanFinished.emit()

    def release_pan(self, event):
        super().release_pan(event)
        self._release_pan_zoom_event(event)

    def release_zoom(self, event):
        super().release_zoom(event)
        self._release_pan_zoom_event(event)

    def set_action_enabled(self, text: str, state: bool):
        """
        Sets the enabled/disabled state of action with the given text
        :param text: Text on the action
        :param state: Enabled if True else it is disabled
        """
        actions = self.actions()
        for action in actions:
            if action.text() == text:
                if action.isChecked() and not state:
                    action.trigger()  # ensure view reacts appropriately
                action.setEnabled(state)

    def set_action_checked(self, text: str, state: bool, trigger: bool = True):
        """
        Sets the checked/unchecked state of toggle button with the given text
        :param text: Text on the action
        :param state: checked if True else it is disabled
        :param trigger: If true the action is triggered if the state changes,
                        else the state changes only
        """
        actions = self.actions()
        for action in actions:
            if action.text() == text:
                if action.isChecked() != state:
                    if trigger:
                        action.trigger()  # ensure view reacts appropriately
                    else:
                        action.setChecked(state)

    def draw_rubberband(self, event, x0, y0, x1, y1):
        height = self.canvas.figure.bbox.height
        y1 = height - y1
        y0 = height - y0
        rect = [int(val) for val in (x0, y0, x1 - x0, y1 - y0)]
        self.canvas.drawRectangle(rect)

    def remove_rubberband(self):
        self.canvas.drawRectangle(None)
