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
* ``setText`` on a ``QLineEdit`` emits none of the signals that typing into it does,
* a click on an item view has to be delivered to its *viewport*, at the index's ``visualRect``,
* ``FileFinderWidget`` searches on a background thread, so its result is not available on the next
  line, and
* anything that waits must keep pumping the Qt event loop (see ``wait_until``).

Two families of helper look similar and are not interchangeable. The ``table_*`` ones work on a
``QTableWidget``, which owns its cells; the ``model_*`` and ``*_index`` ones work on anything backed
by a ``QAbstractItemModel`` (``QTableView``, ``QTreeView``, ``QListView``), where the cells belong to
the model and ``item()``/``cellWidget()`` do not exist at all.
"""

import time

# Qt is imported inside the functions that need it, never at module scope: this module is imported
# by test modules that the collector loads on every build, including framework-only builds with no
# Qt, and an import error there would break collection before the test could report a clean skip.

# a click that has to travel through the event loop is delivered by processEvents; 20 ms is long
# enough to drain a burst of queued signals without making the polling loops feel sluggish
_EVENT_SLICE_MS = 20


def process_events(rounds=1):
    """Drain the Qt event queue. Several rounds are sometimes needed because handlers post further
    events (a queued signal whose slot emits another queued signal)."""
    from qtpy.QtCore import QEventLoop
    from qtpy.QtWidgets import QApplication

    for _ in range(rounds):
        QApplication.processEvents(QEventLoop.AllEvents, _EVENT_SLICE_MS)


def click(widget):
    """Left-click the centre of a widget."""
    from qtpy.QtCore import Qt
    from qtpy.QtTest import QTest

    QTest.mouseClick(widget, Qt.LeftButton)
    process_events()


def click_checkbox(checkbox):
    """Toggle a checkbox by clicking its indicator.

    A click on the centre of a wide checkbox lands on the label, which does toggle it in most
    styles but not reliably in the offscreen platform plugin. The indicator at the left edge is the
    dependable hot-spot, and it is also where the checkboxes embedded in table cells live.
    """
    from qtpy.QtCore import QPoint, Qt
    from qtpy.QtTest import QTest
    from qtpy.QtWidgets import QStyle

    indicator_w = checkbox.style().pixelMetric(QStyle.PM_IndicatorWidth)
    QTest.mouseClick(checkbox, Qt.LeftButton, pos=QPoint(max(indicator_w // 2, 4), checkbox.height() // 2))
    process_events()


def set_checkbox(checkbox, checked):
    """Click a checkbox only if it is not already in the wanted state, so the caller does not have
    to know the current state to end up in a known one."""
    if checkbox.isChecked() != checked:
        click_checkbox(checkbox)
    if checkbox.isChecked() != checked:
        raise AssertionError(f"checkbox '{checkbox.text()}' is {checkbox.isChecked()} after asking for {checked}")
    return checkbox.isChecked()


def click_radio(radio):
    """Select a radio button by clicking its indicator.

    The ``click_checkbox`` problem exactly, with the other style metric: a click on the centre of a
    wide radio button lands on its label, which is not a dependable hot-spot offscreen.
    """
    from qtpy.QtCore import QPoint, Qt
    from qtpy.QtTest import QTest
    from qtpy.QtWidgets import QStyle

    indicator_w = radio.style().pixelMetric(QStyle.PM_ExclusiveIndicatorWidth)
    QTest.mouseClick(radio, Qt.LeftButton, pos=QPoint(max(indicator_w // 2, 4), radio.height() // 2))
    process_events()


def radio_buttons(parent):
    """Every radio button under ``parent``, which may be a widget or a ``QButtonGroup``."""
    from qtpy.QtWidgets import QButtonGroup, QRadioButton

    if isinstance(parent, QButtonGroup):
        return list(parent.buttons())
    return parent.findChildren(QRadioButton)


def select_radio(parent, text):
    """Choose the radio button labelled ``text`` from those under ``parent``.

    Radio buttons are nearly always one option out of a group, so the useful call selects by the
    label the user reads rather than by holding a reference to each button. ``parent`` may be a
    widget or the ``QButtonGroup`` itself.
    """
    buttons = radio_buttons(parent)
    for button in buttons:
        if button.text() == text:
            click_radio(button)
            if not button.isChecked():
                raise AssertionError(f"radio button '{text}' is still unchecked after being clicked")
            return button
    raise AssertionError(f"'{text}' is not among the radio buttons; available: {[b.text() for b in buttons]}")


def set_group_box(group_box, checked):
    """Turn a checkable ``QGroupBox`` on or off.

    The odd one out: this sets rather than clicks. A group box's check indicator is drawn as a
    sub-control of the frame, so its position is style-dependent and there is no reliable hot-spot to
    aim at. Setting is faithful enough because handlers connect to ``toggled``, which ``setChecked``
    emits - a group box is a section switch, not something a presenter watches for ``clicked``.
    """
    if not group_box.isCheckable():
        raise AssertionError(f"group box '{group_box.title()}' is not checkable")
    group_box.setChecked(checked)
    process_events()
    if group_box.isChecked() != checked:
        raise AssertionError(f"group box '{group_box.title()}' is {group_box.isChecked()} after asking for {checked}")
    return group_box.isChecked()


def set_line_edit(line_edit, text, expect_valid=True):
    """Type into a line edit, rather than assigning to it.

    ``setText`` emits neither ``textEdited``, ``returnPressed`` nor ``editingFinished``, and
    interfaces hang their reactions off all three - so assigning leaves the interface unaware that
    anything was entered. Replaying the keystrokes is what makes it behave as it would for a user.
    The sequence is the one Mantid's own line edit tests use.

    ``expect_valid`` asserts against any validator on the widget, so a value the widget would refuse
    fails here rather than several steps later as a puzzling absence of output.
    """
    from qtpy.QtCore import Qt
    from qtpy.QtTest import QTest

    line_edit.setFocus()
    line_edit.selectAll()
    QTest.keyClick(line_edit, Qt.Key_Backspace)
    QTest.keyClicks(line_edit, str(text))
    QTest.keyClick(line_edit, Qt.Key_Enter)
    process_events()
    if expect_valid and line_edit.validator() is not None and not line_edit.hasAcceptableInput():
        raise AssertionError(f"'{text}' was rejected by the validator on '{line_edit.objectName() or line_edit}'")
    return line_edit.text()


def set_spin_box(spin_box, value):
    """Set a ``QSpinBox`` or ``QDoubleSpinBox``, checking the value survived.

    Spin boxes clamp silently to their range and round to their number of decimals, so a value
    outside either would otherwise be read back as the interface having ignored the input.
    """
    spin_box.setValue(value)
    process_events()
    if spin_box.value() != value:
        raise AssertionError(
            f"spin box '{spin_box.objectName() or spin_box}' holds {spin_box.value()} after asking for {value}; "
            f"its range is {spin_box.minimum()} to {spin_box.maximum()}"
        )
    return spin_box.value()


def tab_titles(tab_widget):
    return [tab_widget.tabText(i) for i in range(tab_widget.count())]


def select_tab(tab_widget, title):
    """Make a tab current by its title.

    By title rather than index because indices move: interfaces add tabs conditionally and save and
    restore the current one between sessions. Widgets only report ``isVisible()`` on the current tab,
    so any visibility check has to select the tab first.
    """
    for index in range(tab_widget.count()):
        if tab_widget.tabText(index) == title:
            tab_widget.setCurrentIndex(index)
            process_events()
            return tab_widget.widget(index)
    raise AssertionError(f"no tab titled '{title}'; found {tab_titles(tab_widget)}")


def list_items(list_widget):
    return [list_widget.item(row).text() for row in range(list_widget.count())]


def select_list_item(list_widget, text):
    """Select a ``QListWidget`` row by its text.

    Not only for lists of data: some interfaces navigate with a list beside a ``QStackedWidget``
    instead of a tab bar, so this is how a test reaches their pages.
    """
    for row in range(list_widget.count()):
        item = list_widget.item(row)
        if item.text() == text:
            list_widget.setCurrentItem(item)
            process_events()
            return item
    raise AssertionError(f"'{text}' is not in the list; available: {list_items(list_widget)}")


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


# ---------------------------------------------------------------------------------------------
# QTableWidget. For a QTableView or any other model-backed view see the model/view section below -
# none of these work there, because item() and cellWidget() belong to QTableWidget alone.
# ---------------------------------------------------------------------------------------------


def table_column(table, col):
    """Text of every cell in a column, top to bottom."""
    return [table.item(row, col).text() if table.item(row, col) else None for row in range(table.rowCount())]


def cell_widget(table, row, col, widget_type):
    """The widget of ``widget_type`` in a table cell, or ``None`` if the cell has no widget.

    Tables usually put an embedded control inside a QWidget/QHBoxLayout so it can be centred, which
    makes the control a grandchild of the cell rather than the cell widget itself - but not always,
    so both layouts are accepted. Interfaces embed check boxes, buttons, combo boxes, line edits and
    spin boxes this way.
    """
    container = table.cellWidget(row, col)
    if container is None:
        return None
    if isinstance(container, widget_type):
        return container
    return container.findChild(widget_type)


def table_checkbox(table, row, col):
    """The QCheckBox inside a table cell.

    For a cell that is a *checkable item* rather than an embedded check box - no cell widget at all -
    use ``item_check_state``/``set_item_checked`` instead.
    """
    from qtpy.QtWidgets import QCheckBox

    if table.cellWidget(row, col) is None:
        raise AssertionError(f"no cell widget at row {row}, column {col}")
    checkbox = cell_widget(table, row, col, QCheckBox)
    if checkbox is None:
        raise AssertionError(f"no checkbox inside the cell widget at row {row}, column {col}")
    return checkbox


def cell_button(table, row, col):
    """The QPushButton inside a table cell, e.g. the '[View Shape]' buttons.

    Returns ``None`` for a cell with no widget, because "this row offers no button" is a state worth
    asserting on rather than an error.
    """
    from qtpy.QtWidgets import QPushButton

    return cell_widget(table, row, col, QPushButton)


def cell_combo(table, row, col):
    from qtpy.QtWidgets import QComboBox

    return cell_widget(table, row, col, QComboBox)


def cell_line_edit(table, row, col):
    from qtpy.QtWidgets import QLineEdit

    return cell_widget(table, row, col, QLineEdit)


def cell_spin_box(table, row, col):
    from qtpy.QtWidgets import QAbstractSpinBox

    return cell_widget(table, row, col, QAbstractSpinBox)


def item_check_state(table, row, col):
    """Whether a checkable *item* in a cell is ticked.

    The other way a table offers a tick box: instead of embedding a QCheckBox, the cell's own
    QTableWidgetItem is given ``Qt.ItemIsUserCheckable`` and a check state. There is no cell widget
    at all, so ``table_checkbox`` cannot see it.
    """
    from qtpy.QtCore import Qt

    item = table.item(row, col)
    if item is None:
        raise AssertionError(f"no item at row {row}, column {col}")
    return item.checkState() == Qt.Checked


def set_item_checked(table, row, col, checked):
    """Tick or untick a checkable item in a cell. See ``item_check_state``."""
    from qtpy.QtCore import Qt

    item = table.item(row, col)
    if item is None:
        raise AssertionError(f"no item at row {row}, column {col}")
    item.setCheckState(Qt.Checked if checked else Qt.Unchecked)
    process_events()
    if item_check_state(table, row, col) != checked:
        raise AssertionError(f"item at row {row}, column {col} is not {checked} after being set; is it user-checkable?")
    return checked


# ---------------------------------------------------------------------------------------------
# Model-backed views: QTableView, QTreeView, QListView.
#
# The cells belong to the model, not the view, so nothing in the QTableWidget section above applies.
# The part that cannot be guessed is the clicking: QTest.mouseClick on the view itself does nothing,
# because the view is only a frame around a viewport that draws the delegates. A click has to be
# delivered to that viewport, at the rectangle the view says the index occupies.
# ---------------------------------------------------------------------------------------------


def model_row_count(view, parent=None):
    from qtpy.QtCore import QModelIndex

    return view.model().rowCount(parent if parent is not None else QModelIndex())


def model_column_count(view, parent=None):
    from qtpy.QtCore import QModelIndex

    return view.model().columnCount(parent if parent is not None else QModelIndex())


def model_index(view, row, col=0, parent=None):
    from qtpy.QtCore import QModelIndex

    index = view.model().index(row, col, parent if parent is not None else QModelIndex())
    if not index.isValid():
        raise AssertionError(f"row {row}, column {col} is outside the model, which is {model_row_count(view)} rows")
    return index


def model_cell(view, row, col, role=None):
    """The data at a cell, displayed text by default."""
    from qtpy.QtCore import Qt

    return model_index(view, row, col).data(Qt.DisplayRole if role is None else role)


def model_column(view, col):
    """Displayed text of every cell in a column, top to bottom. The ``table_column`` counterpart."""
    return [model_cell(view, row, col) for row in range(model_row_count(view))]


def index_check_state(view, row, col=0):
    """Whether a model index is ticked, for models that expose ``Qt.CheckStateRole``.

    The role is normalised through ``Qt.CheckState`` before comparing: a C++-backed model such as
    ``QStandardItemModel`` hands back a plain ``int``, which never compares equal to the scoped
    enum ``Qt.Checked`` under PyQt6, so a direct comparison silently reports every ticked item as
    unticked. ``None`` means the model does not answer the role at all, i.e. not ticked.
    """
    from qtpy.QtCore import Qt

    state = model_index(view, row, col).data(Qt.CheckStateRole)
    return state is not None and Qt.CheckState(state) == Qt.Checked


def click_index(view, row, col=0, modifier=None, parent=None):
    """Click a cell in a model-backed view.

    Goes to the viewport at the index's ``visualRect`` - a click on the view itself lands on its
    frame and does nothing. ``scrollTo`` first because ``visualRect`` is an empty rectangle for an
    index that is scrolled out of sight, which would quietly send the click to the top-left corner
    and select the wrong row.
    """
    from qtpy.QtCore import Qt
    from qtpy.QtTest import QTest

    index = model_index(view, row, col, parent)
    view.scrollTo(index)
    QTest.mouseClick(view.viewport(), Qt.LeftButton, Qt.NoModifier if modifier is None else modifier, view.visualRect(index).center())
    process_events()
    return index


def double_click_index(view, row, col=0, parent=None):
    """Double-click a cell, which is what opens its delegate's editor."""
    from qtpy.QtCore import Qt
    from qtpy.QtTest import QTest

    index = click_index(view, row, col, parent=parent)
    QTest.mouseDClick(view.viewport(), Qt.LeftButton, Qt.NoModifier, view.visualRect(index).center())
    process_events()
    return index


