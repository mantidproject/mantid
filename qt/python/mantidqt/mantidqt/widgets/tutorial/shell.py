# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""The frame around the interface being toured: chapter tabs above it, navigation below it.

The controls live here rather than in the caption because they must not move. The caption chases
whatever is being highlighted, and a Next button that moved with it would be somewhere different on
every step - the one control the user has to click would be the one they had to look for. Here they
are in the same place all the way through, and the interface sits between them, framed.

The interface is *reparented into* this widget, so it is no longer a window of its own. That is
what lets the chrome sit outside the dimming: the overlay covers only the interface, and the tabs
and buttons stay bright and usable while everything they act on is dimmed.
"""

from qtpy.QtCore import Qt, Signal
from qtpy.QtWidgets import QFrame, QHBoxLayout, QLabel, QPushButton, QSizePolicy, QTabBar, QVBoxLayout, QWidget


class TutorialShell(QWidget):
    """Wraps ``interface`` in tutorial chrome.

    Emits a signal per control and holds no tour state: the session decides what Next means and
    tells the shell what to display.
    """

    back_requested = Signal()
    next_requested = Signal()
    #: the user picked a chapter from the tab bar
    chapter_selected = Signal(int)
    close_requested = Signal()

    def __init__(self, chapters, interface, parent=None, title=""):
        super().__init__(parent)
        self._interface = interface
        self._chapters = tuple(chapters)

        self.setWindowTitle(title or "Tutorial")
        self.setObjectName("tutorial_shell")

        self._tabs = QTabBar()
        self._tabs.setObjectName("tutorial_chapter_tabs")
        self._tabs.setExpanding(False)
        self._tabs.setDrawBase(True)
        for chapter in self._chapters:
            index = self._tabs.addTab(chapter.name)
            if chapter.description:
                self._tabs.setTabToolTip(index, chapter.description)
        self._tabs.currentChanged.connect(self._on_tab_changed)

        # the interface becomes an ordinary child widget. A QMainWindow is happy to be one; it
        # keeps its own toolbars and status bar, it just stops being a top-level window
        interface.setParent(self)
        interface.setWindowFlags(Qt.Widget)
        interface.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        self.btn_close = QPushButton("End tutorial")
        self.btn_back = QPushButton("Back")
        self.btn_next = QPushButton("Next")
        self.btn_next.setDefault(True)

        self._position_label = QLabel()
        self._position_label.setObjectName("tutorial_position")

        controls = QFrame()
        controls.setObjectName("tutorial_controls")
        controls.setFrameShape(QFrame.StyledPanel)
        controls_layout = QHBoxLayout(controls)
        controls_layout.setContentsMargins(10, 8, 10, 8)
        controls_layout.addWidget(self.btn_close)
        controls_layout.addStretch(1)
        controls_layout.addWidget(self._position_label)
        controls_layout.addSpacing(12)
        controls_layout.addWidget(self.btn_back)
        controls_layout.addWidget(self.btn_next)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._tabs)
        layout.addWidget(interface, 1)
        layout.addWidget(controls)

        self.btn_back.clicked.connect(self.back_requested)
        self.btn_next.clicked.connect(self.next_requested)
        self.btn_close.clicked.connect(self.close_requested)

    # ------------------------------------------------------------------ display

    def show_position(self, chapter_index, step_number, step_count):
        """Reflect where the tour has got to. ``step_number`` is 1-based."""
        self.set_current_chapter(chapter_index)
        self._position_label.setText(f"Step {step_number} of {step_count}")

    def set_current_chapter(self, chapter_index):
        """Move the tab bar without it reporting the move back as a user request.

        The player drives the tab as it crosses into a new chapter, and letting that echo back
        would rebuild the interface underneath a tour that was running perfectly well.
        """
        if not 0 <= chapter_index < self._tabs.count():
            return
        was_blocked = self._tabs.blockSignals(True)
        self._tabs.setCurrentIndex(chapter_index)
        self._tabs.blockSignals(was_blocked)

    def current_chapter(self):
        return self._tabs.currentIndex()

    def set_navigation_enabled(self, back=True, next_=True):
        self.btn_back.setEnabled(back)
        self.btn_next.setEnabled(next_)

    def set_busy(self, busy, message=""):
        """Disable navigation while a step is waiting on the interface.

        Pressing Next in the middle of a file search or an absorption calculation would run the
        following step's action against an interface that had not finished reacting to this one.
        """
        self.btn_back.setEnabled(not busy)
        self.btn_next.setEnabled(not busy)
        self._tabs.setEnabled(not busy)
        if message:
            self._position_label.setText(message)

    def show_finished(self, message="Tutorial complete"):
        self._position_label.setText(message)
        self.btn_next.setEnabled(False)

    # ------------------------------------------------------------------ plumbing

    def _on_tab_changed(self, index):
        self.chapter_selected.emit(index)

    def release_interface(self):
        """Let go of the toured interface so it can be closed independently.

        Called before teardown: the interface is a child, so destroying the shell first would take
        it with it and its ``closeEvent`` - which is what removes its workspaces - would never run
        the way it does for a window the user closes.
        """
        if self._interface is not None:
            self._interface.setParent(None)
            self._interface = None
