# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""Runs a tour: points at something, explains it, and then - when asked - does it.

**Explain first, act second.** A step opens whatever it points at, spotlights it and shows its
caption, and then waits. The action runs when the user presses *Show me*, so they are reading the
explanation of a control at the moment they watch it being used. Performing it first and narrating
afterwards means describing a change that already happened while they were looking elsewhere.

**The tour advances only when the user asks it to.** No timer counts down behind the caption:
there is no interval that is right for everyone, since a step someone already understands is a
wait and a step they do not is a race.

*Next* applies the step's action first if *Show me* has not been pressed. Chapters are cumulative -
there is nothing to add an orientation to until a sample has been loaded - so a skipped action
would leave every later step describing an interface that never got into the state it assumes.

Two more things follow from the tour driving a real interface:

* **Back does not undo.** It re-shows an earlier step's caption and spotlight without re-running
  its action. Replaying one would double it - pressing "Add orientation" twice adds two - and
  unwinding it is not something a step can be asked to describe. So Back is for re-reading, and
  starting a chapter over is done by rebuilding the interface, which the session owns.
* **A step that fails does not kill the tour.** The interface has moved under it, which is worth
  reporting, but stranding the user mid-tour with a dead window helps nobody. The failure is
  emitted and the step is treated as done.
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
    ``next_step`` / ``back_step`` / ``apply_step`` in response.
    """

    #: the whole tour reached its end
    finished = Signal()
    #: now showing (chapter index, step index)
    step_changed = Signal(int, int)
    #: the current step's action has now been performed
    step_applied = Signal()
    #: a step could not be performed; carries the step's label and the reason
    step_failed = Signal(str, str)
    #: the tour is working and should not be driven; carries a message
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
        # positions whose action has already run. Kept per position rather than as a single flag
        # because Back revisits steps, and their actions must not run a second time.
        self._applied = set()
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
        """True while the tour is performing or settling, when driving it would run one step's
        action against an interface still reacting to another."""
        return self._busy

    def current_chapter(self):
        return self._chapters[self._chapter_index]

    def current_step(self):
        return self.current_chapter()[self._step_index]

    def current_step_has_action(self):
        return self.current_step().action is not None

    def is_applied(self):
        """Whether this step's action has run - vacuously true for a step that has none."""
        return not self.current_step_has_action() or self.position in self._applied

    def at_start(self):
        return self._chapter_index == 0 and self._step_index == 0

    def at_end(self):
        return self._chapter_index == len(self._chapters) - 1 and self._step_index == len(self.current_chapter()) - 1

    # ------------------------------------------------------------------ running

    def start(self, chapter_index=0, fast_forward=False):
        """Begin at ``chapter_index``.

        With ``fast_forward``, the actions of every earlier chapter are performed first, silently,
        to catch the interface up to the state the chapter assumes. It is only sound on a freshly
        built interface, which is why the session rebuilds before asking for it.
        """
        if not 0 <= chapter_index < len(self._chapters):
            raise IndexError(f"no chapter {chapter_index}; the tour has {len(self._chapters)}")
        self._running = True
        self._chapter_index = chapter_index
        self._step_index = 0

        if fast_forward and chapter_index > 0:
            self._fast_forward_to(chapter_index)
        else:
            self._present_current_step()

    def stop(self):
        """End the tour without emitting ``finished``. Safe to call at any point."""
        self._running = False
        self._set_busy(False)
        self._cancel_pending()
        self._overlay.set_target(None)
        self._overlay.hide()

    def apply_step(self):
        """Perform the current step's action, leaving its explanation on screen to be watched."""
        if self._busy:
            return
        self._perform(advance_after=False)

    def next_step(self):
        """Move on, performing this step's action first if it has not been performed yet."""
        if self._busy:
            return
        if not self.is_applied():
            self._perform(advance_after=True)
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
        self._present_current_step()

    # ------------------------------------------------------------------ presenting

    def _present_current_step(self):
        """Open up the step's target, spotlight it and show its caption. Runs no action."""
        if not self._running:
            return
        step = self.current_step()
        self._set_busy(True)
        # revealed before it is measured: a target on an unselected tab or inside a collapsed
        # section has a geometry that would put the spotlight somewhere meaningless
        self._reveal(step)
        self._schedule(lambda: self._narrate(step), step.settle_ms)

    def _narrate(self, step):
        if not self._running:
            return
        # a target the step's own action creates does not exist yet, which is why a miss is only
        # worth reporting once the action has run
        self._overlay.set_target(self._locate(step, report=self.is_applied()))
        self._bubble.show_step(text=step.text, title=step.title)
        self._bubble.place_beside(self._spotlight_rect())
        self._set_busy(False)
        self.step_changed.emit(self._chapter_index, self._step_index)

    def _locate(self, step, report=True):
        """The widget to spotlight, or None.

        ``report`` says whether a target that cannot be found is a failure. It is not while the
        step is only being explained - some targets are created by the action about to run - but it
        is once that action has happened, when a missing one means the interface has moved under
        the tour.
        """
        try:
            target = step.resolve_target(self._context)
        except Exception as error:
            if report:
                self.step_failed.emit(step.label, f"could not find what to highlight: {error}")
            return None
        if target is not None:
            try:
                ensure_visible(target)
            except Exception as error:
                self.step_failed.emit(step.label, f"could not bring the target into view: {error}")
        return target

    def _reveal(self, step):
        """Open up the step's target ahead of time, best-effort.

        Failures are swallowed on purpose: a target created *by* the step's action does not exist
        yet, which is legitimate. ``_locate`` resolves it again when the step is shown and reports
        it properly if it is still missing.
        """
        if step.target is None:
            return
        try:
            target = step.target(self._context)
            if target is not None:
                ensure_visible(target)
        except Exception:
            return

    # ------------------------------------------------------------------ performing

    def _perform(self, advance_after):
        step = self.current_step()
        if self.is_applied():
            if advance_after:
                self._advance()
            return

        self._set_busy(True)
        self._applied.add(self.position)

        try:
            step.action(self._context)
        except Exception as error:
            # the interface has moved under the tour. Worth reporting, not worth stranding the
            # user in a half-run tour for
            self.step_failed.emit(step.label, str(error))
            self._schedule(lambda: self._finish_perform(step, advance_after), 0)
            return

        if step.await_ is not None:
            self._set_busy(True, step.await_text)
            self._bubble.show_waiting(step.await_text)
            self._bubble.place_beside(self._spotlight_rect())
            self._waiter = wait_for(
                predicate=lambda: self._await_holds(step),
                on_ready=lambda: self._schedule(lambda: self._finish_perform(step, advance_after), step.settle_ms),
                timeout_s=step.await_timeout_s,
                on_timeout=lambda: self._on_await_timeout(step, advance_after),
                parent=self,
            )
        else:
            self._schedule(lambda: self._finish_perform(step, advance_after), step.settle_ms)

    def _finish_perform(self, step, advance_after):
        if not self._running:
            return
        if advance_after:
            self._set_busy(False)
            self._advance()
            return
        # the caption stays; the highlight is re-measured because the action may have moved,
        # revealed or resized what it points at
        self._overlay.set_target(self._locate(step))
        self._bubble.show_step(text=step.text, title=step.title)
        self._bubble.place_beside(self._spotlight_rect())
        self._set_busy(False)
        self.step_applied.emit()

    def _await_holds(self, step):
        try:
            return step.await_(self._context)
        except Exception as error:
            self.step_failed.emit(step.label, f"while waiting: {error}")
            return True

    def _on_await_timeout(self, step, advance_after):
        self.step_failed.emit(step.label, f"still not ready after {step.await_timeout_s}s")
        self._finish_perform(step, advance_after)

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
            self._set_busy(False)
            self.finished.emit()
            return
        self._present_current_step()

    # ------------------------------------------------------------------ fast forward

    def _fast_forward_to(self, chapter_index):
        """Run every action before ``chapter_index``, with no narration.

        Deliberately synchronous over the actions but never over a wait: a step that awaits
        something is awaited through ``wait_for`` and the rest of the fast-forward continues from
        its callback, so even this stays off the blocking path.
        """
        message = "Setting the interface up for this chapter…"
        self._set_busy(True, message)
        self._bubble.show_waiting(message)
        self._bubble.place_beside(None)
        preceding = [
            ((chapter_number, step_number), step)
            for chapter_number, step_number, _chapter, step in walk(self._chapters)
            if chapter_number < chapter_index
        ]
        self._replay(iter(preceding))

    def _replay(self, steps):
        if not self._running:
            return
        entry = next(steps, None)
        if entry is None:
            self._set_busy(False)
            self._present_current_step()
            return
        position, step = entry

        # marked applied even when it fails, so a Back into this chapter does not try it again
        self._applied.add(position)
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