def edit_index(view, row, col, text, parent=None):
    """Type into a cell of a model-backed view.

    There is no widget to reach until the cell is being edited: the delegate builds its editor on
    demand. So this double-clicks to open the editor, takes whatever widget then has focus, and
    types into that.

    Deliberately not written in terms of ``set_line_edit``, even though the editor is usually a
    ``QLineEdit``. The Enter that commits the edit also *destroys* the editor, so anything that
    touches it afterwards - as ``set_line_edit`` does, to return the text - raises "wrapped C/C++
    object has been deleted". Read the value back from the model instead, which is where it now
    lives.
    """
    from qtpy.QtCore import Qt
    from qtpy.QtTest import QTest

    double_click_index(view, row, col, parent)
    editor = view.focusWidget()
    if editor is None or editor is view:
        raise AssertionError(f"double-clicking row {row}, column {col} opened no editor; is the cell editable?")
    if hasattr(editor, "selectAll"):
        editor.selectAll()
        QTest.keyClick(editor, Qt.Key_Backspace)
    QTest.keyClicks(editor, str(text))
    QTest.keyClick(editor, Qt.Key_Enter)
    process_events(2)
    return model_cell(view, row, col)


def select_index(view, row, col=0, parent=None):
    """Make a cell current without clicking, for when the selection matters but the click does not."""
    index = model_index(view, row, col, parent)
    view.setCurrentIndex(index)
    process_events()
    return index


