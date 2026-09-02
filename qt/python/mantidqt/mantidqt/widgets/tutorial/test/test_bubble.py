# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtCore import QRect
from qtpy.QtWidgets import QAbstractButton, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.bubble import GAP, WIDTH, TutorialBubble


@start_qapplication
class TutorialBubbleTest(unittest.TestCase):
    def setUp(self):
        self.window = QWidget()
        self.window.resize(900, 700)
        self.window.show()
        interaction.process_events(3)

        self.bubble = TutorialBubble(self.window)
        self.bubble.show_step("Some explanation of a control.", title="A step")
        self.bubble.show()
        interaction.process_events(3)

    def tearDown(self):
        self.window.close()
        self.window.deleteLater()
        interaction.process_events()

    def _text_of(self, object_name):
        return self.window.findChild(QWidget, object_name).text()

    # ------------------------------------------------------------------ content

    def test_it_shows_the_step(self):
        self.assertEqual(self._text_of("tutorial_bubble_title"), "A step")
        self.assertEqual(self._text_of("tutorial_bubble_text"), "Some explanation of a control.")

    def test_an_untitled_step_hides_the_title_rather_than_leaving_a_gap(self):
        self.bubble.show_step("Just narrating.")
        interaction.process_events()
        self.assertFalse(self.window.findChild(QWidget, "tutorial_bubble_title").isVisible())

    def test_show_waiting_replaces_only_the_text(self):
        self.bubble.show_waiting("Calculating transmission…")
        self.assertEqual(self._text_of("tutorial_bubble_text"), "Calculating transmission…")
        self.assertEqual(self._text_of("tutorial_bubble_title"), "A step")

    def test_it_carries_no_controls(self):
        # the caption moves with the highlight, so a button on it would be somewhere different on
        # every step. Navigation lives in the shell, which stays put.
        self.assertEqual(self.bubble.findChildren(QAbstractButton), [])

    # ------------------------------------------------------------------ placement

    def _place(self, spotlight, keep_clear=()):
        self.bubble.place_beside(spotlight, keep_clear)
        interaction.process_events()
        # in the host's coordinates, which is what place_beside was given
        return QRect(self.bubble.pos(), self.bubble.size())

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

    def test_it_keeps_clear_of_what_the_step_is_demonstrating(self):
        # a step that ticks a check box while the values it changes are displayed elsewhere would
        # otherwise put the explanation on top of the evidence
        # the real shape of it: a check box near the top, the fields it changes below and to one
        # side, leaving the caption room on the other side
        spotlight = QRect(300, 40, 120, 30)
        evidence = QRect(60, 120, 300, 200)

        without = self._place(spotlight)
        self.assertTrue(without.intersects(evidence), "this is the placement the step has to avoid")

        placed = self._place(spotlight, keep_clear=[evidence])

        self.assertFalse(placed.intersects(evidence))
        self.assertFalse(placed.intersects(spotlight))
        self.assertTrue(self.window.rect().contains(placed))

    def test_an_empty_keep_clear_rect_is_ignored(self):
        spotlight = QRect(300, 40, 120, 30)
        self.assertEqual(self._place(spotlight, keep_clear=[QRect(), None]), self._place(spotlight))

    def test_it_settles_somewhere_even_when_everything_is_blocked(self):
        # a caption slightly in the way beats one off the edge of the window
        spotlight = QRect(300, 300, 40, 40)
        placed = self._place(spotlight, keep_clear=[self.window.rect()])
        self.assertTrue(self.window.rect().contains(placed))

    # ------------------------------------------------------------------ layout

    def test_the_box_is_no_taller_than_the_text_it_holds(self):
        # leftover height is what shows up as uneven padding around the caption
        self.bubble.show_step("A caption long enough that it has to wrap over several lines. " * 3, title="A step")
        interaction.process_events()

        title = self.window.findChild(QWidget, "tutorial_bubble_title")
        text = self.window.findChild(QWidget, "tutorial_bubble_text")
        content_bottom = max(title.geometry().bottom(), text.geometry().bottom())
        slack = self.bubble.height() - content_bottom

        self.assertGreaterEqual(slack, 0)
        self.assertLessEqual(slack, 20, f"{slack}px of unused height below the caption")

    def test_no_caption_is_clipped(self):
        # the box was coming up a line short and cutting off the last one: the frame's border eats
        # into both the width the text wraps in and the height it is given
        captions = (
            "Short.",
            "A caption that runs on for long enough to need wrapping over two lines or so.",
            "This is a working copy of the interface, loaded with a demo sample. Everything the "
            "tutorial does happens here — your own session is untouched.<br><br>"
            "Each step explains a control first. Press <b>Show me</b> to watch the tutorial use it, "
            "then <b>Next</b> to move on — nothing happens on its own. <b>Back</b> re-reads a step, "
            "and the tabs above jump to a chapter.",
        )
        for caption in captions:
            with self.subTest(caption=caption[:40]):
                self.bubble.show_step(caption, title="Welcome to the Texture Planner")
                interaction.process_events()

                text = self.window.findChild(QWidget, "tutorial_bubble_text")
                needed = text.heightForWidth(text.width())
                self.assertGreaterEqual(text.height(), needed, f"the caption is clipped by {needed - text.height()}px")
                self.assertLessEqual(text.geometry().bottom(), self.bubble.height(), "the caption runs past the box")

    def test_the_box_leaves_room_for_its_own_border(self):
        # the frame's border takes from both the width the text wraps in and the height it is
        # given. Missing from the height, the box lands 2px short of its contents; missing from the
        # width, the text wraps into a narrower space than was measured and gains a line
        self.bubble.show_step("A caption that runs on long enough to wrap over a couple of lines. " * 2, title="A step")
        interaction.process_events()

        title = self.window.findChild(QWidget, "tutorial_bubble_title")
        text = self.window.findChild(QWidget, "tutorial_bubble_text")
        margins = self.bubble.layout().contentsMargins()
        border = 2 * self.bubble.frameWidth()

        expected = border + margins.top() + margins.bottom() + title.height() + self.bubble.layout().spacing() + text.height()
        self.assertEqual(self.bubble.height(), expected)
        self.assertEqual(text.width(), WIDTH - margins.left() - margins.right() - border)

    def test_a_short_caption_gets_a_short_box(self):
        self.bubble.show_step("Short.", title="A step")
        interaction.process_events()
        short = self.bubble.height()

        self.bubble.show_step("A much longer caption that will certainly need to wrap. " * 4, title="A step")
        interaction.process_events()

        self.assertGreater(self.bubble.height(), short, "the box should grow with its text, not stay a fixed size")

    def test_the_heading_sits_against_the_top_margin(self):
        self.bubble.show_step("short", title="A step")
        interaction.process_events()

        title = self.window.findChild(QWidget, "tutorial_bubble_title")
        margin = self.bubble.layout().contentsMargins()
        # plus the styled frame's own border
        self.assertEqual(title.geometry().top(), margin.top() + self.bubble.frameWidth())

    def test_the_padding_above_and_below_the_caption_matches(self):
        # the complaint this fixes: uneven blank space around the text
        self.bubble.show_step("A caption that runs on long enough to wrap. " * 3, title="A step")
        interaction.process_events()

        title = self.window.findChild(QWidget, "tutorial_bubble_title")
        text = self.window.findChild(QWidget, "tutorial_bubble_text")
        above = title.geometry().top()
        below = self.bubble.height() - text.geometry().bottom()

        self.assertAlmostEqual(above, below, delta=2, msg=f"{above}px above the caption, {below}px below")

    def test_the_text_follows_the_heading_rather_than_floating(self):
        self.bubble.show_step("short", title="A step")
        interaction.process_events()

        title = self.window.findChild(QWidget, "tutorial_bubble_title")
        text = self.window.findChild(QWidget, "tutorial_bubble_text")
        gap = text.geometry().top() - title.geometry().bottom()
        self.assertLessEqual(gap, self.bubble.layout().spacing() + 2, "the two should stay together at the top")

    def test_with_no_spotlight_it_centres_itself(self):
        placed = self._place(QRect())
        self.assertAlmostEqual(placed.center().x(), self.window.rect().center().x(), delta=2)
        self.assertAlmostEqual(placed.center().y(), self.window.rect().center().y(), delta=2)


if __name__ == "__main__":
    unittest.main()
