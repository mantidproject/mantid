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

# a floor for the tutorial window, for an interface whose own size hint is tiny. Below this the
# chapter tabs and the navigation start competing with the interface for room.
MIN_WIDTH = 900
MIN_HEIGHT = 600


class TutorialShell(QWidget):
    """Wraps ``interface`` in tutorial chrome.

    Emits a signal per control and holds no tour state: the session decides what Next means and
    tells the shell what to display.
    """

    back_requested = Signal()
    next_requested = Signal()
    #: perform the current step's action, with its explanation still on screen
    apply_requested = Signal()
    #: the user picked a chapter from the tab bar
    chapter_selected = Signal(int)
    close_requested = Signal()

    def __init__(self, chapters, interface, parent=None, title=""):
        super().__init__(parent)
        self._interface = interface
        self._chapters = tuple(chapters)

        # Qt.Window is what makes this a window of its own. A QWidget given a parent is otherwise a
        # *child* of it, and would be drawn as a small panel in the corner of the interface that
        # launched the tour rather than as the window the tour lives in. Keeping the parent is
        # still right: it is what keeps the tutorial in front of the window it belongs to and takes
        # it away with it.
        self.setWindowFlags(Qt.Window)
        self.setWindowTitle(title or "Tutorial")
        self.setObjectName("tutorial_shell")

        # the interface arrives sized by its own .ui file; the shell has to be at least that big or
        # the whole point of framing a real interface is lost to scrollbars and clipping
        interface_size = interface.size()

        self._tabs = self._build_tabs()
        controls = self._build_controls()
        self._adopt(interface)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._tabs)
        layout.addWidget(interface, 1)
        layout.addWidget(controls)

        self.resize(
            max(interface_size.width(), MIN_WIDTH),
            max(interface_size.height() + self._tabs.sizeHint().height() + controls.sizeHint().height(), MIN_HEIGHT),
        )

    def _build_tabs(self):
        """The chapter tabs along the top."""
        tabs = QTabBar()
        tabs.setObjectName("tutorial_chapter_tabs")
        tabs.setExpanding(False)
        tabs.setDrawBase(True)
        for chapter in self._chapters:
            index = tabs.addTab(chapter.name)
            hint = "Click again to start this chapter over."
            tabs.setTabToolTip(index, f"{chapter.description}\n{hint}" if chapter.description else hint)
        tabs.currentChanged.connect(self._on_tab_changed)
        # clicking the chapter already showing restarts it. ``currentChanged`` does not fire for
        # the current tab, and a chapter jump is a rebuild - which makes "start this chapter again"
        # the one form of undo the tour can offer that is always exactly right
        tabs.tabBarClicked.connect(self._on_tab_clicked)
        return tabs

    def _build_controls(self):
        """The navigation row along the bottom, and the step counter it shares."""
        self.btn_close = QPushButton("End tutorial")
        self.btn_back = QPushButton("Back")
        self.btn_apply = QPushButton("Show me")
        self.btn_apply.setToolTip("Perform this step in the interface")
        self.btn_next = QPushButton("Next")
        self.btn_next.setDefault(True)

        self._position_label = QLabel()
        self._position_label.setObjectName("tutorial_position")
        # what the label says when nothing is being waited for. Kept because a busy message takes
        # the label over, and the step the tour is on has to come back when the wait ends
        self._position_text = ""

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
        controls_layout.addWidget(self.btn_apply)
        controls_layout.addWidget(self.btn_next)

        self.btn_back.clicked.connect(self.back_requested)
        self.btn_apply.clicked.connect(self.apply_requested)
        self.btn_next.clicked.connect(self.next_requested)
        self.btn_close.clicked.connect(self.close_requested)
        return controls

    def _adopt(self, interface):
        """Take the interface in as an ordinary child widget.

        A QMainWindow is happy to be one; it keeps its own toolbars and status bar, it just stops
        being a top-level window.
        """
        interface.setParent(self)
        interface.setWindowFlags(Qt.Widget)
        interface.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

    # ------------------------------------------------------------------ display

    def show_position(self, chapter_index, step_number, step_count):
        """Reflect where the tour has got to. ``step_number`` is 1-based."""
        self.set_current_chapter(chapter_index)
        self._show_position_text(f"Step {step_number} of {step_count}")

    def _show_position_text(self, text):
        self._position_text = text
        self._position_label.setText(text)

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

    def set_action_available(self, has_action, applied):
        """Offer *Show me* for a step that does something, until it has been done.

        Hidden rather than disabled for a step with no action - a permanently dead button beside
        the two live ones reads as something being broken. Once used it stays visible but disabled,
        so the row does not reflow under the pointer between pressing it and pressing Next.
        """
        self.btn_apply.setVisible(has_action)
        self.btn_apply.setEnabled(has_action and not applied)
        self.btn_apply.setText("Done" if has_action and applied else "Show me")

    def set_busy(self, busy, message=""):
        """Disable navigation while a step is waiting on the interface.

        Pressing Next in the middle of a file search or an absorption calculation would run the
        following step's action against an interface that had not finished reacting to this one.

        A message borrows the step counter's label rather than adding a second one, so it has to be
        given back: a wait that ended would otherwise leave "Working…" sitting where the position
        belongs until the user moved on.
        """
        self.btn_back.setEnabled(not busy)
        self.btn_next.setEnabled(not busy)
        self.btn_apply.setEnabled(not busy and self.btn_apply.text() == "Show me")
        self._tabs.setEnabled(not busy)
        self._position_label.setText(message if busy and message else self._position_text)

    def show_finished(self, message="Tutorial complete"):
        self._show_position_text(message)
        self.btn_next.setEnabled(False)
        self.btn_apply.setVisible(False)

    # ------------------------------------------------------------------ plumbing

    def closeEvent(self, event):
        """Closing the window ends the tour, exactly as *End tutorial* does.

        Without this the two are not the same thing at all: ``close`` on a widget hides it rather
        than destroying it, so the session - which is watching for the window being *destroyed* -
        would hear nothing, and the interface being toured would be left alive behind a hidden
        window, with its workspaces still in the ADS.
        """
        self.close_requested.emit()
        super().closeEvent(event)

    def _on_tab_changed(self, index):
        self.chapter_selected.emit(index)

    def _on_tab_clicked(self, index):
        if index == self._tabs.currentIndex():
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
