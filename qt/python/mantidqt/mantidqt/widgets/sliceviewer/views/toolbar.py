# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.

from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar, MantidStandardNavigationTools, MantidNavigationTool
from qtpy.QtWidgets import QApplication
from qtpy.QtCore import Signal, QSize


class ToolItemText:
    HOME = "Home"
    PAN = "Pan"
    ZOOM = "Zoom"
    GRID = "Grid"
    LINEPLOTS = "LinePlots"
    REGIONSELECTION = "RegionSelection"
    OVERLAY_PEAKS = "OverlayPeaks"
    NONORTHOGONAL_AXES = "NonOrthogonalAxes"
    SAVE = "Save"
    NONAXISALIGNEDCUTS = "NonAxisAlignedCuts"


class SliceViewerNavigationToolbar(MantidNavigationToolbar):
    gridClicked = Signal(bool)
    homeClicked = Signal()
    linePlotsClicked = Signal(bool)
    regionSelectionClicked = Signal(bool)
    nonOrthogonalClicked = Signal(bool)
    peaksOverlayClicked = Signal(bool)
    nonAlignedCutsClicked = Signal(bool)
    zoomPanClicked = Signal(bool)
    zoomPanFinished = Signal()

    toolitems = (
        MantidNavigationTool(ToolItemText.HOME, "Reset original view", "mdi.home", "homeClicked", None),
        MantidStandardNavigationTools.PAN,
        MantidNavigationTool(ToolItemText.ZOOM, "Zoom to rectangle", "mdi.magnify", "zoom", False),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool(ToolItemText.GRID, "Toggle grid on/off", "mdi.grid", "gridClicked", False),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool(ToolItemText.LINEPLOTS, "Toggle lineplots on/off", "mdi.chart-bell-curve", "linePlotsClicked", False),
        MantidNavigationTool(
            ToolItemText.REGIONSELECTION, "Toggle region selection on/off", "mdi.vector-rectangle", "regionSelectionClicked", False
        ),
        MantidNavigationTool(ToolItemText.NONAXISALIGNEDCUTS, "Toggle cutting tool", "mdi.vector-line", "nonAlignedCutsClicked", False),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool(ToolItemText.OVERLAY_PEAKS, "Add peaks overlays on/off", "mdi.chart-bubble", "peaksOverlayClicked", None),
        MantidNavigationTool(
            ToolItemText.NONORTHOGONAL_AXES, "Toggle nonorthogonal axes on/off", "mdi.axis", "nonOrthogonalClicked", False
        ),
        MantidStandardNavigationTools.SEPARATOR,
        MantidNavigationTool(ToolItemText.SAVE, "Save the figure", "mdi.content-save", "save_figure", None),
    )

    def __init__(self, canvas, parent, coordinates=True):
        """coordinates: should we show the coordinates on the right?"""
        super().__init__(canvas, parent, coordinates)

        # Adjust icon size or they are too small in PyQt5 by default
        dpi_ratio = QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QSize(int(24 * dpi_ratio), int(24 * dpi_ratio)))

    def zoom(self, *args):
        super().zoom(*args)
        self.zoomPanClicked.emit(bool(self.mode))

    def pan(self, *args):
        super().pan(*args)
        self.zoomPanClicked.emit(bool(self.mode))

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
        if (abs(x - lastx) < 5) or (abs(y - lasty) < 5):
            return
        self.zoomPanFinished.emit()

    def release_pan(self, event):
        super().release_pan(event)
        self._release_pan_zoom_event(event)

    def release_zoom(self, event):
        super().release_zoom(event)
        self._release_pan_zoom_event(event)
