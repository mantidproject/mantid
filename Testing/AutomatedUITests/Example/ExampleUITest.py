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
    click,
    click_checkbox,
    combo_items,
    process_events,
    select_combo,
    set_checkbox,
    table_checkbox,
    table_column,
    wait_until,
)

# how long the fake background task takes. Long enough that a test which forgot to wait would see
# the un-updated value, short enough not to pad the suite.
_TASK_DURATION_MS = 250

_GREETINGS = ("Hello", "Goodbye", "Good morning")


def _build_widget():
    """The interface under test: one of every widget kind the helpers know how to drive.

    Built inside a function rather than at module scope because a ``QWidget`` cannot be constructed
    before the ``QApplication`` exists, and the harness creates that lazily in ``setUp``.
    """
    from qtpy.QtCore import QTimer
    from qtpy.QtWidgets import QCheckBox, QComboBox, QHBoxLayout, QLabel, QPushButton, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget

    class _ExampleWidget(QWidget):
        def __init__(self):
            super(_ExampleWidget, self).__init__()
            self.setObjectName("ExampleWidget")

            self.combo_greeting = QComboBox()
            self.combo_greeting.addItems(_GREETINGS)
            self.check_shout = QCheckBox("Shout")
            self.button_greet = QPushButton("Greet")
            self.label_greeting = QLabel("")
            self.button_start_task = QPushButton("Start task")
            self.label_task = QLabel("idle")

            # one row per greeting, each with a select box in a centred cell widget and a per-row
            # button - the awkward table layout the helpers exist to reach into
            self.table = QTableWidget(len(_GREETINGS), 3)
            self.table.setHorizontalHeaderLabels(["Select", "Greeting", "Action"])
            for row, greeting in enumerate(_GREETINGS):
                cell = QWidget()
                layout = QHBoxLayout(cell)
                layout.setContentsMargins(0, 0, 0, 0)
                layout.addWidget(QCheckBox())
                self.table.setCellWidget(row, 0, cell)
                self.table.setItem(row, 1, QTableWidgetItem(greeting))
                button = QPushButton("Use")
                button.clicked.connect(lambda _checked=False, text=greeting: self._set_greeting(text))
                self.table.setCellWidget(row, 2, button)

            layout = QVBoxLayout(self)
            for widget in (
                self.combo_greeting,
                self.check_shout,
                self.button_greet,
                self.label_greeting,
                self.button_start_task,
                self.label_task,
                self.table,
            ):
                layout.addWidget(widget)

            self.button_greet.clicked.connect(self._greet)
            self.button_start_task.clicked.connect(self._start_task)

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
