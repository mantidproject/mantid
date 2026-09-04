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
from mantidqt.widgets.tutorial.bubble import GAP, TutorialBubble


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
        # and it is the *preferred* position it settles on - below - not whichever was tried last
        self.assertGreaterEqual(placed.top(), spotlight.bottom())

    # ------------------------------------------------------------------ layout

    # The box sizes itself to its text rather than leaving it to the layout, because a word-wrapped
    # QLabel reports a size hint for a single line. Get that wrong in one direction and the last
    # line of the caption is cut off; wrong in the other and there is a strip of dead space under
    # it. The two tests below are those two failures.

    #: captions of the shapes the tour actually uses: one line, a couple, and the long welcome step
    CAPTIONS = (
        "Short.",
        "A caption that runs on for long enough to need wrapping over two lines or so.",
        "This is a working copy of the interface, loaded with a demo sample. Everything the "
        "tutorial does happens here — your own session is untouched.<br><br>"
        "Each step explains a control first. Press <b>Show me</b> to watch the tutorial use it, "
        "then <b>Next</b> to move on — nothing happens on its own. <b>Back</b> re-reads a step, "
        "and the tabs above jump to a chapter.",
    )

    def test_no_caption_is_clipped(self):
        for caption in self.CAPTIONS:
            with self.subTest(caption=caption[:40]):
                self.bubble.show_step(caption, title="Welcome to the Texture Planner")
                interaction.process_events()

                text = self.window.findChild(QWidget, "tutorial_bubble_text")
                needed = text.heightForWidth(text.width())
                self.assertGreaterEqual(text.height(), needed, f"the caption is clipped by {needed - text.height()}px")
                self.assertLessEqual(text.geometry().bottom(), self.bubble.height(), "the caption runs past the box")

    def test_the_box_is_no_taller_than_the_caption_it_holds(self):
        heights = []
        for caption in self.CAPTIONS:
            with self.subTest(caption=caption[:40]):
                self.bubble.show_step(caption, title="A step")
                interaction.process_events()
                heights.append(self.bubble.height())

                text = self.window.findChild(QWidget, "tutorial_bubble_text")
                slack = self.bubble.height() - text.geometry().bottom()
                self.assertGreaterEqual(slack, 0)
                self.assertLessEqual(slack, 20, f"{slack}px of unused height below the caption")

        self.assertEqual(heights, sorted(heights), "the box should grow with its text, not stay a fixed size")

    def test_with_no_spotlight_it_centres_itself(self):
        placed = self._place(QRect())
        self.assertAlmostEqual(placed.center().x(), self.window.rect().center().x(), delta=2)
        self.assertAlmostEqual(placed.center().y(), self.window.rect().center().y(), delta=2)


if __name__ == "__main__":
    unittest.main()
