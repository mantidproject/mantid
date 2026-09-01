# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from mantidqt.widgets.tutorial.step import TutorialChapter, TutorialStep, total_steps, walk


class TutorialStepTest(unittest.TestCase):
    def test_a_step_must_say_something(self):
        for empty in ("", "   ", "\n"):
            with self.subTest(text=repr(empty)):
                self.assertRaises(ValueError, TutorialStep, text=empty)

    def test_negative_delays_are_rejected(self):
        self.assertRaises(ValueError, TutorialStep, text="hello", dwell_ms=-1)
        self.assertRaises(ValueError, TutorialStep, text="hello", settle_ms=-1)

    def test_awaiting_with_a_non_positive_timeout_is_rejected(self):
        self.assertRaises(ValueError, TutorialStep, text="hello", await_=lambda _ctx: True, await_timeout_s=0)

    def test_label_prefers_the_title(self):
        self.assertEqual(TutorialStep(text="anything", title="Load the shape").label, "Load the shape")

    def test_label_falls_back_to_flattened_text(self):
        self.assertEqual(TutorialStep(text="Pick an\n  instrument").label, "Pick an instrument")

    def test_label_truncates_a_long_text(self):
        label = TutorialStep(text="word " * 40).label
        self.assertLessEqual(len(label), 60)
        self.assertTrue(label.startswith("word word"))
        self.assertTrue(label.endswith("…"))

    def test_label_leaves_a_text_that_already_fits(self):
        text = "Pick the instrument you are planning for"
        self.assertEqual(TutorialStep(text=text).label, text)

    def test_target_is_resolved_against_the_context(self):
        step = TutorialStep(text="there", target=lambda context: context["button"])
        self.assertEqual(step.resolve_target({"button": "a widget"}), "a widget")

    def test_a_step_with_no_target_resolves_to_none(self):
        self.assertIsNone(TutorialStep(text="just narrating").resolve_target({}))

    def test_an_unresolvable_target_raises_rather_than_being_swallowed(self):
        # the interface having moved under the tour must be loud, not a step that quietly stops
        # highlighting
        step = TutorialStep(text="there", target=lambda context: context.gone_away)
        self.assertRaises(AttributeError, step.resolve_target, object())


class TutorialChapterTest(unittest.TestCase):
    @staticmethod
    def _step(text="a step"):
        return TutorialStep(text=text)

    def test_a_chapter_must_be_named(self):
        self.assertRaises(ValueError, TutorialChapter, name=" ", steps=[self._step()])

    def test_a_chapter_must_have_steps(self):
        self.assertRaises(ValueError, TutorialChapter, name="Setup", steps=[])

    def test_steps_are_frozen_into_a_tuple(self):
        mutable = [self._step("one")]
        chapter = TutorialChapter(name="Setup", steps=mutable)
        mutable.append(self._step("two"))
        self.assertEqual(len(chapter), 1)
        self.assertIsInstance(chapter.steps, tuple)

    def test_indexing_and_length(self):
        chapter = TutorialChapter(name="Setup", steps=[self._step("one"), self._step("two")])
        self.assertEqual(len(chapter), 2)
        self.assertEqual(chapter[1].text, "two")


class TutorialWalkTest(unittest.TestCase):
    def setUp(self):
        self.chapters = (
            TutorialChapter(name="First", steps=[TutorialStep(text="1a"), TutorialStep(text="1b")]),
            TutorialChapter(name="Second", steps=[TutorialStep(text="2a")]),
        )

    def test_total_steps_counts_across_chapters(self):
        self.assertEqual(total_steps(self.chapters), 3)

    def test_walk_visits_every_step_in_play_order(self):
        visited = [(chapter_index, step_index, step.text) for chapter_index, step_index, _chapter, step in walk(self.chapters)]
        self.assertEqual(visited, [(0, 0, "1a"), (0, 1, "1b"), (1, 0, "2a")])


if __name__ == "__main__":
    unittest.main()
