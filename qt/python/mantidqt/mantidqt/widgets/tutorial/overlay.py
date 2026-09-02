# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""The dimming layer that puts one widget in the spotlight.

A transparent child of the window being toured, covering all of it, painting everything except the
current target under a scrim. It is a sibling of the widgets it dims rather than a separate
window, which is what keeps it aligned when the window is moved, resized or restacked - there is
no second window to keep in step.

It never takes input. ``WA_TransparentForMouseEvents`` means every click passes through to the
interface underneath, so the overlay cannot swallow one and the tour cannot deadlock behind its
own decoration.
"""

from qtpy.QtCore import QEvent, QRect, QRectF, Qt, QTimer
from qtpy.QtGui import QColor, QPainter, QPainterPath, QPen
from qtpy.QtWidgets import QWidget

# how rounded the spotlight is, in pixels
CORNER_RADIUS = 6

# the spotlight is opened this much wider than the target on each side, so the highlight sits
# around the widget rather than clipping its edge
PADDING = 4

# how often the target's position is re-checked. A target moves for reasons no single event filter
# catches - an ancestor's layout settling, a scroll area scrolling, a tab animating in - so the
# rectangle is re-measured rather than only recomputed when something says it changed. It is a
# cheap comparison, and repainting only happens when it actually differs.
TRACK_INTERVAL_MS = 50


class TutorialOverlay(QWidget):
    """Dims ``host`` except for one widget.

    :param host: the window being toured. The overlay makes itself a child of it and covers it.
    """

    def __init__(self, host):
        super().__init__(host)
        self._host = host
        self._target = None
        self._target_rect = QRect()

        self.setAttribute(Qt.WA_TransparentForMouseEvents, True)
        self.setAttribute(Qt.WA_NoSystemBackground, True)
        self.setFocusPolicy(Qt.NoFocus)
        self.setGeometry(host.rect())

        host.installEventFilter(self)

        self._tracker = QTimer(self)
        self._tracker.setInterval(TRACK_INTERVAL_MS)
        self._tracker.timeout.connect(self._retrack)

    # ------------------------------------------------------------------ target

    def set_target(self, widget):
        """Spotlight ``widget``, or dim the whole window when it is None.

        A target that is not a descendant of the host is refused rather than silently ignored: it
        would be mapped into coordinates that mean nothing here, and the spotlight would land in an
        arbitrary place - which looks like the tour pointing confidently at the wrong thing.
        """
        if widget is not None and not self._host.isAncestorOf(widget):
            raise ValueError(f"{widget.objectName() or widget} is not inside the window being toured")
        self._target = widget
        self._retrack()
        if widget is None:
            self._tracker.stop()
        else:
            self._tracker.start()

    def target(self):
        return self._target

    def target_rect(self):
        """Where the spotlight currently is, in overlay coordinates. Empty when there is none."""
        return QRect(self._target_rect)

    def rect_of(self, widget):
        """Where ``widget`` sits in this overlay's coordinates, or an empty rect.

        Public because the caption needs the same mapping for the widgets it has been asked to keep
        clear of, and they are not the spotlight.
        """
        if widget is None or self._host is None or not widget.isVisible():
            return QRect()
        if widget is not self._host and not self._host.isAncestorOf(widget):
            return QRect()
        top_left = widget.mapTo(self._host, widget.rect().topLeft())
        return QRect(top_left, widget.size())

    def _measure(self):
        rect = self.rect_of(self._target)
        return rect if rect.isEmpty() else rect.adjusted(-PADDING, -PADDING, PADDING, PADDING)

    def _retrack(self):
        measured = self._measure()
        if measured != self._target_rect:
            self._target_rect = measured
            self.update()

    # ------------------------------------------------------------------ staying in place

    def eventFilter(self, watched, event):
        if watched is self._host and event.type() in (QEvent.Resize, QEvent.Show):
            self.setGeometry(self._host.rect())
            self._retrack()
        return super().eventFilter(watched, event)

    def show(self):
        # the overlay is created before the widgets it dims, and Qt stacks later siblings on top,
        # so it has to be raised every time it is shown or it would be painted under them
        self.setGeometry(self._host.rect())
        super().show()
        self.raise_()
        if self._target is not None:
            self._tracker.start()

    def hide(self):
        self._tracker.stop()
        super().hide()

    def detach(self):
        """Stop tracking and let go of the host. Safe to call more than once."""
        self._tracker.stop()
        if self._host is not None:
            self._host.removeEventFilter(self)
            self._host = None
        self._target = None
        self.hide()

    # ------------------------------------------------------------------ painting

    def paintEvent(self, _event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)

        scrim = QPainterPath()
        scrim.addRect(QRectF(self.rect()))

        spotlight = self._target_rect
        if not spotlight.isEmpty():
            cut_out = QPainterPath()
            cut_out.addRoundedRect(QRectF(spotlight), CORNER_RADIUS, CORNER_RADIUS)
            # subtracting rather than painting the scrim in four strips around the target: this
            # gives one path, so the rounded corners stay clean and there are no seams
            scrim = scrim.subtracted(cut_out)

        painter.fillPath(scrim, self._scrim_colour())

        if not spotlight.isEmpty():
            painter.setPen(QPen(self._highlight_colour(), 2))
            painter.setBrush(Qt.NoBrush)
            painter.drawRoundedRect(QRectF(spotlight), CORNER_RADIUS, CORNER_RADIUS)

    def _scrim_colour(self):
        # dark enough to push the rest of the interface back without hiding it - the user should
        # still see the context the highlighted widget sits in
        return QColor(0, 0, 0, 130)

    def _highlight_colour(self):
        # the palette's highlight, so the spotlight matches whatever theme the user is running
        colour = QColor(self.palette().highlight().color())
        colour.setAlpha(230)
        return colour