def click_row_header(view, row, modifier=None):
    """Click a row's header, which selects the whole row."""
    from qtpy.QtCore import QPoint, Qt
    from qtpy.QtTest import QTest

    header = view.verticalHeader()
    # sectionViewportPosition, not sectionPosition: the click is delivered to the viewport, and the
    # two differ by the scroll offset as soon as the view has been scrolled. The cross-axis
    # coordinate is the viewport's width - length() is the sum of every section, which for anything
    # but a one-row view lands outside the header entirely.
    pos = QPoint(int(header.viewport().width() / 2), int(header.sectionViewportPosition(row) + header.sectionSize(row) / 2))
    QTest.mouseClick(header.viewport(), Qt.LeftButton, Qt.NoModifier if modifier is None else modifier, pos)
    process_events()


def click_column_header(view, col, modifier=None):
    """Click a column's header, which selects the whole column - or sorts by it, if sorting is on."""
    from qtpy.QtCore import QPoint, Qt
    from qtpy.QtTest import QTest

    header = view.horizontalHeader()
    # see click_row_header for why this is the viewport position and the viewport's height
    pos = QPoint(int(header.sectionViewportPosition(col) + header.sectionSize(col) / 2), int(header.viewport().height() / 2))
    QTest.mouseClick(header.viewport(), Qt.LeftButton, Qt.NoModifier if modifier is None else modifier, pos)
    process_events()


def tree_rows(view, parent=None, depth=0, col=0):
    """Walk a tree's rows depth-first, as ``(depth, text, index)`` triples.

    A flat listing is what an assertion usually wants - "these entries, indented like this" - and it
    is also the only practical way to reach a child index, since the caller needs the parent index to
    build one and would otherwise have to recurse themselves.
    """
    from qtpy.QtCore import QModelIndex, Qt

    model = view.model()
    parent = QModelIndex() if parent is None else parent
    rows = []
    for row in range(model.rowCount(parent)):
        index = model.index(row, col, parent)
        rows.append((depth, index.data(Qt.DisplayRole), index))
        if model.hasChildren(index):
            rows.extend(tree_rows(view, index, depth + 1, col))
    return rows


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
    # the search is started from a queued signal, so isSearching() can still be false for the
    # *previous* text on the next line; wait for the widget to take the new text first
    wait_until(lambda: finder.getText() == text, msg=f"file finder accepting '{text}'")
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
    from qtpy.QtWidgets import QApplication

    return [w.objectName() for w in QApplication.topLevelWidgets() if w.isVisible()]
