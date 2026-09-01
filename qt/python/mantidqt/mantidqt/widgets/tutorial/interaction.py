# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""Driving a live interface from code, for a guided tutorial.

Most of this exists because the naive Qt call does not do what you want:

* ``setText`` on a ``QLineEdit`` emits none of the signals typing into it does, so an interface
  that reacts to ``editingFinished`` never notices the value,
* ``setCurrentText`` on a non-editable combo silently does nothing when the text is not there,
* a spin box clamps to its range and rounds to its decimals without saying so, and
* a widget on an unselected tab, inside a collapsed group box, or scrolled out of view has a
  geometry that is meaningless to point at.

``Testing/AutomatedUITests/qt_interaction_helpers.py`` solves the same problems for tests and is
where most of this came from. What is *not* shared with it is the waiting: those helpers wait by
blocking (``QTest.qWait``), which is right for a test and wrong here. A tutorial that blocks stops
repainting, so the overlay it is drawing freezes along with everything else. ``wait_for`` below is
the non-blocking equivalent, and it is the only way this package should ever wait.

Nothing here imports ``QtTest``. Driving a widget through its own API is enough for a tutorial,
which - unlike a test - does not need to prove that a real mouse press would have worked.
"""

import time

from qtpy.QtCore import QEventLoop, QTimer
from qtpy.QtWidgets import (
    QAbstractButton,
    QAbstractScrollArea,
    QApplication,
    QGroupBox,
    QLineEdit,
    QScrollArea,
    QStackedWidget,
    QTabWidget,
    QWidget,
)

# how often ``wait_for`` re-runs its predicate. The event loop keeps running throughout, so this
# only sets the polling granularity, not the responsiveness of the interface.
POLL_INTERVAL_MS = 100


# --------------------------------------------------------------------------------------------
# waiting
# --------------------------------------------------------------------------------------------


def wait_for(predicate, on_ready, timeout_s, on_timeout=None, interval_ms=POLL_INTERVAL_MS, parent=None):
    """Call ``on_ready`` once ``predicate()`` holds, without blocking.

    The point of this over a wait loop: everything a tutorial shows - the spotlight, the caption,
    the interface reacting - is painted by the event loop, so the tutorial must return to it
    between checks rather than sit in a loop pumping it by hand.

    ``timeout_s`` has no default on purpose. How long a step may wait depends entirely on what it
    is waiting for, and a default would only ever be wrong in the direction that hides a hang.
    ``on_timeout`` decides what that means: it raises by default, and a caller that would rather
    carry on (the player, which moves the tour along rather than stranding the user) passes its
    own.

    Returns the ``QTimer`` doing the polling, so a caller that may be torn down mid-wait can stop
    it. Keep a reference to it: an unparented timer with no Python reference can be collected, and
    the wait would then simply never finish. Passing ``parent`` avoids that.
    """
    if timeout_s <= 0:
        raise ValueError(f"timeout_s must be positive, got {timeout_s}")

    if predicate():
        on_ready()
        return None

    timer = QTimer(parent)
    timer.setInterval(interval_ms)
    deadline = time.monotonic() + timeout_s

    def poll():
        try:
            ready = predicate()
        except Exception:
            timer.stop()
            raise
        if ready:
            timer.stop()
            on_ready()
        elif time.monotonic() > deadline:
            timer.stop()
            if on_timeout is None:
                raise RuntimeError(f"timed out after {timeout_s}s waiting for {getattr(predicate, '__doc__', None) or predicate}")
            on_timeout()

    timer.timeout.connect(poll)
    timer.start()
    return timer


# --------------------------------------------------------------------------------------------
# making a widget worth pointing at
# --------------------------------------------------------------------------------------------


def ancestors(widget):
    """Every widget above ``widget``, innermost first."""
    found = []
    parent = widget.parentWidget()
    while parent is not None:
        found.append(parent)
        parent = parent.parentWidget()
    return found


def ensure_visible(widget):
    """Do whatever it takes to bring ``widget`` into view, and report whether it worked.

    A tutorial points at things, so a target that is on an unselected tab, inside a collapsed
    group box, or scrolled past is not merely awkward - its geometry is stale or empty, and the
    spotlight would be drawn somewhere meaningless. This reveals it the way a user would: select
    the tab it is on, expand the section it is in, scroll to it.

    Revealing runs outermost-first, because an inner tab bar does not have a usable geometry until
    the outer tab holding it has been selected. Scrolling is left until last for the same reason:
    where a widget sits inside a scroll area is only known once everything around it is shown.
    """
    chain = list(reversed(ancestors(widget)))
    scroll_areas = []

    for ancestor in chain:
        if isinstance(ancestor, QTabWidget):
            _select_page_containing(ancestor, widget, ancestor.setCurrentIndex)
        elif isinstance(ancestor, QStackedWidget):
            _select_page_containing(ancestor, widget, ancestor.setCurrentIndex)
        elif isinstance(ancestor, QGroupBox) and ancestor.isCheckable() and not ancestor.isChecked():
            # collapsible sections are built this way throughout Mantid's interfaces: the group box
            # is checkable and its contents are hidden while it is unchecked
            ancestor.setChecked(True)
        elif isinstance(ancestor, QAbstractScrollArea):
            scroll_areas.append(ancestor)

    process_events()

    for area in scroll_areas:
        if isinstance(area, QScrollArea):
            area.ensureWidgetVisible(widget)
    process_events()

    return widget.isVisible()


def _select_page_containing(container, widget, select):
    for index in range(container.count()):
        page = container.widget(index)
        if page is widget or page.isAncestorOf(widget):
            select(index)
            return True
    return False


def process_events(rounds=1):
    """Drain the Qt event queue.

    Several rounds are sometimes needed because handlers post further events - a queued signal
    whose slot emits another queued signal. Use this to let a layout catch up before measuring it;
    use ``wait_for`` for anything that takes real time.
    """
    for _ in range(rounds):
        QApplication.processEvents(QEventLoop.AllEvents, 20)


# --------------------------------------------------------------------------------------------
# driving widgets
# --------------------------------------------------------------------------------------------


def click(button):
    """Press a button, visibly. **Completes asynchronously.**

    ``animateClick`` rather than ``click`` because a tutorial is watched: it draws the button held
    down for about 100 ms, so the user sees *which* button the tour pressed rather than only its
    consequences. The cost is that ``clicked`` is emitted at the end of that animation, so nothing
    the press causes has happened when this returns. Steps absorb that in ``settle_ms``; anything
    that must observe the result should go through ``wait_for``.

    Use ``set_check_state`` for a button whose *state* matters - it cannot afford this delay.
    """
    if not isinstance(button, QAbstractButton):
        raise TypeError(f"click expects a button, got {type(button).__name__}")
    if not button.isEnabled():
        raise RuntimeError(f"'{button.objectName() or button.text()}' is disabled, so the tutorial cannot press it")
    button.animateClick()
    process_events()


def set_check_state(button, checked):
    """Put a check box, radio button or checkable group box into a known state.

    Synchronous, unlike ``click``: the caller asked for a state rather than for a press, so the
    state has to hold by the time this returns - both for the caller's next line and for the
    assertion below, which is what turns "the interface ignored us" into an error here rather than
    a puzzling absence of output several steps later. The visible press is given up to get that.
    """
    if button.isChecked() == checked:
        return button.isChecked()
    if isinstance(button, QGroupBox):
        # a group box's indicator is a sub-control of its frame with no dependable hot spot, and
        # handlers watch ``toggled``, which setChecked emits
        button.setChecked(checked)
    else:
        # click() rather than setChecked() so that handlers connected to ``clicked`` - not only
        # those on ``toggled`` - run, as they would for a user
        button.click()
    process_events()
    if button.isChecked() != checked:
        raise RuntimeError(f"'{button.objectName() or _button_text(button)}' is {button.isChecked()} after asking for {checked}")
    return button.isChecked()


def _button_text(button):
    return button.title() if isinstance(button, QGroupBox) else button.text()


def select_combo(combo, text):
    """Select a combo entry by the text the user reads.

    ``setCurrentText`` on a non-editable combo leaves the selection untouched when the text is not
    among the items, which would leave the tutorial narrating a choice it never made.
    """
    index = combo.findText(text)
    if index < 0:
        raise ValueError(f"'{text}' is not in the combo box; available: {combo_items(combo)}")
    combo.setCurrentIndex(index)
    process_events()
    return index


def combo_items(combo):
    return [combo.itemText(index) for index in range(combo.count())]


def select_tab(tab_widget, title):
    """Make a tab current by its title.

    By title rather than index because indices move: interfaces add tabs conditionally, and a
    tutorial written against index 2 would quietly start describing the wrong page.
    """
    for index in range(tab_widget.count()):
        if tab_widget.tabText(index) == title:
            tab_widget.setCurrentIndex(index)
            process_events()
            return tab_widget.widget(index)
    available = [tab_widget.tabText(index) for index in range(tab_widget.count())]
    raise ValueError(f"no tab titled '{title}'; found {available}")


def set_text(line_edit, text):
    """Put text into a line edit as though it had been typed.

    ``setText`` emits ``textChanged`` and nothing else, so an interface that reacts to
    ``textEdited``, ``editingFinished`` or ``returnPressed`` - and Mantid's interfaces react to all
    three - would not notice. Emitting them explicitly is what makes the interface respond as it
    would for a user, without replaying keystrokes through ``QtTest``.
    """
    if not isinstance(line_edit, QLineEdit):
        raise TypeError(f"set_text expects a QLineEdit, got {type(line_edit).__name__}")
    text = str(text)
    line_edit.setText(text)
    line_edit.textEdited.emit(text)
    line_edit.editingFinished.emit()
    line_edit.returnPressed.emit()
    process_events()
    return line_edit.text()


def set_spin_box(spin_box, value):
    """Set a spin box, checking the value survived.

    Spin boxes clamp to their range and round to their number of decimals silently, so a tutorial
    that asked for an out-of-range value would go on to describe a result the interface never
    produced.
    """
    spin_box.setValue(value)
    process_events()
    if spin_box.value() != value:
        raise ValueError(
            f"spin box '{spin_box.objectName() or spin_box}' holds {spin_box.value()} after asking for {value}; "
            f"its range is {spin_box.minimum()} to {spin_box.maximum()}"
        )
    return spin_box.value()


def widget_of_type(parent, widget_type, name=""):
    """One child widget, found by type and optionally object name.

    For the widgets a tutorial wants to point at that are built in code rather than named in a
    ``.ui`` file - a matplotlib canvas dropped into a placeholder, a toolbar's buttons.
    """
    found = parent.findChildren(widget_type, name) if name else parent.findChildren(widget_type)
    if not found:
        raise ValueError(f"no {widget_type.__name__}{f' named {name!r}' if name else ''} under {parent.objectName() or parent}")
    return found[0]


def is_shown(widget):
    """Whether a widget is really on screen, not merely constructed.

    ``isVisible`` is False for everything while the window itself is hidden, so this is the check a
    tutorial wants when deciding whether it can point at something.
    """
    return isinstance(widget, QWidget) and widget.isVisible() and not widget.visibleRegion().isEmpty()
