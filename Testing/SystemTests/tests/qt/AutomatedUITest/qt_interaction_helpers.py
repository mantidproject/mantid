# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Widget-level helpers for driving a Qt interface from a test.

These are free functions with no dependency on the system test framework, so they can be used from
an ordinary unit test as readily as from an ``AutomatedUITestBase`` subclass. Most of them exist
because the naive Qt call does not do what you want in a headless test:

* a ``QCheckBox`` only toggles when the click lands on its *indicator*, not its label,
* ``FileFinderWidget`` searches on a background thread, so its result is not available on the next
  line, and
* anything that waits must keep pumping the Qt event loop (see ``wait_until``).
"""

import time

from qtpy.QtCore import QEventLoop, QPoint, Qt
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QApplication, QCheckBox, QStyle

# a click that has to travel through the event loop is delivered by processEvents; 20 ms is long
# enough to drain a burst of queued signals without making the polling loops feel sluggish
_EVENT_SLICE_MS = 20


def process_events(rounds=1):
    """Drain the Qt event queue. Several rounds are sometimes needed because handlers post further
    events (a queued signal whose slot emits another queued signal)."""
    for _ in range(rounds):
        QApplication.processEvents(QEventLoop.AllEvents, _EVENT_SLICE_MS)


def click(widget):
    """Left-click the centre of a widget."""
    QTest.mouseClick(widget, Qt.LeftButton)
    process_events()


def click_checkbox(checkbox):
    """Toggle a checkbox by clicking its indicator.

    A click on the centre of a wide checkbox lands on the label, which does toggle it in most
    styles but not reliably in the offscreen platform plugin. The indicator at the left edge is the
    dependable hot-spot, and it is also where the checkboxes embedded in table cells live.
    """
    indicator_w = checkbox.style().pixelMetric(QStyle.PM_IndicatorWidth)
    QTest.mouseClick(checkbox, Qt.LeftButton, pos=QPoint(max(indicator_w // 2, 4), checkbox.height() // 2))
    process_events()


def set_checkbox(checkbox, checked):
    """Click a checkbox only if it is not already in the wanted state, so the caller does not have
    to know the current state to end up in a known one."""
    if checkbox.isChecked() != checked:
        click_checkbox(checkbox)
    return checkbox.isChecked()


def combo_items(combo):
    return [combo.itemText(i) for i in range(combo.count())]


def select_combo(combo, text):
    """Select a combo entry by its visible text, failing loudly rather than silently leaving the
    selection unchanged (which is what ``setCurrentText`` does for a non-editable combo)."""
    index = combo.findText(text)
    if index < 0:
        raise AssertionError(f"'{text}' is not in the combo box; available: {combo_items(combo)}")
    combo.setCurrentIndex(index)
    process_events()


def table_column(table, col):
    """Text of every cell in a column, top to bottom."""
    return [table.item(row, col).text() if table.item(row, col) else None for row in range(table.rowCount())]


def table_checkbox(table, row, col):
    """The QCheckBox inside a table cell.

    Both the correction and texture tables put their select box inside a QWidget/QHBoxLayout so it
    can be centred, so the checkbox is a grandchild of the cell rather than the cell widget itself.
    """
    cell_widget = table.cellWidget(row, col)
    if cell_widget is None:
        raise AssertionError(f"no cell widget at row {row}, column {col}")
    checkbox = cell_widget.findChild(QCheckBox)
    if checkbox is None:
        raise AssertionError(f"no checkbox inside the cell widget at row {row}, column {col}")
    return checkbox


def cell_button(table, row, col):
    """The QPushButton inside a table cell, e.g. the '[View Shape]' buttons."""
    from qtpy.QtWidgets import QPushButton

    cell_widget = table.cellWidget(row, col)
    if cell_widget is None:
        return None
    return cell_widget if isinstance(cell_widget, QPushButton) else cell_widget.findChild(QPushButton)


def wait_until(predicate, timeout=120.0, msg=""):
    """Block until ``predicate()`` is true, pumping the event loop while waiting.

    Never use a bare ``sleep`` or ``thread.join()`` in a GUI test: work that finishes on a
    background thread usually reports back through a queued - sometimes *blocking* queued - Qt
    connection, which cannot be delivered unless this thread is running its event loop.
    """
    deadline = time.time() + timeout
    while not predicate():
        process_events()
        if time.time() > deadline:
            raise RuntimeError(f"timed out after {timeout}s waiting for {msg or 'condition'}")
        time.sleep(0.005)
    process_events()


def wait_for_file_finder(finder, msg=""):
    """Wait for a FileFinderWidget to finish its background search."""
    wait_until(lambda: not finder.isSearching(), msg=msg or "file finder search")


def set_finder_text(finder, text, expect_valid=True):
    """Type a run number or path into a FileFinderWidget and wait for the search to resolve.

    The Engineering presenters refuse to act (and pop a modal warning) while a finder is still
    searching or is invalid, so a test that does not wait here fails in a way that looks like a
    logic bug rather than a race.
    """
    finder.setFileTextWithSearch(text)
    wait_for_file_finder(finder, msg=f"file finder resolving '{text}'")
    if expect_valid and not finder.isValid():
        raise AssertionError(f"file finder could not resolve '{text}'")
    return finder


def figure_numbers():
    """Open matplotlib figure numbers. Used to assert that a 'plot output' checkbox really did (or
    really did not) create a plot."""
    import matplotlib.pyplot as plt

    return set(plt.get_fignums())


def close_all_figures():
    import matplotlib.pyplot as plt

    plt.close("all")


def top_level_widget_names():
    """Object names of the currently open top-level widgets, for asserting that a button opened a
    new window."""
    return [w.objectName() for w in QApplication.topLevelWidgets() if w.isVisible()]
