# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""Runs a tour: performs each step, points at it, says what it is, and waits.

**The tour advances only when the user asks it to.** There is no timer counting down behind the
caption, because there is no interval that is right: a step someone already understands is a wait,
and a step they do not is a race. They press Next when they have finished reading.

What is still on a timer is the interface catching up. A step's action runs, the interface is given
``settle_ms`` to lay out before the spotlight is measured, and a step that waits on real work polls
for it - and between each of those the player returns to the event loop, so the interface keeps
painting and its controls keep working. A player that waited by blocking would freeze the
demonstration it is giving.

Two things follow from the tour driving a real interface:

* **Back does not undo.** It re-shows the previous step's caption and spotlight without re-running
  its action. Replaying an action would double it - pressing "Add orientation" twice adds two - and
  unwinding one is not something a step can be asked to describe. So Back is for re-reading, and
  starting a chapter over is done by rebuilding the interface, which the session owns.
* **A step that fails does not kill the tour.** The interface has moved under it, which is worth
  reporting, but stranding the user mid-tour with a dead window helps nobody. The step is skipped
  and its failure emitted.
"""

from qtpy.QtCore import QObject, QTimer, Signal

from mantidqt.widgets.tutorial.interaction import ensure_visible, process_events, wait_for
from mantidqt.widgets.tutorial.step import walk

# how long a fast-forwarded step is given to settle. Shorter than a played one because nothing is
# being read - this is only getting the interface into the state a later chapter starts from.
FAST_FORWARD_SETTLE_MS = 30


class TutorialPlayer(QObject):
    """Plays ``chapters`` against ``context``, drawing on ``overlay`` and ``bubble``.

    The player owns sequencing and nothing else. It does not build or tear down the interface it is
    touring, and it does not decide what the controls mean - a session does both, and calls
    ``next_step`` / ``back_step`` in response.
    """

    #: the whole tour reached its end
    finished = Signal()
    #: now showing (chapter index, step index)
    step_changed = Signal(int, int)
    #: a step could not be performed; carries the step's label and the reason
    step_failed = Signal(str, str)
    #: the tour is waiting on the interface and should not be advanced; carries a message
    busy_changed = Signal(bool, str)

    def __init__(self, chapters, context, overlay, bubble, parent=None):
        super().__init__(parent)
        if not chapters:
            raise ValueError("a tutorial needs at least one chapter")
        self._chapters = tuple(chapters)
        self._context = context
        self._overlay = overlay
        self._bubble = bubble

        self._chapter_index = 0
        self._step_index = 0
        self._running = False
        self._busy = False
        self._busy_message = ""
        self._pending = None  # the QTimer for whatever happens next, so it can be cancelled
        self._waiter = None  # the QTimer polling a step's await_, likewise

    # ------------------------------------------------------------------ state

    @property
    def position(self):
        return self._chapter_index, self._step_index

    @property
    def is_running(self):
        return self._running

    @property
    def is_busy(self):
        """True while a step is waiting on the interface, when advancing would run the next step's
        action against something that has not finished reacting to this one."""
        return self._busy

    def current_chapter(self):
        return self._chapters[self._chapter_index]

    def current_step(self):
        return self.current_chapter()[self._step_index]

    def at_start(self):
        return self._chapter_index == 0 and self._step_index == 0

    def at_end(self):
        return self._chapter_index == len(self._chapters) - 1 and self._step_index == len(self.current_chapter()) - 1

    # ------------------------------------------------------------------ running

    def start(self, chapter_index=0, fast_forward=False):
        """Begin at ``chapter_index``.

        With ``fast_forward``, the actions of every earlier chapter are performed first, silently.
        Chapters are cumulative - there is nothing to add an orientation to until a sample has been
        loaded - so starting partway through means catching the interface up first. It is only
        sound on a freshly built interface, which is why the session rebuilds before asking for it.
        """
        if not 0 <= chapter_index < len(self._chapters):
            raise IndexError(f"no chapter {chapter_index}; the tour has {len(self._chapters)}")
        self._running = True
        self._chapter_index = chapter_index
        self._step_index = 0

        if fast_forward and chapter_index > 0:
            self._fast_forward_to(chapter_index)
        else:
            self._run_current_step()

    def stop(self):
        """End the tour without emitting ``finished``. Safe to call at any point, including from
        inside a step."""
        self._running = False
        self._set_busy(False)
        self._cancel_pending()
        self._overlay.set_target(None)
        self._overlay.hide()

    def next_step(self):
        """Move to the next step. Ignored while the tour is waiting on the interface."""
        if self._busy:
            return
        self._cancel_pending()
        self._advance()

    def back_step(self):
        """Re-show the previous step. Does not undo anything - see the module docstring."""
        if self._busy:
            return
        self._cancel_pending()
        if self._step_index > 0:
            self._step_index -= 1
        elif self._chapter_index > 0:
            self._chapter_index -= 1
            self._step_index = len(self.current_chapter()) - 1
        else:
            return  # already at the very start
        self._narrate(self.current_step())

    # ------------------------------------------------------------------ the step cycle

    def _run_current_step(self):
        if not self._running:
            return
        step = self.current_step()

        # busy from the moment the action starts until the step is on screen. Without this, Next
        # pressed during the settle would cancel the pending narration and move on, so the step's
        # action would have run but its caption would never have been shown. No message: this lasts
        # a couple of hundred milliseconds and should not flash text into the controls.
        self._set_busy(True)

        if step.action is not None:
            try:
                step.action(self._context)
            except Exception as error:
                # the interface has moved under the tour. Worth reporting, not worth stranding the
                # user in a half-run tour for
                self.step_failed.emit(step.label, str(error))
                self._narrate(step)
                return

        if step.await_ is not None:
            self._set_busy(True, step.await_text)
            self._bubble.show_waiting(step.await_text)
            self._bubble.place_beside(self._spotlight_rect())
            self._waiter = wait_for(
                predicate=lambda: self._await_holds(step),
                on_ready=lambda: self._schedule(lambda: self._narrate(step), step.settle_ms),
                timeout_s=step.await_timeout_s,
                on_timeout=lambda: self._on_await_timeout(step),
                parent=self,
            )
        else:
            self._schedule(lambda: self._narrate(step), step.settle_ms)

    def _await_holds(self, step):
        try:
            return step.await_(self._context)
        except Exception as error:
            self.step_failed.emit(step.label, f"while waiting: {error}")
            return True

    def _on_await_timeout(self, step):
        self.step_failed.emit(step.label, f"still not ready after {step.await_timeout_s}s")
        self._narrate(step)

    def _narrate(self, step):
        """Point at the step's target and say what it is, then wait for the user.

        Deliberately separate from running the action: Back comes straight here, which is the whole
        of what makes Back safe to press.
        """
        if not self._running:
            return

        target = None
        try:
            target = step.resolve_target(self._context)
            if target is not None:
                ensure_visible(target)
        except Exception as error:
            self.step_failed.emit(step.label, f"could not find what to highlight: {error}")
            target = None

        self._overlay.set_target(target)
        self._bubble.show_step(text=step.text, title=step.title)
        self._bubble.place_beside(self._spotlight_rect())
        self._set_busy(False)
        self.step_changed.emit(self._chapter_index, self._step_index)

    def _advance(self):
        if not self._running:
            return
        if self._step_index + 1 < len(self.current_chapter()):
            self._step_index += 1
        elif self._chapter_index + 1 < len(self._chapters):
            self._chapter_index += 1
            self._step_index = 0
        else:
            self._running = False
            self.finished.emit()
            return
        self._run_current_step()

    # ------------------------------------------------------------------ fast forward

    def _fast_forward_to(self, chapter_index):
        """Run every action before ``chapter_index`` with no narration.

        Deliberately synchronous over the actions but never over a wait: a step that awaits
        something is awaited through ``wait_for`` and the rest of the fast-forward continues from
        its callback, so even this stays off the blocking path.
        """
        message = "Setting the interface up for this chapter…"
        self._set_busy(True, message)
        self._bubble.show_waiting(message)
        self._bubble.place_beside(None)
        preceding = [step for chapter_number, _step_number, _chapter, step in walk(self._chapters) if chapter_number < chapter_index]
        self._replay(iter(preceding))

    def _replay(self, steps):
        if not self._running:
            return
        step = next(steps, None)
        if step is None:
            self._set_busy(False)
            self._run_current_step()
            return

        if step.action is not None:
            try:
                step.action(self._context)
            except Exception as error:
                self.step_failed.emit(step.label, f"while setting up: {error}")
        process_events()

        if step.await_ is not None:
            self._waiter = wait_for(
                predicate=lambda: self._await_holds(step),
                on_ready=lambda: self._replay(steps),
                timeout_s=step.await_timeout_s,
                on_timeout=lambda: self._replay(steps),
                parent=self,
            )
        else:
            self._schedule(lambda: self._replay(steps), FAST_FORWARD_SETTLE_MS)

    # ------------------------------------------------------------------ plumbing

    def _set_busy(self, busy, message=""):
        if busy == self._busy and message == self._busy_message:
            return
        self._busy = busy
        self._busy_message = message
        self.busy_changed.emit(busy, message)

    def _spotlight_rect(self):
        target_rect = getattr(self._overlay, "target_rect", None)
        return target_rect() if callable(target_rect) else None

    def _schedule(self, call, delay_ms):
        """Do something after a delay, keeping the timer so it can be cancelled.

        One pending action at a time: scheduling replaces whatever was pending, so an interrupted
        settle cannot fire later and narrate a step the tour has already left.
        """
        self._cancel_pending()
        timer = QTimer(self)
        timer.setSingleShot(True)
        timer.timeout.connect(call)
        timer.start(max(0, delay_ms))
        self._pending = timer

    def _cancel_pending(self):
        for timer in (self._pending, self._waiter):
            if timer is not None:
                timer.stop()
        self._pending = None
        self._waiter = None
