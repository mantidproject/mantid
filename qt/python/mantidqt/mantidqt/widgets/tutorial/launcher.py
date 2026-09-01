# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""Starting and owning a tutorial - the part an interface actually calls.

An interface adds a tour with ``run_tutorial``, giving it a factory for the window to tour and a
list of chapters. Everything else - the overlay, the caption, the sequencing, cleaning up
afterwards - is handled here.

**The tour drives its own copy of the interface, never the user's.** The factory builds a fresh
window, and the session closes it at the end. That is what makes a tour safe to run at any moment:
it cannot type into a form the user was halfway through, and it cannot delete their data. It also
makes starting a chapter partway through possible - the interface is rebuilt and caught up rather
than unwound.
"""

from qtpy.QtCore import QObject, QSettings, Qt, Signal
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QLabel, QListWidget, QListWidgetItem, QVBoxLayout

from mantidqt.utils.qt.qsettings_change_aware import QSettingsChangeAware
from mantidqt.widgets.tutorial.bubble import TutorialBubble
from mantidqt.widgets.tutorial.overlay import TutorialOverlay
from mantidqt.widgets.tutorial.player import TutorialPlayer

# tutorial preferences live beside the interfaces' own settings, under their own key, so an
# interface's tutorial state is stored the same way as the rest of its state
SETTINGS_GROUP = "CustomInterfaces"


# ------------------------------------------------------------------------------------------------
# "have they seen it?"
# ------------------------------------------------------------------------------------------------


def _seen_key(settings_key):
    return f"{settings_key}/tutorial_seen"


def should_show_on_startup(settings_key, settings=None):
    """Whether the tutorial has yet to be shown automatically for ``settings_key``.

    QSettings coerces a value it cannot parse to False rather than raising, which for this flag
    would mean showing the tour again to someone who has already seen it. Reading it as a string
    and comparing explicitly avoids trusting that coercion.
    """
    settings = QSettings() if settings is None else settings
    settings.beginGroup(SETTINGS_GROUP)
    try:
        stored = settings.value(_seen_key(settings_key), defaultValue="", type=str)
    finally:
        settings.endGroup()
    return str(stored).lower() != "true"


def mark_seen(settings_key, settings=None):
    """Record that the tutorial has been shown, so it does not appear unbidden again.

    Called when the tour is *offered*, not when it is completed: a user who closes it immediately
    has made their decision, and a tour that kept coming back until finished would be a nuisance.
    The toolbar button is what makes that safe - the tour stays available, it just stops
    interrupting.
    """
    settings = QSettings() if settings is None else settings
    settings.beginGroup(SETTINGS_GROUP)
    try:
        QSettingsChangeAware(settings).setValue(_seen_key(settings_key), "true")
    finally:
        settings.endGroup()


# ------------------------------------------------------------------------------------------------
# chapter picker
# ------------------------------------------------------------------------------------------------


class ChapterPicker(QDialog):
    """Lets the user choose where to start. Modal, because the tour behind it is about to be
    rebuilt underneath whatever they pick."""

    def __init__(self, chapters, parent=None, current=0):
        super().__init__(parent)
        self.setWindowTitle("Tutorial chapters")
        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)
        self.setModal(True)

        self._list = QListWidget()
        for chapter in chapters:
            item = QListWidgetItem(chapter.name)
            if chapter.description:
                item.setToolTip(chapter.description)
            self._list.addItem(item)
        self._list.setCurrentRow(max(0, min(current, len(chapters) - 1)))
        self._list.itemDoubleClicked.connect(lambda _item: self.accept())

        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)

        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("Start the tutorial from:"))
        layout.addWidget(self._list)
        layout.addWidget(buttons)

    def chosen_chapter(self):
        return self._list.currentRow()


# ------------------------------------------------------------------------------------------------
# the session
# ------------------------------------------------------------------------------------------------


class TutorialSession(QObject):
    """One run of a tutorial: the sandbox interface, its decoration, and the player driving it.

    Owns the lifecycle the player deliberately does not. Jumping to a chapter rebuilds the sandbox
    from scratch and fast-forwards to it, which is the only way to reach a chapter's starting state
    without asking every step to know how to undo itself.
    """

    finished = Signal()

    def __init__(self, sandbox_factory, chapters, parent=None, settings_key=None):
        super().__init__(parent)
        self._factory = sandbox_factory
        self._chapters = tuple(chapters)
        self._parent = parent
        self._settings_key = settings_key

        self._sandbox = None
        self._overlay = None
        self._bubble = None
        self._player = None
        self._failures = []
        self._closing = False

    @property
    def failures(self):
        """``(step label, reason)`` for every step that could not be performed. A tour that
        reports none of these is one that still matches the interface."""
        return list(self._failures)

    @property
    def player(self):
        return self._player

    @property
    def window(self):
        return None if self._sandbox is None else self._sandbox.window

    def start(self, chapter_index=0):
        self._build(chapter_index, fast_forward=chapter_index > 0)

    # ------------------------------------------------------------------ building and tearing down

    def _build(self, chapter_index, fast_forward):
        self._tear_down_sandbox()

        self._sandbox = self._factory()
        window = self._sandbox.window
        window.show()

        self._overlay = TutorialOverlay(window)
        self._bubble = TutorialBubble(window)
        self._overlay.show()
        self._bubble.show()
        self._bubble.raise_()

        self._player = TutorialPlayer(self._chapters, self._sandbox, self._overlay, self._bubble, parent=self)
        self._player.finished.connect(self._on_player_finished)
        self._player.step_failed.connect(self._on_step_failed)

        self._bubble.next_requested.connect(self._player.next_step)
        self._bubble.back_requested.connect(self._player.back_step)
        self._bubble.pause_toggled.connect(self._player.set_paused)
        self._bubble.chapters_requested.connect(self._choose_chapter)
        self._bubble.close_requested.connect(self.close)

        # the user closing the sandbox window is the same as ending the tour: there would be
        # nothing left to point at
        window.destroyed.connect(self._on_window_destroyed)

        self._player.start(chapter_index, fast_forward=fast_forward)

    def _tear_down_sandbox(self):
        if self._player is not None:
            self._player.stop()
            self._player.deleteLater()
            self._player = None
        for decoration in (self._overlay, self._bubble):
            if decoration is not None:
                if hasattr(decoration, "detach"):
                    decoration.detach()
                decoration.setParent(None)
                decoration.deleteLater()
        self._overlay = self._bubble = None
        if self._sandbox is not None:
            sandbox, self._sandbox = self._sandbox, None
            sandbox.teardown()

    # ------------------------------------------------------------------ user actions

    def _choose_chapter(self):
        if self._player is not None:
            self._player.set_paused(True)
            self._bubble.set_paused(True)
        picker = ChapterPicker(self._chapters, parent=self.window, current=self._player.position[0])
        if picker.exec_() != QDialog.Accepted:
            if self._player is not None:
                self._player.set_paused(False)
                self._bubble.set_paused(False)
            return
        chosen = picker.chosen_chapter()
        # always rebuilt, even going forwards: a chapter's steps assume the state the chapters
        # before it leave behind, and only a fresh run produces that reliably
        self._build(chosen, fast_forward=chosen > 0)

    def close(self):
        """End the tour and close the interface it was touring."""
        if self._closing:
            return
        self._closing = True
        self._tear_down_sandbox()
        self.finished.emit()

    # ------------------------------------------------------------------ player callbacks

    def _on_player_finished(self):
        if self._bubble is not None:
            self._bubble.show_step(
                "That is the end of the tutorial. Close this window to return to your own session — nothing done here has touched it.",
                title="Finished",
            )
            self._bubble.place_beside(None)
            self._bubble.set_navigation_enabled(back=True, next_=False)
        if self._overlay is not None:
            self._overlay.set_target(None)

    def _on_step_failed(self, label, reason):
        self._failures.append((label, reason))

    def _on_window_destroyed(self, *_args):
        # the window has already gone, so there is nothing left to tear down but the bookkeeping
        self._sandbox = None
        self.close()


def run_tutorial(sandbox_factory, chapters, parent=None, settings_key=None, mark_as_seen=False):
    """Start a tutorial and return its session.

    :param sandbox_factory: called with no arguments; must return an object with a ``window``
        attribute (the interface to tour, not yet shown) and a ``teardown()`` method. It is also
        the context every step receives, so put whatever the steps need to reach - the presenter,
        the model, a temporary directory - on it.
    :param chapters: the ``TutorialChapter`` list to play.
    :param parent: the widget the tour belongs to, usually the user's own interface window.
    :param settings_key: the interface's settings key, e.g. ``"TexturePlanner"``.
    :param mark_as_seen: record that the tutorial has now been offered. True when it appeared by
        itself on startup; False when the user asked for it, which should not change anything.

    Keep the returned session alive for as long as the tour runs - it owns the sandbox window, and
    letting it be collected would take the tour down with it.
    """
    if mark_as_seen and settings_key:
        mark_seen(settings_key)
    session = TutorialSession(sandbox_factory, chapters, parent=parent, settings_key=settings_key)
    session.start()
    return session
