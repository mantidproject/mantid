# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""A worked example of an automated UI test, and the smoke test for the harness itself.

The widget under test is defined here rather than imported from Mantid on purpose. Every other
suite in this tree drives a real interface, so when one of them fails the first question is always
"is it the interface or is it the harness?". This module answers that: it depends on nothing but
``AutomatedUITestBase`` and ``qt_interaction_helpers``, so if it fails the harness is broken.

Read it as a template. A real suite differs only in that ``setUp`` builds a Mantid interface instead
of ``_ExampleWidget``, and that the assertions look at workspaces and files as well as widgets.
"""

import unittest

from automated_ui_test_base import AutomatedUITestBase
from qt_interaction_helpers import (
    cell_button,
    cell_combo,
    click,
    click_checkbox,
    click_column_header,
    click_index,
    click_row_header,
    combo_items,
    double_click_index,
    edit_index,
    index_check_state,
    item_check_state,
    list_items,
    model_cell,
    model_column,
    model_row_count,
    process_events,
    select_combo,
    select_index,
    select_list_item,
    select_radio,
    select_tab,
    set_checkbox,
    set_group_box,
    set_item_checked,
    set_line_edit,
    set_spin_box,
    table_checkbox,
    table_column,
    tree_rows,
    wait_until,
)

# how long the fake background task takes. Long enough that a test which forgot to wait would see
# the un-updated value, short enough not to pad the suite.
_TASK_DURATION_MS = 250

_GREETINGS = ("Hello", "Goodbye", "Good morning")

# tab titles, deliberately not in alphabetical or any other order that could be guessed from an
# index - selecting a tab by title has to actually look the title up
_TAB_TITLES = ("Results", "Setup", "Diagnostics")

# pages of the list-driven navigation, the shape some interfaces use instead of a tab bar
_PAGE_NAMES = ("Runs", "Masking", "Output")

# rows of the model behind the QTableView. (name, ticked, note) - note is the editable column
_MODEL_ROWS = (("alpha", True, "first"), ("beta", False, "second"), ("gamma", False, "third"))

# the tree, as (parent, [children]) - two levels is enough to exercise a recursive walk
_TREE = (("North", ("N1", "N2")), ("South", ("S1",)))


def _build_table_model():
    """A small editable, checkable table model.

    Defined here rather than reusing ``QStandardItemModel`` because the helpers have to work against
    a hand-written ``QAbstractTableModel`` - which is what every model-backed Mantid interface has -
    and those implement ``data``/``setData``/``flags`` themselves.
    """
    from qtpy.QtCore import QAbstractTableModel, QModelIndex, Qt

    class _ExampleTableModel(QAbstractTableModel):
        HEADERS = ("Name", "Use", "Note")

        def __init__(self):
            super(_ExampleTableModel, self).__init__()
            self.rows = [list(row) for row in _MODEL_ROWS]

        def rowCount(self, parent=QModelIndex()):
            return 0 if parent.isValid() else len(self.rows)

        def columnCount(self, parent=QModelIndex()):
            return 0 if parent.isValid() else len(self.HEADERS)

        def headerData(self, section, orientation, role=Qt.DisplayRole):
            if role == Qt.DisplayRole and orientation == Qt.Horizontal:
                return self.HEADERS[section]
            return None

        def data(self, index, role=Qt.DisplayRole):
            name, ticked, note = self.rows[index.row()]
            if role in (Qt.DisplayRole, Qt.EditRole):
                return {0: name, 1: "", 2: note}[index.column()]
            if role == Qt.CheckStateRole and index.column() == 1:
                return Qt.Checked if ticked else Qt.Unchecked
            return None

        def setData(self, index, value, role=Qt.EditRole):
            if role == Qt.CheckStateRole and index.column() == 1:
                self.rows[index.row()][1] = Qt.CheckState(value) == Qt.Checked
            elif role == Qt.EditRole and index.column() == 2:
                self.rows[index.row()][2] = str(value)
            else:
                return False
            self.dataChanged.emit(index, index)
            return True

        def flags(self, index):
            flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable
            if index.column() == 1:
                flags |= Qt.ItemIsUserCheckable
            if index.column() == 2:
                flags |= Qt.ItemIsEditable
            return flags

    return _ExampleTableModel()


def _build_tree_model():
    from qtpy.QtGui import QStandardItem, QStandardItemModel

    model = QStandardItemModel()
    model.setHorizontalHeaderLabels(["Bank"])
    for parent_name, child_names in _TREE:
        parent = QStandardItem(parent_name)
        for child_name in child_names:
            parent.appendRow(QStandardItem(child_name))
        model.appendRow(parent)
    return model


def _build_widget():
    """The interface under test: one of every widget kind the helpers know how to drive.

    Built inside a function rather than at module scope because a ``QWidget`` cannot be constructed
    before the ``QApplication`` exists, and the harness creates that lazily in ``setUp``.
    """
    from qtpy.QtCore import QTimer, Qt
    from qtpy.QtGui import QDoubleValidator
    from qtpy.QtWidgets import (
        QButtonGroup,
        QCheckBox,
        QComboBox,
        QGroupBox,
        QHBoxLayout,
        QLabel,
        QLineEdit,
        QListWidget,
        QPushButton,
        QRadioButton,
        QSpinBox,
        QStackedWidget,
        QTableWidget,
        QTableWidgetItem,
        QTableView,
        QTabWidget,
        QTreeView,
        QVBoxLayout,
        QWidget,
    )

    class _ExampleWidget(QWidget):
        def __init__(self):
            super(_ExampleWidget, self).__init__()
            self.setObjectName("ExampleWidget")

            layout = QVBoxLayout(self)
            layout.addWidget(self._build_greeting_section())
            layout.addWidget(self._build_entry_section())
            layout.addWidget(self._build_choice_section())
            layout.addWidget(self._build_navigation_section())
            layout.addWidget(self._build_table())
            layout.addWidget(self._build_item_views())

        # -------------------------------------------------------------- sections

        def _build_greeting_section(self):
            self.combo_greeting = QComboBox()
            self.combo_greeting.addItems(_GREETINGS)
            self.check_shout = QCheckBox("Shout")
            self.button_greet = QPushButton("Greet")
            self.label_greeting = QLabel("")
            self.button_start_task = QPushButton("Start task")
            self.label_task = QLabel("idle")

            self.button_greet.clicked.connect(self._greet)
            self.button_start_task.clicked.connect(self._start_task)

            return _stack(
                self.combo_greeting,
                self.check_shout,
                self.button_greet,
                self.label_greeting,
                self.button_start_task,
                self.label_task,
            )

        def _build_entry_section(self):
            # a validator, so a test can prove set_line_edit checks against one
            self.edit_threshold = QLineEdit()
            self.edit_threshold.setValidator(QDoubleValidator(0.0, 100.0, 2))
            # only editingFinished, never textChanged: the point is that typing fires it and
            # assigning with setText does not
            self.label_threshold = QLabel("unset")
            self.edit_threshold.editingFinished.connect(lambda: self.label_threshold.setText(self.edit_threshold.text()))

            # a deliberately narrow range, so a test can prove set_spin_box notices the clamp
            self.spin_count = QSpinBox()
            self.spin_count.setRange(1, 10)

            return _stack(self.edit_threshold, self.label_threshold, self.spin_count)

        def _build_choice_section(self):
            self.radio_north = QRadioButton("North")
            self.radio_south = QRadioButton("South")
            self.radio_north.setChecked(True)
            self.group_bank = QButtonGroup(self)
            self.group_bank.addButton(self.radio_north)
            self.group_bank.addButton(self.radio_south)

            self.box_advanced = QGroupBox("Advanced")
            self.box_advanced.setCheckable(True)
            self.box_advanced.setChecked(False)
            self.check_nested = QCheckBox("Nested option")
            QVBoxLayout(self.box_advanced).addWidget(self.check_nested)
            self.label_advanced = QLabel("off")
            self.box_advanced.toggled.connect(lambda on: self.label_advanced.setText("on" if on else "off"))

            return _stack(self.radio_north, self.radio_south, self.box_advanced, self.label_advanced)

        def _build_navigation_section(self):
            self.tabs = QTabWidget()
            for title in _TAB_TITLES:
                page = QLabel(f"page for {title}")
                page.setObjectName(f"page_{title}")
                self.tabs.addTab(page, title)

            # the other way interfaces navigate: a list beside a stack, rather than a tab bar
            self.list_pages = QListWidget()
            self.list_pages.addItems(_PAGE_NAMES)
            self.stack_pages = QStackedWidget()
            for name in _PAGE_NAMES:
                self.stack_pages.addWidget(QLabel(f"contents of {name}"))
            self.list_pages.currentRowChanged.connect(self.stack_pages.setCurrentIndex)

            return _stack(self.tabs, self.list_pages, self.stack_pages)

        def _build_table(self):
            """A QTableWidget offering a tick box both ways round, plus a button and a combo."""
            self.table = QTableWidget(len(_GREETINGS), 5)
            self.table.setHorizontalHeaderLabels(["Select", "Greeting", "Action", "Enabled", "Style"])
            for row, greeting in enumerate(_GREETINGS):
                # an embedded check box, centred inside a container widget - the awkward layout
                cell = QWidget()
                cell_layout = QHBoxLayout(cell)
                cell_layout.setContentsMargins(0, 0, 0, 0)
                cell_layout.addWidget(QCheckBox())
                self.table.setCellWidget(row, 0, cell)

                self.table.setItem(row, 1, QTableWidgetItem(greeting))

                button = QPushButton("Use")
                button.clicked.connect(lambda _checked=False, text=greeting: self._set_greeting(text))
                self.table.setCellWidget(row, 2, button)

                # the same idea as column 0, expressed as a checkable *item* instead - no cell widget
                item = QTableWidgetItem()
                item.setFlags(Qt.ItemIsUserCheckable | Qt.ItemIsEnabled)
                item.setCheckState(Qt.Unchecked)
                self.table.setItem(row, 3, item)

                combo = QComboBox()
                combo.addItems(["plain", "bold"])
                self.table.setCellWidget(row, 4, combo)
            return self.table

        def _build_item_views(self):
            self.table_model = _build_table_model()
            self.table_view = QTableView()
            self.table_view.setModel(self.table_model)

            self.tree_view = QTreeView()
            self.tree_view.setModel(_build_tree_model())
            self.tree_view.expandAll()

            # give them room: visualRect is empty for a row with nowhere to be drawn, and a click
            # aimed at an empty rectangle silently lands in the corner
            for view in (self.table_view, self.tree_view):
                view.setMinimumSize(320, 160)
            return _stack(self.table_view, self.tree_view)

        # -------------------------------------------------------------- behaviour

        def _greet(self):
            text = f"{self.combo_greeting.currentText()}, world"
            self._set_greeting(text.upper() if self.check_shout.isChecked() else text)

        def _set_greeting(self, text):
            self.label_greeting.setText(text)

        def _start_task(self):
            """Stand in for the real thing: work that finishes on the event loop rather than on the
            next line, so a test has to wait for it with ``wait_until``."""
            self.label_task.setText("running")
            QTimer.singleShot(_TASK_DURATION_MS, lambda: self.label_task.setText("done"))

    def _stack(*widgets):
        holder = QWidget()
        holder_layout = QVBoxLayout(holder)
        holder_layout.setContentsMargins(0, 0, 0, 0)
        for widget in widgets:
            holder_layout.addWidget(widget)
        return holder

    return _ExampleWidget()


class ExampleUITest(AutomatedUITestBase):
    def setUp(self):
        super(ExampleUITest, self).setUp()
        self.widget = _build_widget()
        self.widget.show()
        process_events(2)

    def tearDown(self):
        widget = getattr(self, "widget", None)
        if widget is not None:
            widget.close()
            process_events(2)
            self.widget = None
        super(ExampleUITest, self).tearDown()

    def test_buttons_and_combo_boxes(self):
        with self.check("Example / the combo offers every greeting"):
            self.assertEqual(combo_items(self.widget.combo_greeting), list(_GREETINGS))

        select_combo(self.widget.combo_greeting, "Goodbye")
        click(self.widget.button_greet)
        with self.check("Example / clicking Greet uses the selected greeting"):
            self.assertEqual(self.widget.label_greeting.text(), "Goodbye, world")

        with self.check("Example / an absent combo entry is reported rather than ignored"):
            # setCurrentText() would silently do nothing here, which is the trap select_combo exists
            # to close, so the failure has to be an assertion
            self.assertRaises(AssertionError, select_combo, self.widget.combo_greeting, "Bonjour")

    def test_check_boxes(self):
        click_checkbox(self.widget.check_shout)
        with self.check("Example / clicking the indicator toggles the checkbox"):
            self.assertTrue(self.widget.check_shout.isChecked())

        with self.check("Example / a plain click() lands on the indicator too"):
            # click() dispatches on the widget kind, so reaching for the obvious call is not a trap
            click(self.widget.check_shout)
            self.assertFalse(self.widget.check_shout.isChecked())
            click(self.widget.check_shout)
            self.assertTrue(self.widget.check_shout.isChecked())

        with self.check("Example / setting a checkbox to the state it is already in is a no-op"):
            self.assertTrue(set_checkbox(self.widget.check_shout, True))

        set_checkbox(self.widget.check_shout, False)
        click(self.widget.button_greet)
        with self.check("Example / an unticked box leaves the greeting unshouted"):
            self.assertEqual(self.widget.label_greeting.text(), "Hello, world")

        set_checkbox(self.widget.check_shout, True)
        click(self.widget.button_greet)
        with self.check("Example / a ticked box reaches the handler"):
            self.assertEqual(self.widget.label_greeting.text(), "HELLO, WORLD")

    def test_tables(self):
        table = self.widget.table
        with self.check("Example / the table has one row per greeting"):
            self.assertEqual(table_column(table, 1), list(_GREETINGS))

        with self.check("Example / rows start unselected"):
            self.assertEqual([table_checkbox(table, row, 0).isChecked() for row in range(table.rowCount())], [False] * 3)

        set_checkbox(table_checkbox(table, 1, 0), True)
        with self.check("Example / a single row can be ticked on its own"):
            self.assertEqual([table_checkbox(table, row, 0).isChecked() for row in range(table.rowCount())], [False, True, False])

        click(cell_button(table, 2, 2))
        with self.check("Example / a per-row button acts on its own row"):
            self.assertEqual(self.widget.label_greeting.text(), _GREETINGS[2])

    def test_line_edits_and_spin_boxes(self):
        set_line_edit(self.widget.edit_threshold, "12.5")
        with self.check("Example / typing puts the text in the line edit"):
            self.assertEqual(self.widget.edit_threshold.text(), "12.5")
        with self.check("Example / and fires editingFinished, which setText would not"):
            # the whole reason set_line_edit replays keystrokes instead of assigning
            self.assertEqual(self.widget.label_threshold.text(), "12.5")

        with self.check("Example / a second entry replaces the first rather than appending"):
            set_line_edit(self.widget.edit_threshold, "7")
            self.assertEqual(self.widget.edit_threshold.text(), "7")

        with self.check("Example / a value the validator refuses is reported"):
            # the validator caps at 100, so the widget would hold something unexpected and the
            # interface would quietly do nothing - that has to surface here instead
            self.assertRaises(AssertionError, set_line_edit, self.widget.edit_threshold, "999")

        with self.check("Example / a spin box takes a value inside its range"):
            self.assertEqual(set_spin_box(self.widget.spin_count, 4), 4)

        with self.check("Example / a value outside the range is reported, not silently clamped"):
            self.assertRaises(AssertionError, set_spin_box, self.widget.spin_count, 99)

    def test_radio_buttons_and_group_boxes(self):
        with self.check("Example / a radio button can be selected by its label"):
            select_radio(self.widget.group_bank, "South")
            self.assertTrue(self.widget.radio_south.isChecked())
        with self.check("Example / and selecting one deselects the other"):
            self.assertFalse(self.widget.radio_north.isChecked())

        with self.check("Example / radio buttons can also be found through their parent widget"):
            select_radio(self.widget, "North")
            self.assertTrue(self.widget.radio_north.isChecked())

        with self.check("Example / an absent radio button is reported rather than ignored"):
            self.assertRaises(AssertionError, select_radio, self.widget.group_bank, "East")

        with self.check("Example / a checkable group box can be switched on"):
            self.assertTrue(set_group_box(self.widget.box_advanced, True))
        with self.check("Example / and its toggled handler runs"):
            self.assertEqual(self.widget.label_advanced.text(), "on")

        set_group_box(self.widget.box_advanced, False)
        with self.check("Example / switching a group box off disables what it contains"):
            self.assertFalse(self.widget.check_nested.isEnabled())

    def test_tabs_and_lists(self):
        # deliberately no assertion on the whole list of tab titles: that only restates the tab bar
        # and needs updating whenever a tab is added. Selecting a tab is what proves it exists.
        page = select_tab(self.widget.tabs, "Diagnostics")
        with self.check("Example / a tab is selected by title, not by index"):
            self.assertEqual(self.widget.tabs.tabText(self.widget.tabs.currentIndex()), "Diagnostics")
        with self.check("Example / and the page it returns is the current one"):
            self.assertIs(page, self.widget.tabs.currentWidget())

        with self.check("Example / an absent tab title is reported"):
            self.assertRaises(AssertionError, select_tab, self.widget.tabs, "Nonexistent")

        with self.check("Example / every list entry is listed"):
            self.assertEqual(list_items(self.widget.list_pages), list(_PAGE_NAMES))

        select_list_item(self.widget.list_pages, "Masking")
        with self.check("Example / selecting a list entry drives the stack beside it"):
            self.assertEqual(self.widget.stack_pages.currentIndex(), 1)

        with self.check("Example / an absent list entry is reported"):
            self.assertRaises(AssertionError, select_list_item, self.widget.list_pages, "Nonexistent")

    def test_table_items_and_cell_widgets(self):
        table = self.widget.table

        with self.check("Example / a checkable item is not an embedded check box"):
            # column 3 holds a checkable QTableWidgetItem, so there is no cell widget to find
            self.assertRaises(AssertionError, table_checkbox, table, 0, 3)

        with self.check("Example / checkable items start unticked"):
            self.assertEqual([item_check_state(table, row, 3) for row in range(table.rowCount())], [False] * 3)

        set_item_checked(table, 2, 3, True)
        with self.check("Example / a checkable item can be ticked on its own"):
            self.assertEqual([item_check_state(table, row, 3) for row in range(table.rowCount())], [False, False, True])

        with self.check("Example / a combo box embedded in a cell is reachable"):
            combo = cell_combo(table, 1, 4)
            self.assertEqual(combo_items(combo), ["plain", "bold"])
            select_combo(combo, "bold")
            self.assertEqual(combo.currentText(), "bold")

        with self.check("Example / a cell holding no widget of that kind gives None"):
            self.assertIsNone(cell_combo(table, 1, 1))

    def test_item_views(self):
        view = self.widget.table_view

        with self.check("Example / a model-backed column reads like a table column"):
            self.assertEqual(model_column(view, 0), [row[0] for row in _MODEL_ROWS])
        with self.check("Example / the row count comes from the model"):
            self.assertEqual(model_row_count(view), len(_MODEL_ROWS))

        with self.check("Example / the QTableWidget helpers do not work on a QTableView"):
            # the distinction the two sections of the helper module exist to keep straight
            self.assertRaises(AttributeError, table_column, view, 0)

        with self.check("Example / check states come from the model, not from a cell widget"):
            self.assertEqual([index_check_state(view, row, 1) for row in range(3)], [True, False, False])

        click_index(view, 2, 0)
        with self.check("Example / clicking an index selects it"):
            # a click on the view rather than its viewport would leave the selection untouched
            self.assertEqual(view.currentIndex().row(), 2)

        select_index(view, 0, 2)
        with self.check("Example / an index can be made current without a click"):
            self.assertEqual((view.currentIndex().row(), view.currentIndex().column()), (0, 2))

        click_row_header(view, 1)
        with self.check("Example / clicking a row header selects the whole row"):
            self.assertEqual(sorted(i.column() for i in view.selectionModel().selectedIndexes()), [0, 1, 2])

        click_column_header(view, 2)
        with self.check("Example / clicking a column header selects the whole column"):
            self.assertEqual(sorted(i.row() for i in view.selectionModel().selectedIndexes()), [0, 1, 2])

        edit_index(view, 0, 2, "edited")
        with self.check("Example / typing into a cell reaches the model through its delegate"):
            self.assertEqual(model_cell(view, 0, 2), "edited")

        with self.check("Example / an index outside the model is reported"):
            self.assertRaises(AssertionError, model_cell, view, 99, 0)

        rows = tree_rows(self.widget.tree_view)
        with self.check("Example / a tree walks depth-first with its nesting"):
            self.assertEqual([(depth, text) for depth, text, _ in rows], [(0, "North"), (1, "N1"), (1, "N2"), (0, "South"), (1, "S1")])

        # last, because it leaves an editor open - nothing after it would see a settled view
        with self.check("Example / double-clicking an editable cell opens its delegate's editor"):
            # what edit_index is built on: a single click only selects, so there would be nothing to type into
            double_click_index(view, 1, 2)
            editor = view.focusWidget()
            self.assertIsNotNone(editor)
            self.assertIsNot(editor, view)

    def test_waiting_for_work_that_finishes_on_the_event_loop(self):
        click(self.widget.button_start_task)
        with self.check("Example / the task has not finished by the time the click returns"):
            self.assertEqual(self.widget.label_task.text(), "running")

        wait_until(lambda: self.widget.label_task.text() == "done", timeout=30.0, msg="the example task")
        with self.check("Example / waiting pumps the event loop until the task completes"):
            self.assertEqual(self.widget.label_task.text(), "done")

    def test_a_failed_check_does_not_hide_the_ones_after_it(self):
        """The property the whole suite relies on, asserted rather than assumed.

        ``check`` has to keep going after a failure, otherwise a run reports one regression at a
        time. Verified against a throwaway ``TestCase`` so this test can pass while containing a
        deliberate failure.
        """
        observed = []

        class _Inner(AutomatedUITestBase):
            # The base lifecycle is deliberately replaced rather than run. This throwaway case
            # executes inside a live test, and the base setUp/tearDown act on process-global state:
            # they repoint the QSettings ini path and, on the way out, clear the ADS and close every
            # figure - pulling the enclosing test's own state out from under it. Nothing checked
            # here needs either. A test case copied from this module as a template should not
            # inherit that pattern.
            def setUp(inner_self):
                pass

            def tearDown(inner_self):
                pass

            def test_inner(inner_self):
                with inner_self.check("first"):
                    observed.append("first")
                    inner_self.fail("deliberate")
                with inner_self.check("second"):
                    observed.append("second")
                    raise RuntimeError("deliberate")
                observed.append("after")

        result = unittest.TestResult()
        _Inner("test_inner").run(result)

        with self.check("Example / every check runs even after one fails"):
            self.assertEqual(observed, ["first", "second", "after"])
        with self.check("Example / each failed check is reported separately"):
            self.assertEqual(len(result.failures) + len(result.errors), 2)
        with self.check("Example / a failed check names the step it belongs to"):
            self.assertIn("[first]", str(result.failures[0][0]))


if __name__ == "__main__":
    unittest.main()
