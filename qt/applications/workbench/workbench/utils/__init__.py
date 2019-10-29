# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package
from __future__ import absolute_import, unicode_literals

import platform

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication


def on_top_of_main_window(cls):
    """Class decorator that keeps a widget on top of Workbench's main window"""
    # On Windows, setting the Workbench's main window as the window's
    # parent always keeps the window on top, but still allows minimization.
    # On Ubuntu the child is NOT kept above the parent, hence we use the
    # focusWindowChanged event to bring the window back to the top when
    # the main window gets focus. This does cause a slight flicker effect
    # as the window is hidden and quickly brought back to the front. Using
    # the parent-child method at least avoids this flicker on Windows.

    # Using the Qt.WindowStaysOnTopFlag was tried, however this caused the
    # window to stay on top of all other windows, including external
    # applications. This flag could be toggled off when the application
    # was inactive, however a QWindow needs to be re-drawn when it is given
    # new flags, which again, causes a flicker.

    # Using the Qt.Tool flag, and setting the main window as the window's
    # parent, keeps the window on top. However it does not allow the window to
    # be minimized.
    def class_reference(*args, **kwargs):
        original_init = cls.__init__
        if platform.system() == 'Windows':
            from workbench.utils.windowfinder import get_main_window_widget

            def new_init(self, *args, **kwargs):
                original_init(self, *args, **kwargs)
                self.setParent(get_main_window_widget(), Qt.Window)
        else:
            from workbench.app import MAIN_WINDOW_OBJECT_NAME
            # We hook into focusWindowChanged instead of focusChanged here.
            # focusChanged returns the widget that now has focus. We can determine
            # if the widget is a child of the main window but, we do not want to
            # raise the window above all children of the main window, e.g. matrix
            # workspace display. We only want to raise the window above the main
            # window's window.

            # The window object returned here is a QtGui.QWindow https://doc.qt.io/qt-5/qwindow.html
            # which only has access to the widget whose focus is being changed to,
            # e.g. MessageDisplay, but for reasons given above we cannot use this.
            # The object name of the window appears to be the object name of the
            # window's central widget with "Window" appended to it; so we check for
            # this.
            def _on_focusWindowChanged(self, window):
                if window and MAIN_WINDOW_OBJECT_NAME + 'Window' == window.objectName():
                    self.raise_()

            setattr(cls, '_on_focusWindowChanged', _on_focusWindowChanged)

            def new_init(self, *args, **kwargs):
                original_init(self, *args, **kwargs)
                QApplication.instance().focusWindowChanged.connect(self._on_focusWindowChanged)

        setattr(cls, '__init__', new_init)
        return cls(*args, **kwargs)

    return class_reference
