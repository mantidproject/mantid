# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.

from mantidqt.icons import get_icon

from qtpy.QtGui import QIcon
from qtpy.QtCore import Signal, Qt
from qtpy.QtWidgets import QToolBar, QFileDialog, QMessageBox, QLabel, QSizePolicy
import matplotlib
from matplotlib.backend_bases import NavigationToolbar2
from matplotlib.backends.backend_qt5 import SubplotToolQt
from matplotlib import backend_tools

from distutils.version import LooseVersion
import os


class MantidNavigationTool:
    """
    Mantid navigation tool
    """

    def __init__(self, text, tooltip, icon=None, callback=None, checked=None, initialiser=None):
        """
        Context manager to temporarily change value of autoscale_on_update
        :param text: Name of the action
        :param tooltip: Tooltip information describing the action
        :param icon: Icon for the action (optional)
        :param callback: Callback for the action (optional)
        :param checked: Whether the action is checked or unchecked (optional)
        :param initialiser: Initialiser used the create the action, it must return an action (optional)
        """
        self.text = text
        self.tooltip = tooltip
        self.icon = icon
        self.callback = callback
        self.checked = checked
        self.initializer = initialiser

    def __iter__(self):
        return iter((self.text, self.tooltip, self.icon, self.callback, self.checked, self.initializer))


class MantidStandardNavigationTools:
    """
    Standard navigation tools.
    Cnnected to the callbacks:
    home, back, foward, pan, save_figure, zoom
    """
    HOME = MantidNavigationTool('Home', 'Reset axes limits', 'mdi.home', 'home', None)
    BACK = MantidNavigationTool('Back', 'Back to previous view', 'mdi.arrow-left', 'back', None)
    FORWARD = MantidNavigationTool('Forward', 'Forward to next view', 'mdi.arrow-right', 'forward', None)
    PAN = MantidNavigationTool('Pan', 'Pan: L-click \nStretch: R-click', 'mdi.arrow-all', 'pan', False)
    SAVE = MantidNavigationTool('Save', 'Save image file', 'mdi.content-save', 'save_figure', None)
    ZOOM = MantidNavigationTool('Zoom', 'Zoom to rectangle', 'mdi.magnify', 'zoom', False)
    CONFIGURE = MantidNavigationTool('Subplots', 'Edit subplots', 'mdi.settings', 'configure_subplots', None)
    SEPARATOR = MantidNavigationTool(None, None, None, None, None)


# Available cursor types
cursord = {
    backend_tools.cursors.MOVE: Qt.SizeAllCursor,
    backend_tools.cursors.HAND: Qt.PointingHandCursor,
    backend_tools.cursors.POINTER: Qt.ArrowCursor,
    backend_tools.cursors.SELECT_REGION: Qt.CrossCursor,
    backend_tools.cursors.WAIT: Qt.WaitCursor,
    }


class MantidNavigationToolbar(NavigationToolbar2, QToolBar):
    """
    Base class for the MantidNavigationToolbar
    """
    message = Signal(str)

    # Default tool items, may be overriden in sub classes
    toolitems = (
        MantidStandardNavigationTools.HOME,
        MantidStandardNavigationTools.PAN,
        MantidStandardNavigationTools.ZOOM,
        MantidStandardNavigationTools.SEPARATOR,
        MantidStandardNavigationTools.SAVE,
    )

    def __init__(self, canvas, parent, coordinates=True):
        """coordinates: should we show the coordinates on the right?"""
        QToolBar.__init__(self, parent)
        self.setAllowedAreas(
            Qt.TopToolBarArea | Qt.BottomToolBarArea)
        self._actions = {}  # mapping of toolitem method names to QActions.
        self.coordinates = coordinates

        for text, tooltip_text, mdi_icon, callback, checked, initializer in self.toolitems:
            if text is None:
                self.addSeparator()
            else:
                if callable(initializer):
                    a = initializer(self, text, tooltip_text, mdi_icon, callback, checked, initializer)
                elif mdi_icon:
                    a = self.addAction(get_icon(mdi_icon), text, getattr(self, callback))
                else:
                    a = self.addAction(text, getattr(self, callback))
                self._actions[callback] = a
                if checked is not None:
                    a.setCheckable(True)
                    a.setChecked(checked)
                if tooltip_text is not None:
                    a.setToolTip(tooltip_text)

        if self.coordinates:
            self.locLabel = QLabel("", self)
            self.locLabel.setAlignment(Qt.AlignRight | Qt.AlignTop)
            self.locLabel.setSizePolicy(
                QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Ignored))
            labelAction = self.addWidget(self.locLabel)
            labelAction.setVisible(True)
        NavigationToolbar2.__init__(self, canvas)

    def _init_toolbar(self):
        # Empty init_toolbar method kept for backwards compatability
        pass

    def set_message(self, s):
        self.message.emit(s)
        if self.coordinates:
            self.locLabel.setText(s)

    def set_cursor(self, cursor):
        self.canvas.setCursor(cursord[cursor])

    def draw_rubberband(self, event, x0, y0, x1, y1):
        height = self.canvas.figure.bbox.height
        y1 = height - y1
        y0 = height - y0
        rect = [int(val) for val in (x0, y0, x1 - x0, y1 - y0)]
        self.canvas.drawRectangle(rect)

    def remove_rubberband(self):
        self.canvas.drawRectangle(None)

    def zoom(self, *args):
        super().zoom(*args)
        self._update_buttons_checked()

    def pan(self, *args):
        super().pan(*args)
        self._update_buttons_checked()

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

        filename, filter = QFileDialog.getSaveFileName(
            self.canvas.parent(), "Choose a filename to save to", start,
            filters, selectedFilter)
        if filename:
            # Save dir for next time, unless empty str (i.e., use cwd).
            if startpath != "":
                matplotlib.rcParams['savefig.directory'] = (
                    os.path.dirname(filename))
            try:
                self.canvas.figure.savefig(filename)
            except Exception as e:
                QMessageBox.critical(
                    self, "Error saving file", str(e),
                    QMessageBox.Ok, QMessageBox.NoButton)

    def set_history_buttons(self):
        if LooseVersion(matplotlib.__version__) >= LooseVersion("2.1.1"):
            can_backward = self._nav_stack._pos > 0
            can_forward = self._nav_stack._pos < len(self._nav_stack._elements) - 1
            if 'back' in self._actions:
                self._actions['back'].setEnabled(can_backward)
            if 'forward' in self._actions:
                self._actions['forward'].setEnabled(can_forward)

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

    def configure_subplots(self):
        image = os.path.join(matplotlib.get_data_path(),
                             'images', 'matplotlib.png')
        dia = SubplotToolQt(self.canvas.figure, self.canvas.parent())
        dia.setWindowIcon(QIcon(image))
        dia.exec_()

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
