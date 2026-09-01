# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""The caption that sits beside whatever is being spotlighted.

Only a caption. It says what the highlighted widget is and nothing else: the controls for driving
the tour live in the shell around the interface (see ``shell.py``), which stays still. A panel that
carried the buttons *and* moved to follow the highlight would put Next somewhere different on every
step, so the one thing the user has to click would be the one thing they had to hunt for.

A child of the window being toured rather than of the overlay, because the overlay refuses mouse
events so clicks reach the interface, and it is raised above it so the caption is never dimmed by
the scrim it sits on.
"""

from qtpy.QtCore import QPoint, QRect, Qt
from qtpy.QtWidgets import QFrame, QLabel, QSizePolicy, QVBoxLayout

# a fixed width so the text does not reflow as the tour moves between steps, which is distracting
# to read
WIDTH = 380

# kept this far from the spotlight and from the window's edges
GAP = 12


class TutorialBubble(QFrame):
    """Shows one step's title and explanation, positioned clear of the spotlight."""

    def __init__(self, host):
        super().__init__(host)
        self._host = host

        self.setObjectName("tutorial_bubble")
        self.setFrameShape(QFrame.StyledPanel)
        self.setAutoFillBackground(True)
        self.setFixedWidth(WIDTH)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Minimum)

        self._title_label = QLabel()
        self._title_label.setObjectName("tutorial_bubble_title")
        self._title_label.setWordWrap(True)
        title_font = self._title_label.font()
        title_font.setBold(True)
        self._title_label.setFont(title_font)

        self._text_label = QLabel()
        self._text_label.setObjectName("tutorial_bubble_text")
        self._text_label.setWordWrap(True)
        self._text_label.setTextFormat(Qt.RichText)
        # steps explain what a control does, and some of that explanation lives in the docs
        self._text_label.setOpenExternalLinks(True)
        self._text_label.setTextInteractionFlags(Qt.TextBrowserInteraction)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(14, 12, 14, 12)
        layout.setSpacing(6)
        layout.addWidget(self._title_label)
        layout.addWidget(self._text_label)

    # ------------------------------------------------------------------ content

    def show_step(self, text, title=""):
        self._title_label.setText(title)
        self._title_label.setVisible(bool(title))
        self._text_label.setText(text)
        self.adjustSize()

    def show_waiting(self, message):
        """Replace the caption with a note that the tour is waiting on the interface.

        Without this a step that runs a real calculation looks like the tour having frozen, which
        is the moment a user gives up on it.
        """
        self._text_label.setText(message)
        self.adjustSize()

    # ------------------------------------------------------------------ placement

    def place_beside(self, spotlight):
        """Move next to ``spotlight`` (in host coordinates), or to the centre when it is empty.

        Below the spotlight is preferred because that is where the eye goes next; above is the
        fallback, and beside is the last resort for a target that spans the height of the window.
        Whatever is chosen is then clamped inside the host - the caption being readable matters
        more than it being perfectly placed.
        """
        self.adjustSize()
        size = self.size()
        host_rect = self._host.rect()

        if spotlight is None or spotlight.isEmpty():
            centred = QPoint(
                host_rect.center().x() - size.width() // 2,
                host_rect.center().y() - size.height() // 2,
            )
            self.move(self._clamped(centred, size, host_rect))
            self.raise_()
            return

        below = spotlight.bottom() + GAP
        above = spotlight.top() - GAP - size.height()
        if below + size.height() <= host_rect.bottom():
            top_left = QPoint(spotlight.center().x() - size.width() // 2, below)
        elif above >= host_rect.top():
            top_left = QPoint(spotlight.center().x() - size.width() // 2, above)
        else:
            right = spotlight.right() + GAP
            fits_right = right + size.width() <= host_rect.right()
            x = right if fits_right else spotlight.left() - GAP - size.width()
            top_left = QPoint(x, spotlight.center().y() - size.height() // 2)

        self.move(self._clamped(top_left, size, host_rect))
        self.raise_()

    @staticmethod
    def _clamped(top_left, size, host_rect):
        x = max(host_rect.left() + GAP, min(top_left.x(), host_rect.right() - size.width() - GAP))
        y = max(host_rect.top() + GAP, min(top_left.y(), host_rect.bottom() - size.height() - GAP))
        return QPoint(x, y)

    def geometry_in_host(self):
        """Where the bubble is, for a caller that needs to keep something clear of it."""
        return QRect(self.pos(), self.size())
