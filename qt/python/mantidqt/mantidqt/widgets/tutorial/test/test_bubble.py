# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtCore import QRect
from qtpy.QtWidgets import QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.bubble import GAP, TutorialBubble


@start_qapplication
class TutorialBubbleTest(unittest.TestCase):
    def setUp(self):
        self.window = QWidget()
        self.window.resize(900, 700)
        self.window.show()
        interaction.process_events(3)

        self.bubble = TutorialBubble(self.window)
        self.bubble.show_step("Some explanation of a control.", title="A step", chapter_name="Setup", step_number=2, step_count=5)
        self.bubble.show()
        interaction.process_events(3)

    def tearDown(self):
        self.window.close()
        self.window.deleteLater()
        interaction.process_events()

    def _text_of(self, object_name):
        return self.window.findChild(QWidget, object_name).text()

    # ------------------------------------------------------------------ content

    def test_it_shows_the_step_and_its_position_in_the_chapter(self):
        self.assertEqual(self._text_of("tutorial_bubble_title"), "A step")
        self.assertEqual(self._text_of("tutorial_bubble_text"), "Some explanation of a control.")
        self.assertEqual(self._text_of("tutorial_bubble_chapter"), "Setup — step 2 of 5")

    def test_an_untitled_step_hides_the_title_rather_than_leaving_a_gap(self):
        self.bubble.show_step("Just narrating.", chapter_name="Setup", step_number=1, step_count=3)
        interaction.process_events()
        self.assertFalse(self.window.findChild(QWidget, "tutorial_bubble_title").isVisible())

    def test_a_message_with_no_step_number_hides_the_progress_line(self):
        self.bubble.show_step("A closing remark.")
        interaction.process_events()
        self.assertFalse(self.window.findChild(QWidget, "tutorial_bubble_chapter").isVisible())

    def test_show_waiting_replaces_only_the_text(self):
        self.bubble.show_waiting("Calculating transmission…")
        self.assertEqual(self._text_of("tutorial_bubble_text"), "Calculating transmission…")
        self.assertEqual(self._text_of("tutorial_bubble_title"), "A step")

    # ------------------------------------------------------------------ controls

    def test_each_button_emits_its_signal(self):
        for button, signal_name in (
            (self.bubble.btn_back, "back_requested"),
            (self.bubble.btn_next, "next_requested"),
            (self.bubble.btn_chapters, "chapters_requested"),
            (self.bubble.btn_close, "close_requested"),
        ):
            with self.subTest(button=button.text()):
                fired = []
                getattr(self.bubble, signal_name).connect(lambda: fired.append(True))
                button.click()
                interaction.process_events()
                self.assertEqual(fired, [True])

    def test_pause_reports_its_new_state_and_renames_itself(self):
        states = []
        self.bubble.pause_toggled.connect(states.append)

        self.bubble.btn_pause.click()
        interaction.process_events()
        self.assertEqual(states, [True])
        self.assertEqual(self.bubble.btn_pause.text(), "Resume")

        self.bubble.btn_pause.click()
        interaction.process_events()
        self.assertEqual(states, [True, False])
        self.assertEqual(self.bubble.btn_pause.text(), "Pause")

    def test_set_paused_does_not_echo_back_to_the_player(self):
        # the player calls this to reflect its own state; emitting would drive it back round
        states = []
        self.bubble.pause_toggled.connect(states.append)

        self.bubble.set_paused(True)
        interaction.process_events()

        self.assertEqual(states, [])
        self.assertTrue(self.bubble.btn_pause.isChecked())
        self.assertEqual(self.bubble.btn_pause.text(), "Resume")

    def test_navigation_can_be_disabled_at_the_ends_of_the_tour(self):
        self.bubble.set_navigation_enabled(back=False, next_=True)
        self.assertFalse(self.bubble.btn_back.isEnabled())
        self.assertTrue(self.bubble.btn_next.isEnabled())

    # ------------------------------------------------------------------ placement

    def _place(self, spotlight):
        self.bubble.place_beside(spotlight)
        interaction.process_events()
        return self.bubble.geometry_in_host()

    def test_it_sits_below_a_spotlight_near_the_top(self):
        spotlight = QRect(300, 40, 200, 60)
        placed = self._place(spotlight)
        self.assertGreaterEqual(placed.top(), spotlight.bottom())
        self.assertFalse(placed.intersects(spotlight))

    def test_it_moves_above_a_spotlight_with_no_room_below(self):
        spotlight = QRect(300, 560, 200, 120)
        placed = self._place(spotlight)
        self.assertLessEqual(placed.bottom(), spotlight.top())
        self.assertFalse(placed.intersects(spotlight))

    def test_it_goes_beside_a_spotlight_that_spans_the_window(self):
        spotlight = QRect(20, 10, 200, 680)
        placed = self._place(spotlight)
        self.assertFalse(placed.intersects(spotlight))
        self.assertGreaterEqual(placed.left(), spotlight.right())

    def test_it_stays_inside_the_window_whatever_it_is_beside(self):
        for spotlight in (
            QRect(0, 0, 10, 10),
            QRect(880, 680, 10, 10),
            QRect(-50, 300, 40, 40),
            QRect(860, -20, 60, 60),
        ):
            with self.subTest(spotlight=spotlight):
                placed = self._place(spotlight)
                self.assertTrue(
                    self.window.rect().contains(placed),
                    f"{placed} is not inside {self.window.rect()}",
                )
                self.assertGreaterEqual(placed.left(), GAP)

    def test_with_no_spotlight_it_centres_itself(self):
        placed = self._place(QRect())
        self.assertAlmostEqual(placed.center().x(), self.window.rect().center().x(), delta=2)
        self.assertAlmostEqual(placed.center().y(), self.window.rect().center().y(), delta=2)


if __name__ == "__main__":
    unittest.main()
