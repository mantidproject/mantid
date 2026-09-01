# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""What the tutorial says, and the controls for driving it.

A child of the window being toured rather than of the overlay: the overlay refuses mouse events
so that clicks reach the interface, and buttons inside something transparent to the mouse would
be unclickable. Being a sibling of the overlay also means it can be raised above it, so the
caption is never dimmed by the scrim it sits on.

It positions itself out of the way of whatever is being spotlighted - below it if there is room,
otherwise above, otherwise beside - and always inside the window, because a caption half off the
edge of the screen is worse than one in an awkward place.
"""

from qtpy.QtCore import QPoint, QRect, Qt, Signal
from qtpy.QtWidgets import QFrame, QHBoxLayout, QLabel, QPushButton, QSizePolicy, QVBoxLayout

# the caption is a fixed width so that text does not reflow as the tour moves between steps, which
# is distracting to read
WIDTH = 380

# kept this far from the spotlight and from the window's edges
GAP = 12


class TutorialBubble(QFrame):
    """The tutorial's caption panel, with its navigation buttons.

    Emits a signal per control and holds no tour state of its own - the player decides what
    ``Next`` means and tells the bubble what to display.
    """

    back_requested = Signal()
    next_requested = Signal()
    chapters_requested = Signal()
    close_requested = Signal()
    pause_toggled = Signal(bool)

    def __init__(self, host):
        super().__init__(host)
        self._host = host

        self.setObjectName("tutorial_bubble")
        self.setFrameShape(QFrame.StyledPanel)
        self.setAutoFillBackground(True)
        self.setFixedWidth(WIDTH)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Minimum)

        self._chapter_label = QLabel()
        self._chapter_label.setObjectName("tutorial_bubble_chapter")
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

        self.btn_chapters = QPushButton("Chapters…")
        self.btn_back = QPushButton("Back")
        self.btn_pause = QPushButton("Pause")
        self.btn_pause.setCheckable(True)
        self.btn_next = QPushButton("Next")
        self.btn_close = QPushButton("End tour")

        buttons = QHBoxLayout()
        buttons.addWidget(self.btn_chapters)
        buttons.addWidget(self.btn_close)
        buttons.addStretch(1)
        buttons.addWidget(self.btn_back)
        buttons.addWidget(self.btn_pause)
        buttons.addWidget(self.btn_next)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(14, 12, 14, 12)
        layout.setSpacing(6)
        layout.addWidget(self._chapter_label)
        layout.addWidget(self._title_label)
        layout.addWidget(self._text_label)
        layout.addSpacing(4)
        layout.addLayout(buttons)

        self.btn_chapters.clicked.connect(self.chapters_requested)
        self.btn_back.clicked.connect(self.back_requested)
        self.btn_next.clicked.connect(self.next_requested)
        self.btn_close.clicked.connect(self.close_requested)
        self.btn_pause.toggled.connect(self._on_pause_toggled)

    # ------------------------------------------------------------------ content

    def show_step(self, text, title="", chapter_name="", step_number=0, step_count=0):
        """Display one step. ``step_number`` is 1-based; pass 0 for either count to hide the
        progress line (a status message that is not a step of its own)."""
        self._title_label.setText(title)
        self._title_label.setVisible(bool(title))
        self._text_label.setText(text)
        if chapter_name and step_number and step_count:
            self._chapter_label.setText(f"{chapter_name} — step {step_number} of {step_count}")
        else:
            self._chapter_label.setText(chapter_name)
        self._chapter_label.setVisible(bool(self._chapter_label.text()))
        self.adjustSize()

    def show_waiting(self, message):
        """Replace the caption with a note that the tour is waiting on the interface.

        Without this a step that runs a real calculation looks like the tour having frozen, which
        is the moment a user gives up on it.
        """
        self._text_label.setText(message)
        self.adjustSize()

    def set_navigation_enabled(self, back=True, next_=True):
        self.btn_back.setEnabled(back)
        self.btn_next.setEnabled(next_)

    def set_paused(self, paused):
        """Reflect the player's state without emitting ``pause_toggled`` back at it."""
        was_blocked = self.btn_pause.blockSignals(True)
        self.btn_pause.setChecked(paused)
        self.btn_pause.setText("Resume" if paused else "Pause")
        self.btn_pause.blockSignals(was_blocked)

    def _on_pause_toggled(self, paused):
        self.btn_pause.setText("Resume" if paused else "Pause")
        self.pause_toggled.emit(paused)

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
