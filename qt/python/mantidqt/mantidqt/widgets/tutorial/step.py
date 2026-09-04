# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""The units a guided tutorial is written in.

A tour is a list of ``TutorialChapter``, each a list of ``TutorialStep``. A step says what to
spotlight, what to say about it, and - because these tours drive the interface themselves rather
than asking the user to - what to do to it.

Everything a step refers to in the interface is a *callable* taking the tutorial context, never a
widget or an object name. Two reasons, and both bite in practice:

* the interesting widgets often do not exist when the tour is written down. Matplotlib canvases
  are injected into placeholder widgets after construction, and table cell widgets only appear
  once the table has rows - which, in an automated tour, is several steps in.
* a chapter can be replayed from its start (this is what ``Back`` does), and a replay happens
  against a freshly built interface. A captured widget reference would be dangling by then.

The context object itself belongs to whoever wrote the tour: the framework only passes it through.
It is whatever the sandbox factory produced, so an interface's steps can reach its presenter and
model as readily as its widgets.
"""

from dataclasses import dataclass, field
from typing import Any, Callable, NamedTuple, Optional, Sequence, Tuple

# ``settle`` default. A step's action usually triggers a repaint, a replot or a queued signal, and
# the spotlight is positioned from the target's geometry - so the geometry has to have caught up
# before it is measured.
DEFAULT_SETTLE_MS = 150


@dataclass(frozen=True)
class TutorialStep:
    """One thing the tour points at and says.

    :param text: what to tell the user. Rich text - the bubble renders it as HTML.
    :param target: ``context -> widget`` to spotlight, or None to narrate with no highlight
        (an opening or closing remark, say). Returning None is allowed and means the same.
    :param action: ``context -> None``, performed when the user presses *Show me* - that is, only
        once the step has been narrated, so they read what a control does and then watch it used.
        Must not block: see ``await_``.
    :param avoid: ``context -> widget`` (or a sequence of them) the caption must not cover.
        For a step whose effect appears somewhere other than the control it points at - ticking a
        check box while the values it changes are displayed elsewhere - so the explanation does not
        end up sitting on top of the evidence.
    :param title: short heading for the bubble. Falls back to the chapter name when empty.
    :param settle_ms: how long to let the interface repaint before the target is measured and
        spotlighted - both when the step is first shown and again once ``action`` has run.
    :param await_: ``context -> bool``, polled after ``action`` until it holds; the step is not
        reported as done until it does, and the bubble shows ``await_text`` meanwhile. This is how
        a step waits for slow work (a fit, an absorption calculation) without blocking the event
        loop and freezing the very repaints the tour is showing off.
    :param await_timeout_s: how long ``await_`` may take. Deliberately explicit whenever it is
        long: a tour that waits on something slow should say so here rather than inherit a
        generous default that would hide a hang.
    :param await_text: what the bubble says while ``await_`` is pending, so a pause that is doing
        real work does not read as the tour having frozen.
    """

    text: str
    target: Optional[Callable[[Any], Any]] = None
    action: Optional[Callable[[Any], None]] = None
    avoid: Optional[Callable[[Any], Any]] = None
    title: str = ""
    settle_ms: int = DEFAULT_SETTLE_MS
    await_: Optional[Callable[[Any], bool]] = None
    await_timeout_s: float = 10.0
    await_text: str = "Working…"

    def __post_init__(self) -> None:
        if not self.text.strip():
            raise ValueError("a tutorial step must say something; 'text' is empty")
        if self.settle_ms < 0:
            raise ValueError(f"'{self.label}' has a negative delay")
        if self.await_ is not None and self.await_timeout_s <= 0:
            raise ValueError(f"'{self.label}' waits on a predicate with a non-positive timeout")

    @property
    def label(self) -> str:
        """A name for this step in a log or a test failure. The title if it has one, otherwise the
        opening of its text, so an untitled step is still identifiable."""
        if self.title:
            return self.title
        flat = " ".join(self.text.split())
        return flat if len(flat) <= 60 else flat[:57] + "…"

    def resolve_avoid(self, context: Any) -> Tuple[Any, ...]:
        """Widgets the caption must keep clear of. Empty when the step names none.

        Unlike the target, a miss here is swallowed: keeping out of the way of something is a
        courtesy, and failing at it is not worth interrupting the tour for.
        """
        if self.avoid is None:
            return ()
        try:
            found = self.avoid(context)
        except Exception:
            return ()
        if found is None:
            return ()
        if isinstance(found, (list, tuple, set)):
            return tuple(widget for widget in found if widget is not None)
        return (found,)

    def resolve_target(self, context: Any) -> Any:
        """The widget to spotlight, or None for a step that only narrates.

        Failure is not caught here. A target that cannot be resolved means the interface has moved
        underneath the tour - a renamed ``.ui`` object, a widget built only on some code path - and
        the tour is now describing something that is not there. That should surface loudly in the
        chapter test rather than be smoothed over into a step that silently stops highlighting.
        """
        if self.target is None:
            return None
        return self.target(context)


@dataclass(frozen=True)
class TutorialChapter:
    """A named run of steps that can be played on its own.

    Chapters are the unit the user picks from and the unit ``Back`` rewinds to: a chapter is
    replayed from its first step against a freshly built interface, rather than the tour trying to
    undo actions it has taken. That is only sound if a chapter is self-contained, so a chapter
    should begin from whatever state the sandbox factory produces and not rely on an earlier
    chapter having run.
    """

    name: str
    steps: Sequence[TutorialStep] = field(default_factory=tuple)
    description: str = ""

    def __post_init__(self) -> None:
        if not self.name.strip():
            raise ValueError("a tutorial chapter must be named; it is what the user picks from")
        if not self.steps:
            raise ValueError(f"chapter '{self.name}' has no steps")
        object.__setattr__(self, "steps", tuple(self.steps))

    def __len__(self) -> int:
        return len(self.steps)

    def __getitem__(self, index: int) -> TutorialStep:
        return self.steps[index]


class Visit(NamedTuple):
    """One step, and where in the tour it was reached.

    A tuple, so ``chapter_index, step_index, chapter, step = visit`` still works - but naming the
    fields means a caller that wants two of them does not have to spell out throwaways for the
    other two.
    """

    chapter_index: int
    step_index: int
    chapter: TutorialChapter
    step: TutorialStep

    @property
    def position(self) -> Tuple[int, int]:
        """What the player calls a position: the pair that identifies this step in the tour."""
        return self.chapter_index, self.step_index


def walk(chapters: Sequence[TutorialChapter]) -> Tuple[Visit, ...]:
    """Every step in play order.

    Used by the chapter test to visit the whole tour, and by the player to catch an interface up to
    a chapter that is being jumped to.
    """
    return tuple(
        Visit(chapter_index, step_index, chapter, step)
        for chapter_index, chapter in enumerate(chapters)
        for step_index, step in enumerate(chapter.steps)
    )
