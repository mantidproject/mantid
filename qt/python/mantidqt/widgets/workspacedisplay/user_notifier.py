# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import sys
import weakref

from qtpy.QtCore import QPoint
from qtpy.QtGui import QCursor, QFont, QFontMetrics
from qtpy.QtWidgets import QToolTip


class UserNotifier(object):
    """
    Provides common functionality for notifying the user using a
    status bar, or (Windows only) mouse tooltip notifications.
    """
    NO_SELECTION_MESSAGE = "No selection"
    COPY_SUCCESSFUL_MESSAGE = "Copy Successful"
    WORKING_MESSAGE = "Operation in progress.."
    DEFAULT_TIMEOUT = 2000

    def __init__(self, status_bar):
        self._status_bar = status_bar

    @property
    def _status_bar(self):
        return self.__status_bar()

    @_status_bar.setter
    def _status_bar(self, status_bar):
        self.__status_bar = weakref.ref(status_bar)

    def show_mouse_toast(self, message):
        if not sys.platform == "win32":
            # There is no reason to draw tooltips for OSs that don't display anything
            return

        QToolTip.showText(self._get_mouse_position(), message)

    def _get_mouse_position(self):
        # Creates a text with empty space to get the height of the rendered text - this is used
        # to provide the same offset for the tooltip, scaled relative to the current resolution and zoom.
        font_metrics = QFontMetrics(QFont(" "))
        # The height itself is divided by 2 just to reduce the offset so that the tooltip is
        # reasonably positioned, relative to the cursor
        return QCursor.pos() + QPoint(font_metrics.height() / 2, 0)

    def show_status_message(self, msg):
        self._status_bar.showMessage(msg, self.DEFAULT_TIMEOUT)

    def notify_no_selection_to_copy(self):
        self.show_mouse_toast(self.NO_SELECTION_MESSAGE)
        self.show_status_message(self.NO_SELECTION_MESSAGE)

    def notify_successful_copy(self):
        self.show_mouse_toast(self.COPY_SUCCESSFUL_MESSAGE)
        self.show_status_message(self.COPY_SUCCESSFUL_MESSAGE)

    def notify_working(self):
        # only display this on the status bar
        self.show_status_message(self.WORKING_MESSAGE)
