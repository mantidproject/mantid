# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.

from mantidqt.MPLwidgets import NavigationToolbar2QT
from mantidqt.icons import get_icon
from qtpy.QtCore import Signal, Qt, QSize
from qtpy.QtWidgets import QLabel, QSizePolicy


class ToolItemText:
    HOME = 'Home'
    PAN = 'Pan'
    ZOOM = 'Zoom'
    GRID = 'Grid'
    LINEPLOTS = 'LinePlots'
    OVERLAYPEAKS = 'OverlayPeaks'
    NONORTHOGONAL_AXES = 'NonOrthogonalAxes'
    SAVE = 'Save'
    CUSTOMIZE = 'Customize'


class SliceViewerNavigationToolbar(NavigationToolbar2QT):

    gridClicked = Signal(bool)
    homeClicked = Signal()
    linePlotsClicked = Signal(bool)
    nonOrthogonalClicked = Signal(bool)
    peaksOverlayClicked = Signal(bool)
    plotOptionsChanged = Signal()
    zoomPanFinished = Signal()

    toolitems = (
        (ToolItemText.HOME, 'Reset original view', 'mdi.home', 'homeClicked', None),
        (ToolItemText.PAN, 'Pan axes with left mouse, zoom with right', 'mdi.arrow-all', 'pan',
         False),
        (ToolItemText.ZOOM, 'Zoom to rectangle', 'mdi.magnify', 'zoom', False),
        (None, None, None, None, None),
        (ToolItemText.GRID, 'Toggle grid on/off', 'mdi.grid', 'gridClicked', False),
        (ToolItemText.LINEPLOTS, 'Toggle lineplots on/off', 'mdi.chart-bell-curve',
         'linePlotsClicked', False),
        (ToolItemText.OVERLAYPEAKS, 'Add peaks overlays on/off', 'mdi.chart-bubble',
         'peaksOverlayClicked', None),
        (ToolItemText.NONORTHOGONAL_AXES, 'Toggle nonorthogonal axes on/off', 'mdi.axis',
         'nonOrthogonalClicked', False),
        (None, None, None, None, None),
        (ToolItemText.SAVE, 'Save the figure', 'mdi.content-save', 'save_figure', None),
        (ToolItemText.CUSTOMIZE, 'Configure plot options', 'mdi.settings', 'edit_parameters', None),
    )

    def _init_toolbar(self):
        for text, tooltip_text, fa_icon, callback, checked in self.toolitems:
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

        # Add the x,y location widget at the right side of the toolbar
        # The stretch factor is 1 which means any resizing of the toolbar
        # will resize this label instead of the buttons.
        if self.coordinates:
            self.locLabel = QLabel("", self)
            self.locLabel.setAlignment(Qt.AlignRight | Qt.AlignTop)
            self.locLabel.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Ignored))
            labelAction = self.addWidget(self.locLabel)
            labelAction.setVisible(True)

        # Adjust icon size or they are too small in PyQt5 by default
        self.setIconSize(QSize(24, 24))

        # Location of a press event
        self._pressed_xy = None

    def edit_parameters(self):
        NavigationToolbar2QT.edit_parameters(self)
        self.plotOptionsChanged.emit()

    def press(self, event):
        """
        Called by matplotlib after a press event has been handled. Stores the location
        of the event.
        """
        self._pressed_xy = event.x, event.y

    def release(self, event):
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

    def set_action_checked(self, text: str, state: bool):
        """
        Sets the checked/unchecked state of toggle button with the given text
        :param text: Text on the action
        :param state: checked if True else it is disabled
        """
        actions = self.actions()
        for action in actions:
            if action.text() == text:
                if action.isChecked() != state:
                    action.trigger()  # ensure view reacts appropriately
