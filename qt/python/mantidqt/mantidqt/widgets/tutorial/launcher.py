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

from qtpy.QtCore import QObject, QSettings, Signal

from mantidqt.utils.qt.qsettings_change_aware import QSettingsChangeAware
from mantidqt.widgets.tutorial.annotator import TutorialAnnotator
from mantidqt.widgets.tutorial.player import TutorialPlayer
from mantidqt.widgets.tutorial.shell import TutorialShell

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
# the session
# ------------------------------------------------------------------------------------------------


class TutorialSession(QObject):
    """One run of a tutorial: the sandbox interface, its decoration, and the player driving it.

    Owns the lifecycle the player deliberately does not. Jumping to a chapter rebuilds the sandbox
    from scratch and fast-forwards to it, which is the only way to reach a chapter's starting state
    without asking every step to know how to undo itself.
    """

    finished = Signal()

    def __init__(self, sandbox_factory, chapters, parent=None, settings_key=None, title=""):
        super().__init__(parent)
        self._factory = sandbox_factory
        self._chapters = tuple(chapters)
        self._parent = parent
        self._settings_key = settings_key
        self._title = title

        self._sandbox = None
        self._shell = None
        self._annotator = None
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
    def annotator(self):
        """The spotlight and caption, wherever the tour is currently pointing."""
        return self._annotator

    @property
    def shell(self):
        """The tutorial window: the interface framed by the chapter tabs and navigation."""
        return self._shell

    @property
    def window(self):
        """The interface being toured. Its window is the shell around it - see ``shell``."""
        return None if self._sandbox is None else self._sandbox.window

    def start(self, chapter_index=0):
        self._build(chapter_index, fast_forward=chapter_index > 0)

    # ------------------------------------------------------------------ building and tearing down

    def _build(self, chapter_index, fast_forward):
        geometry = self._shell.geometry() if self._shell is not None else None
        self._tear_down_sandbox()

        self._sandbox = self._factory()
        interface = self._sandbox.window

        # the shell takes the interface as a child, so it must be built before anything is shown -
        # the overlay measures against an interface that is already in its final place
        self._shell = TutorialShell(self._chapters, interface, parent=self._parent, title=self._title)
        if geometry is not None:
            # a chapter jump rebuilds everything; reusing the geometry keeps the window where the
            # user put it instead of snapping back on every tab click
            self._shell.setGeometry(geometry)
        self._shell.show()

        # one annotator, however many windows the tour ends up pointing into - a settings dialog
        # is a window of its own, and its widgets cannot be spotlighted from the interface's overlay
        self._annotator = TutorialAnnotator(interface, parent=self)
        self._annotator.show()

        self._player = TutorialPlayer(self._chapters, self._sandbox, self._annotator, parent=self)
        self._player.finished.connect(self._on_player_finished)
        self._player.step_failed.connect(self._on_step_failed)
        self._player.step_changed.connect(self._on_step_changed)
        self._player.step_applied.connect(self._on_step_applied)
        self._player.busy_changed.connect(self._on_busy_changed)

        self._shell.next_requested.connect(self._player.next_step)
        self._shell.back_requested.connect(self._player.back_step)
        self._shell.apply_requested.connect(self._player.apply_step)
        self._shell.chapter_selected.connect(self._on_chapter_selected)
        self._shell.close_requested.connect(self.close)

        # the user closing the tutorial window is the same as ending the tour
        self._shell.destroyed.connect(self._on_window_destroyed)

        self._shell.set_current_chapter(chapter_index)
        self._player.start(chapter_index, fast_forward=fast_forward)

    def _tear_down_sandbox(self):
        if self._player is not None:
            self._player.stop()
            self._player.deleteLater()
            self._player = None
        if self._annotator is not None:
            self._annotator.detach()
            self._annotator.deleteLater()
            self._annotator = None
        if self._shell is not None:
            shell, self._shell = self._shell, None
            # stop listening to the shell before discarding it. It reports both being closed and
            # being destroyed as the end of the tour, and on a chapter jump - which tears the old
            # shell down to build a new one - either would arrive after the replacement exists and
            # take the whole session down with it
            shell.blockSignals(True)
            try:
                shell.destroyed.disconnect(self._on_window_destroyed)
            except (RuntimeError, TypeError):
                pass  # already disconnected, or the shell is gone
            # hand the interface back before the shell goes, or destroying the shell would take its
            # child with it and the closeEvent that cleans the interface up would never run
            shell.release_interface()
            shell.close()
            shell.deleteLater()
        if self._sandbox is not None:
            sandbox, self._sandbox = self._sandbox, None
            sandbox.teardown()

    # ------------------------------------------------------------------ user actions

    def _on_chapter_selected(self, chapter_index):
        # always rebuilt, even going forwards: a chapter's steps assume the state the chapters
        # before it leave behind, and only a fresh run produces that reliably
        self._build(chapter_index, fast_forward=chapter_index > 0)

    def close(self):
        """End the tour and close the interface it was touring."""
        if self._closing:
            return
        self._closing = True
        self._tear_down_sandbox()
        self.finished.emit()

    # ------------------------------------------------------------------ player callbacks

    def _on_step_changed(self, chapter_index, step_index):
        if self._shell is None:
            return
        chapter = self._chapters[chapter_index]
        self._shell.show_position(chapter_index, step_index + 1, len(chapter))
        self._shell.set_navigation_enabled(
            back=not self._player.at_start(),
            next_=True,
        )
        self._shell.set_action_available(self._player.current_step_has_action(), self._player.is_applied())

    def _on_step_applied(self):
        if self._shell is not None:
            self._shell.set_action_available(self._player.current_step_has_action(), self._player.is_applied())

    def _on_busy_changed(self, busy, message):
        if self._shell is not None:
            self._shell.set_busy(busy, message)

    def _on_player_finished(self):
        if self._annotator is not None:
            self._annotator.set_target(None)
            self._annotator.show_step(
                "That is the end of the tutorial. Close this window to return to your own session — nothing done here has touched it. "
                "Use the tabs above to revisit a chapter.",
                title="Finished",
            )
            self._annotator.place_beside(None)
        if self._shell is not None:
            self._shell.show_finished()

    def _on_step_failed(self, label, reason):
        self._failures.append((label, reason))

    def _on_window_destroyed(self, *_args):
        # the window has already gone, so there is nothing left to tear down but the bookkeeping
        self._shell = None
        self._sandbox = None
        self.close()


def run_tutorial(sandbox_factory, chapters, parent=None, settings_key=None, mark_as_seen=False, title=""):
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
    :param title: window title for the tutorial.

    Keep the returned session alive for as long as the tour runs - it owns the sandbox window, and
    letting it be collected would take the tour down with it.
    """
    if mark_as_seen and settings_key:
        mark_seen(settings_key)
    session = TutorialSession(sandbox_factory, chapters, parent=parent, settings_key=settings_key, title=title)
    session.start()
    return session
