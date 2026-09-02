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

        # Both labels sit at the top of whatever height they are given, and each takes only the
        # height its text needs. Without this a wrapped QLabel is centred in its slice of the box
        # and any spare height is split between the two, which reads as uneven padding above and
        # below the caption - and pushes the heading away from the top edge.
        for label in (self._title_label, self._text_label):
            label.setAlignment(Qt.AlignLeft | Qt.AlignTop)
            policy = label.sizePolicy()
            policy.setVerticalPolicy(QSizePolicy.Minimum)
            # kept, not replaced: clearing this flag is what makes a layout size a wrapped label
            # for a single line and hand the rest of the height to the stretch below it
            policy.setHeightForWidth(True)
            label.setSizePolicy(policy)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(14, 12, 14, 12)
        layout.setSpacing(6)
        layout.addWidget(self._title_label)
        layout.addWidget(self._text_label)
        # spare height goes here rather than between the labels, so the heading stays put
        layout.addStretch(1)

    # ------------------------------------------------------------------ content

    def show_step(self, text, title=""):
        self._title_label.setText(title)
        self._title_label.setVisible(bool(title))
        self._text_label.setText(text)
        self._fit_to_contents()

    def show_waiting(self, message):
        """Replace the caption with a note that the tour is waiting on the interface.

        Without this a step that runs a real calculation looks like the tour having frozen, which
        is the moment a user gives up on it.
        """
        self._text_label.setText(message)
        self._fit_to_contents()

    def _fit_to_contents(self):
        """Make the box exactly as tall as its text, at the fixed width.

        Added up from the labels rather than taken from ``adjustSize``: a word-wrapped ``QLabel``
        reports a size hint that assumes a single line, so the box comes out a different height
        from the text it holds - and the leftover is what shows up as uneven padding around the
        caption. ``heightForWidth`` is the number that accounts for the wrapping, and asking each
        label for its own is exact where asking the layout is not.
        """
        layout = self.layout()
        margins = layout.contentsMargins()
        available = WIDTH - margins.left() - margins.right()

        height = margins.top() + margins.bottom()
        showing = [label for label in (self._title_label, self._text_label) if label.isVisibleTo(self)]
        for label in showing:
            height += label.heightForWidth(available) if label.hasHeightForWidth() else label.sizeHint().height()
        height += layout.spacing() * max(len(showing) - 1, 0)

        # fixed rather than resized: a word-wrapped label's minimumSizeHint is its *unwrapped*
        # height, which is taller than the text actually needs, and a plain resize cannot go below
        # it - leaving exactly the unused strip this method exists to remove
        self.setFixedHeight(height)

    # ------------------------------------------------------------------ placement

    def place_beside(self, spotlight, keep_clear=()):
        """Move next to ``spotlight`` (in host coordinates), or to the centre when it is empty.

        Below the spotlight is preferred because that is where the eye goes next; above, then
        beside, are the fallbacks for a target with no room under it.

        ``keep_clear`` are further rectangles the caption must not cover - whatever the step is
        asking the user to watch. A step that ticks a check box while the change it causes appears
        somewhere else entirely would otherwise be explained by a caption sitting on top of the
        evidence. Each candidate position is tried in turn and the first that fouls nothing wins;
        if none is clean the preferred one is used anyway, because a caption slightly in the way is
        better than one off the edge of the window.
        """
        self._fit_to_contents()
        size = self.size()
        host_rect = self._host.rect()
        blocked = [rect for rect in keep_clear if rect is not None and not rect.isEmpty()]

        if spotlight is None or spotlight.isEmpty():
            centred = QPoint(
                host_rect.center().x() - size.width() // 2,
                host_rect.center().y() - size.height() // 2,
            )
            self.move(self._clamped(centred, size, host_rect))
            self.raise_()
            return

        placed = None
        for candidate in self._candidates(spotlight, size, host_rect):
            position = self._clamped(candidate, size, host_rect)
            rect = QRect(position, size)
            if placed is None:
                placed = position  # the preferred position, used if nothing is clean
            if not rect.intersects(spotlight) and not any(rect.intersects(other) for other in blocked):
                placed = position
                break

        self.move(placed)
        self.raise_()

    @staticmethod
    def _candidates(spotlight, size, host_rect):
        """Where the caption could go, best first: below, above, right, left."""
        centred_x = spotlight.center().x() - size.width() // 2
        centred_y = spotlight.center().y() - size.height() // 2
        return (
            QPoint(centred_x, spotlight.bottom() + GAP),
            QPoint(centred_x, spotlight.top() - GAP - size.height()),
            QPoint(spotlight.right() + GAP, centred_y),
            QPoint(spotlight.left() - GAP - size.width(), centred_y),
        )

    @staticmethod
    def _clamped(top_left, size, host_rect):
        x = max(host_rect.left() + GAP, min(top_left.x(), host_rect.right() - size.width() - GAP))
        y = max(host_rect.top() + GAP, min(top_left.y(), host_rect.bottom() - size.height() - GAP))
        return QPoint(x, y)

    def geometry_in_host(self):
        """Where the bubble is, for a caller that needs to keep something clear of it."""
        return QRect(self.pos(), self.size())
