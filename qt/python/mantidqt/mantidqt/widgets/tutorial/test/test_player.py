# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtTest import QTest
from qtpy.QtWidgets import QGroupBox, QPushButton, QSpinBox, QVBoxLayout, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.player import TutorialPlayer
from mantidqt.widgets.tutorial.step import TutorialChapter, TutorialStep


class FakeAnnotator:
    """Records what the tour pointed at and said, so a test can check both without going near
    painting. The whole surface the player uses, so nothing has to be probed for."""

    def __init__(self):
        self.targets = []
        self.shown = []
        self.waiting = []
        self.kept_clear = ()

    def set_target(self, widget):
        self.targets.append(widget)

    def target_rect(self):
        return None

    def rect_of(self, _widget):
        return None

    def show_step(self, text, title=""):
        self.shown.append((title, text))

    def show_waiting(self, message):
        self.waiting.append(message)

    def place_beside(self, _spotlight, keep_clear=()):
        self.kept_clear = keep_clear

    def show(self):
        pass

    def hide(self):
        pass

    def detach(self):
        pass


@start_qapplication
class TutorialPlayerTest(unittest.TestCase):
    def setUp(self):
        self.window = QWidget()
        layout = QVBoxLayout(self.window)
        self.first = QPushButton("First")
        self.second = QPushButton("Second")
        layout.addWidget(self.first)
        layout.addWidget(self.second)
        self.window.resize(400, 300)
        self.window.show()
        interaction.process_events(3)

        self.performed = []
        self.annotator = FakeAnnotator()
        self.context = {"window": self.window, "first": self.first, "second": self.second}

    def tearDown(self):
        self.window.close()
        self.window.deleteLater()
        interaction.process_events()

    # ------------------------------------------------------------------ helpers

    def _step(self, text, **kwargs):
        kwargs.setdefault("settle_ms", 1)
        return TutorialStep(text=text, **kwargs)

    def _recording_step(self, text, **kwargs):
        return self._step(text, action=lambda _ctx: self.performed.append(text), **kwargs)

    def _player(self, chapters):
        player = TutorialPlayer(chapters, self.context, self.annotator, parent=self.window)
        self.finished = []
        self.failures = []
        self.busy = []
        self.applied = []
        player.finished.connect(lambda: self.finished.append(True))
        player.step_failed.connect(lambda label, reason: self.failures.append((label, reason)))
        player.busy_changed.connect(lambda busy, message: self.busy.append((busy, message)))
        player.step_applied.connect(lambda: self.applied.append(True))
        return player

    @staticmethod
    def _pump_until(predicate, timeout_ms=5000):
        waited = 0
        while not predicate() and waited < timeout_ms:
            QTest.qWait(5)
            waited += 5
        return predicate()

    def _two_chapters(self):
        return (
            TutorialChapter(
                name="Setup",
                steps=[
                    self._recording_step("load", target=lambda ctx: ctx["first"]),
                    self._recording_step("material", target=lambda ctx: ctx["second"]),
                ],
            ),
            TutorialChapter(name="Results", steps=[self._recording_step("plot")]),
        )

    def _started(self, chapters=None, **kwargs):
        """A player showing its first step and ready to be driven."""
        player = self._player(chapters or self._two_chapters())
        player.start(**kwargs)
        self._ready(player)
        return player

    def _ready(self, player):
        """Wait until the tour will accept navigation.

        The player ignores Back/Next/Show me while a step is settling or waiting, so a test that
        clicked immediately would be testing that guard rather than what it meant to.
        """
        self.assertTrue(self._pump_until(lambda: not player.is_busy))

    def _next(self, player):
        self._ready(player)
        player.next_step()

    def _apply(self, player):
        self._ready(player)
        player.apply_step()
        self._ready(player)

    def _play_to_the_end(self, player):
        """Press Next until the tour finishes, as a user would."""
        for _ in range(50):
            if self.finished:
                return True
            self._next(player)
        return bool(self.finished)

    # ------------------------------------------------------------------ explain first, act second

    def test_it_explains_a_step_without_doing_anything_to_the_interface(self):
        player = self._started()

        QTest.qWait(300)

        self.assertEqual(player.position, (0, 0))
        self.assertEqual(self.performed, [], "the step should be explained before it is performed")
        self.assertFalse(player.is_applied())
        self.assertEqual(len(self.annotator.shown), 1)
        self.assertFalse(self.finished)

    def test_show_me_performs_the_step_and_leaves_its_explanation_up(self):
        player = self._started()

        self._apply(player)

        self.assertEqual(self.applied, [True])
        self.assertEqual(self.performed, ["load"])
        self.assertEqual(player.position, (0, 0), "performing a step does not move off it")
        self.assertTrue(player.is_applied())
        self.assertEqual(self.annotator.shown[-1][1], "load", "the caption stays while the action is watched")

    def test_show_me_twice_does_not_perform_it_twice(self):
        # pressing "Add orientation" a second time would add a second one
        player = self._started()

        self._apply(player)
        player.apply_step()
        QTest.qWait(100)

        self.assertEqual(self.performed, ["load"])

    def test_a_step_with_no_action_is_already_applied(self):
        player = self._started((TutorialChapter(name="Only", steps=[self._step("just narrating")]),))
        self.assertFalse(player.current_step_has_action())
        self.assertTrue(player.is_applied())

    # ------------------------------------------------------------------ user-driven advance

    def test_next_performs_a_step_that_was_not_shown_before_moving_on(self):
        # chapters are cumulative, so a skipped action would break every later step
        player = self._started()

        self._next(player)

        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self.assertEqual(self.performed, ["load"])

    def test_next_after_show_me_does_not_perform_it_again(self):
        player = self._started()
        self._apply(player)

        self._next(player)

        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self.assertEqual(self.performed, ["load"])

    def test_next_crosses_into_the_following_chapter(self):
        player = self._started()
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))

        self._next(player)

        self.assertTrue(self._pump_until(lambda: player.position == (1, 0)))
        self.assertEqual(self.performed, ["load", "material"])

    def test_next_past_the_last_step_finishes_the_tour(self):
        player = self._started()

        self.assertTrue(self._play_to_the_end(player))

        self.assertEqual(self.performed, ["load", "material", "plot"], "every action ran on the way through")
        self.assertFalse(player.is_running)

    def test_it_spotlights_each_step_target(self):
        player = self._started()
        self._play_to_the_end(player)

        self.assertEqual(self.annotator.targets[:2], [self.first, self.second])

    def test_a_step_with_no_target_clears_the_spotlight_rather_than_leaving_the_last_one(self):
        self._started((TutorialChapter(name="Only", steps=[self._step("a closing remark")]),))
        self.assertEqual(self.annotator.targets, [None])

    def test_it_starts_at_a_named_chapter_without_fast_forwarding(self):
        player = self._started(chapter_index=1)

        self.assertEqual(player.position, (1, 0))
        self.assertEqual(self.performed, [], "the chapter's own step is explained, not performed")

    def test_starting_beyond_the_last_chapter_is_refused(self):
        player = self._player(self._two_chapters())
        self.assertRaises(IndexError, player.start, 5)

    def test_it_knows_when_it_is_at_the_ends_of_the_tour(self):
        player = self._started()
        self.assertTrue(player.at_start())
        self.assertFalse(player.at_end())

        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self.assertFalse(player.at_start())

        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (1, 0)))
        self.assertTrue(player.at_end())

    # ------------------------------------------------------------------ back

    def test_back_re_shows_the_previous_step_without_re_running_its_action(self):
        # the crux of Back: pressing "Add orientation" a second time would add a second one
        player = self._started()
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self.assertEqual(self.performed, ["load"])

        self._ready(player)
        player.back_step()
        self.assertTrue(self._pump_until(lambda: player.position == (0, 0)))
        # the position moves at once but the caption follows after the settle
        self._ready(player)

        self.assertEqual(self.annotator.shown[-1][1], "load")
        self.assertEqual(self.performed, ["load"], "back must not perform anything again")
        self.assertTrue(player.is_applied(), "the step it returned to has already been performed")

    def test_next_after_back_does_not_repeat_the_action(self):
        player = self._started()
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self._ready(player)
        player.back_step()
        self.assertTrue(self._pump_until(lambda: player.position == (0, 0)))

        self._next(player)

        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self.assertEqual(self.performed, ["load"])

    def test_back_crosses_into_the_previous_chapter(self):
        player = self._started()
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (1, 0)))

        self._ready(player)
        player.back_step()

        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))

    def test_back_at_the_very_start_does_nothing(self):
        player = self._started()

        player.back_step()
        interaction.process_events(3)

        self.assertEqual(player.position, (0, 0))

    # ------------------------------------------------------------------ revealing

    def test_the_target_is_opened_up_before_the_action_runs(self):
        # otherwise the value is set inside a section the user cannot see, and the box only opens
        # afterwards to explain a change they never watched happen
        group = QGroupBox("Section", self.window)
        group.setCheckable(True)
        group.setChecked(False)
        group_layout = QVBoxLayout(group)
        spin = QSpinBox()
        spin.setRange(0, 100)
        group_layout.addWidget(spin)
        self.window.layout().addWidget(group)
        interaction.process_events(3)
        # an unchecked checkable group box disables its contents
        self.assertFalse(spin.isEnabled())

        state_when_the_action_ran = {}

        def set_the_value(_ctx):
            state_when_the_action_ran["enabled"] = spin.isEnabled()
            spin.setValue(42)

        player = self._started(
            (TutorialChapter(name="Setup", steps=[self._step("set it", target=lambda _ctx: group, action=set_the_value)]),)
        )
        self.assertTrue(group.isChecked(), "the section should be open before the step is even explained")

        self._apply(player)

        self.assertTrue(state_when_the_action_ran["enabled"], "the section must be open before the action runs")
        self.assertEqual(spin.value(), 42)

    def test_a_target_that_only_appears_after_the_action_is_highlighted_once_it_is_shown(self):
        made = {}

        def create_it(_ctx):
            made["button"] = QPushButton("Made", self.window)
            made["button"].show()

        player = self._started(
            (TutorialChapter(name="Setup", steps=[self._step("appears", target=lambda _ctx: made["button"], action=create_it)]),)
        )
        # nothing to point at while the step is only being explained, and that is not a failure
        self.assertEqual(self.annotator.targets, [None])
        self.assertEqual(self.failures, [])

        self._apply(player)

        self.assertEqual(self.annotator.targets[-1], made["button"])
        self.assertEqual(self.failures, [])

    # ------------------------------------------------------------------ waiting

    def test_a_step_waits_for_its_action_to_finish_before_reporting_it_done(self):
        ready = {"now": False}
        player = self._started(
            (
                TutorialChapter(
                    name="Results",
                    steps=[
                        self._recording_step(
                            "calculate",
                            await_=lambda _ctx: ready["now"],
                            await_timeout_s=5.0,
                            await_text="Calculatingâ€¦",
                        )
                    ],
                ),
            )
        )

        player.apply_step()

        self.assertTrue(self._pump_until(lambda: bool(self.annotator.waiting)))
        self.assertEqual(self.annotator.waiting, ["Calculatingâ€¦"])
        self.assertTrue(player.is_busy)
        self.assertIn((True, "Calculatingâ€¦"), self.busy)
        self.assertEqual(self.applied, [], "not done until the work it started has finished")

        ready["now"] = True
        self.assertTrue(self._pump_until(lambda: bool(self.applied)))
        self.assertFalse(player.is_busy)
        self.assertEqual(self.annotator.shown[-1][1], "calculate", "the caption comes back after the wait")

    def test_navigation_is_ignored_while_a_step_is_working(self):
        # advancing mid-calculation would run the next step's action against an interface that had
        # not finished reacting to this one
        ready = {"now": False}
        player = self._started(
            (
                TutorialChapter(
                    name="Setup",
                    steps=[
                        self._recording_step("slow", await_=lambda _ctx: ready["now"], await_timeout_s=5.0),
                        self._recording_step("after"),
                    ],
                ),
            )
        )
        player.apply_step()
        self.assertTrue(self._pump_until(lambda: player.is_busy))

        player.next_step()
        player.back_step()
        QTest.qWait(100)

        self.assertEqual(player.position, (0, 0))
        self.assertEqual(self.performed, ["slow"])

        ready["now"] = True
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))

    def test_a_wait_that_times_out_reports_it_and_carries_on(self):
        player = self._started(
            (TutorialChapter(name="Results", steps=[self._recording_step("never ready", await_=lambda _ctx: False, await_timeout_s=0.05)]),)
        )

        player.apply_step()

        self.assertTrue(self._pump_until(lambda: bool(self.applied)))
        self.assertEqual(len(self.failures), 1)
        self.assertIn("still not ready", self.failures[0][1])
        self.assertFalse(player.is_busy, "a timed-out wait must not leave navigation locked")

    # ------------------------------------------------------------------ failure

    def test_a_failing_action_is_reported_and_the_tour_carries_on(self):
        def explode(_context):
            raise RuntimeError("the button went away")

        player = self._started(
            (
                TutorialChapter(
                    name="Setup",
                    steps=[self._step("broken", action=explode, title="Broken step"), self._recording_step("after")],
                ),
            )
        )

        self._apply(player)

        self.assertEqual(self.failures, [("Broken step", "the button went away")])
        self.assertTrue(player.is_applied(), "a step that failed is not retried")

        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))

    def test_a_target_still_missing_after_the_action_is_reported(self):
        player = self._started(
            (
                TutorialChapter(
                    name="Setup",
                    steps=[self._step("gone", target=lambda ctx: ctx["nope"], title="Missing target", action=lambda _ctx: None)],
                ),
            )
        )
        self.assertEqual(self.failures, [], "not a failure while the step is only being explained")

        self._apply(player)

        self.assertEqual(len(self.failures), 1)
        self.assertIn("could not find what to highlight", self.failures[0][1])

    # ------------------------------------------------------------------ stopping

    def test_stop_ends_the_tour_without_reporting_it_as_finished(self):
        player = self._started()

        player.stop()
        QTest.qWait(100)

        self.assertFalse(player.is_running)
        self.assertEqual(self.finished, [], "stopping is not the same as reaching the end")

    # ------------------------------------------------------------------ fast forward

    def test_fast_forward_runs_earlier_actions_without_narrating_them(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1, fast_forward=True)
        self._ready(player)

        # the earlier chapter's actions ran, so the interface is in the state this chapter assumes
        self.assertEqual(self.performed, ["load", "material"])
        # ...but only the chapter asked for was narrated, and its own step is not performed yet
        self.assertEqual(len(self.annotator.shown), 1)
        self.assertFalse(player.is_applied())
        self.assertIn("Setting the interface up", self.annotator.waiting[0])

    def test_fast_forward_locks_navigation_while_it_catches_up(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1, fast_forward=True)

        self.assertTrue(self.busy)
        self.assertEqual(self.busy[0][0], True)
        self._ready(player)
        self.assertEqual(self.busy[-1][0], False)

    def test_back_into_a_fast_forwarded_chapter_does_not_repeat_its_actions(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1, fast_forward=True)
        self._ready(player)
        self.assertEqual(self.performed, ["load", "material"])

        player.back_step()
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self.assertTrue(player.is_applied())

        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (1, 0)))
        self.assertEqual(self.performed, ["load", "material"])

    def test_fast_forward_awaits_an_earlier_chapters_wait(self):
        ready = {"now": False}
        chapters = (
            TutorialChapter(
                name="Slow",
                steps=[self._recording_step("slow", await_=lambda _ctx: ready["now"], await_timeout_s=5.0)],
            ),
            TutorialChapter(name="After", steps=[self._recording_step("after")]),
        )
        player = self._player(chapters)
        player.start(chapter_index=1, fast_forward=True)

        QTest.qWait(60)
        self.assertEqual(self.performed, ["slow"], "it should be held at the wait, not racing past it")
        self.assertTrue(player.is_busy)

        ready["now"] = True
        self._ready(player)
        self.assertEqual(player.position, (1, 0))


if __name__ == "__main__":
    unittest.main()
