# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtTest import QTest
from qtpy.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QGroupBox,
    QLabel,
    QLineEdit,
    QPushButton,
    QScrollArea,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction


@start_qapplication
class InteractionTest(unittest.TestCase):
    def setUp(self):
        self.widgets = []

    def tearDown(self):
        for widget in self.widgets:
            widget.close()
            widget.deleteLater()
        interaction.process_events()

    def _keep(self, widget):
        self.widgets.append(widget)
        return widget

    @staticmethod
    def _pump_until(predicate, timeout_ms=5000):
        """Let real time pass while the event loop runs, until ``predicate`` holds.

        ``processEvents`` on its own is not enough here: it returns the moment the queue is empty
        without any wall-clock time passing, so a loop around it never reaches a timer that is due
        in 100 ms. ``qWait`` sleeps *and* pumps, which is what timer-driven code needs to be
        observed. It is a test-only tool - the production code under test must never wait this way.
        """
        waited = 0
        while not predicate() and waited < timeout_ms:
            QTest.qWait(10)
            waited += 10
        return predicate()

    # ------------------------------------------------------------------ driving widgets

    def test_click_presses_a_button(self):
        button = self._keep(QPushButton("Go"))
        pressed = []
        button.clicked.connect(lambda: pressed.append(True))
        button.show()

        interaction.click(button)

        # synchronous on purpose: a press still pending when the tour ends would fire against an
        # interface that had already been torn down
        self.assertEqual(pressed, [True])
        self.assertFalse(button.isDown(), "the button must not be left held down")

    def test_click_refuses_a_non_button(self):
        self.assertRaises(TypeError, interaction.click, self._keep(QLabel("not a button")))

    def test_click_refuses_a_disabled_button(self):
        button = self._keep(QPushButton("Go"))
        button.setEnabled(False)
        self.assertRaises(RuntimeError, interaction.click, button)

    def test_set_check_state_takes_effect_immediately(self):
        box = self._keep(QCheckBox("Include"))
        clicks = []
        box.clicked.connect(lambda *_: clicks.append(True))
        box.show()

        # synchronous, unlike click(): the state holds on return, and handlers on ``clicked`` ran
        self.assertTrue(interaction.set_check_state(box, True))
        self.assertEqual(len(clicks), 1)

    def test_set_check_state_is_idempotent(self):
        box = self._keep(QCheckBox("Include"))
        clicks = []
        box.clicked.connect(lambda *_: clicks.append(True))
        box.show()

        self.assertTrue(interaction.set_check_state(box, True))
        self.assertTrue(interaction.set_check_state(box, True))
        self.assertEqual(len(clicks), 1, "asking for the state it is already in should not click it again")

    def test_set_check_state_toggles_a_group_box(self):
        group = self._keep(QGroupBox("Section"))
        group.setCheckable(True)
        group.setChecked(False)
        toggles = []
        group.toggled.connect(toggles.append)

        interaction.set_check_state(group, True)

        self.assertTrue(group.isChecked())
        self.assertEqual(toggles, [True])

    def test_select_combo_by_text(self):
        combo = self._keep(QComboBox())
        combo.addItems(["ENGINX", "IMAT", "POLDI"])
        self.assertEqual(interaction.select_combo(combo, "IMAT"), 1)
        self.assertEqual(combo.currentText(), "IMAT")

    def test_select_combo_reports_what_was_available(self):
        combo = self._keep(QComboBox())
        combo.addItems(["ENGINX", "IMAT"])
        with self.assertRaises(ValueError) as caught:
            interaction.select_combo(combo, "WISH")
        self.assertIn("ENGINX", str(caught.exception))

    def test_select_tab_by_title(self):
        tabs = self._keep(QTabWidget())
        first, second = QWidget(), QWidget()
        tabs.addTab(first, "Sample Setup")
        tabs.addTab(second, "Experimental Setup")

        self.assertIs(interaction.select_tab(tabs, "Experimental Setup"), second)
        self.assertEqual(tabs.currentIndex(), 1)

    def test_select_tab_reports_what_was_available(self):
        tabs = self._keep(QTabWidget())
        tabs.addTab(QWidget(), "Sample Setup")
        with self.assertRaises(ValueError) as caught:
            interaction.select_tab(tabs, "Nope")
        self.assertIn("Sample Setup", str(caught.exception))

    def test_set_text_emits_what_typing_would(self):
        line_edit = self._keep(QLineEdit())
        seen = {"edited": [], "finished": 0, "returned": 0}
        line_edit.textEdited.connect(seen["edited"].append)
        line_edit.editingFinished.connect(lambda: seen.__setitem__("finished", seen["finished"] + 1))
        line_edit.returnPressed.connect(lambda: seen.__setitem__("returned", seen["returned"] + 1))

        self.assertEqual(interaction.set_text(line_edit, "0,1,0"), "0,1,0")

        # setText alone emits none of these, which is the whole reason this helper exists
        self.assertEqual(seen["edited"], ["0,1,0"])
        self.assertEqual(seen["finished"], 1)
        self.assertEqual(seen["returned"], 1)

    def test_set_text_refuses_a_non_line_edit(self):
        self.assertRaises(TypeError, interaction.set_text, self._keep(QLabel()), "x")

    def test_set_spin_box(self):
        spin = self._keep(QDoubleSpinBox())
        spin.setRange(-180.0, 180.0)
        self.assertEqual(interaction.set_spin_box(spin, 45.0), 45.0)

    def test_set_spin_box_rejects_a_clamped_value(self):
        spin = self._keep(QDoubleSpinBox())
        spin.setRange(0.0, 10.0)
        with self.assertRaises(ValueError) as caught:
            interaction.set_spin_box(spin, 99.0)
        self.assertIn("range is 0.0 to 10.0", str(caught.exception))

    # ------------------------------------------------------------------ ensure_visible

    def _nested_window(self):
        """A target buried the way a real interface buries one: on the second tab, inside a
        collapsed group box, inside a scroll area."""
        window = self._keep(QWidget())
        layout = QVBoxLayout(window)
        tabs = QTabWidget()
        layout.addWidget(tabs)

        tabs.addTab(QLabel("first page"), "First")

        page = QWidget()
        page_layout = QVBoxLayout(page)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        page_layout.addWidget(scroll)

        contents = QWidget()
        contents_layout = QVBoxLayout(contents)
        # tall filler so the target starts well below the visible part of the scroll area
        for index in range(30):
            contents_layout.addWidget(QLabel(f"filler {index}"))
        group = QGroupBox("Collapsed section")
        group.setCheckable(True)
        group.setChecked(False)
        group_layout = QVBoxLayout(group)
        target = QPushButton("Target")
        group_layout.addWidget(target)
        contents_layout.addWidget(group)
        scroll.setWidget(contents)

        tabs.addTab(page, "Second")
        tabs.setCurrentIndex(0)

        window.resize(400, 300)
        window.show()
        interaction.process_events(3)
        return window, tabs, group, target

    def test_ensure_visible_selects_the_tab_expands_the_group_and_scrolls(self):
        window, tabs, group, target = self._nested_window()
        self.assertFalse(target.isVisible())

        shown = interaction.ensure_visible(target)

        self.assertEqual(tabs.currentIndex(), 1)
        self.assertTrue(group.isChecked())
        self.assertTrue(shown)
        self.assertTrue(target.isVisible())

    def test_ensure_visible_opens_a_collapsed_group_box_it_is_pointed_at(self):
        # pointing at a shut box while describing what is inside it shows the user nothing
        window, tabs, group, target = self._nested_window()

        interaction.ensure_visible(group)

        self.assertTrue(group.isChecked())
        self.assertTrue(target.isVisible(), "the contents should be on show, not just the box")

    def test_ensure_visible_does_not_change_the_page_of_a_tab_widget_it_is_pointed_at(self):
        # a step describing the tab bar itself should not have the interface move under it
        window, tabs, _group, _target = self._nested_window()
        self.assertEqual(tabs.currentIndex(), 0)

        interaction.ensure_visible(tabs)

        self.assertEqual(tabs.currentIndex(), 0)

    def test_ensure_visible_leaves_an_already_visible_widget_alone(self):
        button = self._keep(QPushButton("Plain"))
        button.show()
        interaction.process_events()
        self.assertTrue(interaction.ensure_visible(button))

    def test_ancestors_are_innermost_first(self):
        window, tabs, group, target = self._nested_window()
        chain = interaction.ancestors(target)
        self.assertIs(chain[0], group)
        self.assertIs(chain[-1], window)

    # ------------------------------------------------------------------ wait_for

    def test_wait_for_calls_back_immediately_when_already_true(self):
        called = []
        handle = interaction.wait_for(lambda: True, lambda: called.append("ready"), timeout_s=1.0)
        self.assertEqual(called, ["ready"])
        self.assertIsNone(handle, "no timer should be started when the predicate already holds")

    def test_wait_for_does_not_block_and_fires_when_the_predicate_turns_true(self):
        state = {"ready": False}
        called = []
        holder = self._keep(QWidget())

        handle = interaction.wait_for(lambda: state["ready"], lambda: called.append("ready"), timeout_s=5.0, interval_ms=10, parent=holder)
        # control came straight back, which is the point: a blocking wait would have hung here
        self.assertEqual(called, [])
        self.assertTrue(handle.isActive())

        state["ready"] = True
        self.assertTrue(self._pump_until(lambda: bool(called)))

        self.assertEqual(called, ["ready"])
        self.assertFalse(handle.isActive())

    def test_wait_for_calls_on_timeout_rather_than_on_ready(self):
        called = []
        holder = self._keep(QWidget())

        interaction.wait_for(
            lambda: False,
            lambda: called.append("ready"),
            timeout_s=0.05,
            on_timeout=lambda: called.append("timeout"),
            interval_ms=10,
            parent=holder,
        )
        self.assertTrue(self._pump_until(lambda: bool(called)))
        self.assertEqual(called, ["timeout"])

    def test_wait_for_rejects_a_non_positive_timeout(self):
        self.assertRaises(ValueError, interaction.wait_for, lambda: False, lambda: None, 0)


if __name__ == "__main__":
    unittest.main()
