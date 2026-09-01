# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtTest import QTest
from qtpy.QtWidgets import QPushButton, QVBoxLayout, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.player import TutorialPlayer
from mantidqt.widgets.tutorial.step import TutorialChapter, TutorialStep


class FakeOverlay:
    """Records what it was asked to spotlight, so a test can check the tour pointed at the right
    thing without going near painting."""

    def __init__(self):
        self.targets = []

    def set_target(self, widget):
        self.targets.append(widget)

    def target_rect(self):
        return None

    def show(self):
        pass

    def hide(self):
        pass

    def detach(self):
        pass


class FakeBubble:
    def __init__(self):
        self.shown = []
        self.waiting = []

    def show_step(self, text, title=""):
        self.shown.append((title, text))

    def show_waiting(self, message):
        self.waiting.append(message)

    def place_beside(self, _spotlight):
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
        self.overlay = FakeOverlay()
        self.bubble = FakeBubble()
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
        player = TutorialPlayer(chapters, self.context, self.overlay, self.bubble, parent=self.window)
        self.finished = []
        self.failures = []
        self.busy = []
        player.finished.connect(lambda: self.finished.append(True))
        player.step_failed.connect(lambda label, reason: self.failures.append((label, reason)))
        player.busy_changed.connect(lambda busy, message: self.busy.append((busy, message)))
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

    def _next(self, player):
        """Wait until the current step is on screen, then press Next - as a user would.

        Pressing it before the step has settled is deliberately ignored by the player, so a test
        that clicked immediately would be testing that guard rather than what it meant to.
        """
        self.assertTrue(self._pump_until(lambda: not player.is_busy))
        player.next_step()

    def _play_to_the_end(self, player):
        """Press Next until the tour reports it has finished, as a user would.

        Waits for each step to actually be on screen first - the player refuses to advance until
        then, so pressing Next early would simply be ignored and the loop would spin.
        """
        seen = 0
        for _ in range(50):
            if self.finished:
                return True
            self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) > seen))
            seen = len(self.bubble.shown)
            player.next_step()
        return bool(self.finished)

    # ------------------------------------------------------------------ user-driven advance

    def test_it_does_not_advance_on_its_own(self):
        # the whole point of the redesign: no timer is counting down behind the caption
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))

        QTest.qWait(400)

        self.assertEqual(player.position, (0, 0))
        self.assertEqual(self.performed, ["load"], "only the first step's action should have run")
        self.assertEqual(len(self.bubble.shown), 1)
        self.assertFalse(self.finished)

    def test_next_moves_one_step_at_a_time(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))

        player.next_step()
        self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) == 2))
        self.assertEqual(player.position, (0, 1))
        self.assertEqual(self.performed, ["load", "material"])

    def test_next_during_the_settle_is_ignored_rather_than_skipping_the_caption(self):
        # otherwise a step's action would have run while its explanation was never shown
        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[self._recording_step("one", settle_ms=200), self._recording_step("two", settle_ms=200)],
            ),
        )
        player = self._player(chapters)
        player.start()
        self.assertTrue(player.is_busy, "the action has run but the caption is not up yet")

        player.next_step()

        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self.assertEqual(player.position, (0, 0))
        self.assertEqual(self.bubble.shown[-1][1], "one")
        self.assertEqual(self.performed, ["one"])

    def test_next_crosses_into_the_following_chapter(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))

        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (1, 0)))
        self.assertEqual(self.performed, ["load", "material", "plot"])

    def test_next_past_the_last_step_finishes_the_tour(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._play_to_the_end(player))

        self.assertEqual(self.performed, ["load", "material", "plot"])
        self.assertFalse(player.is_running)

    def test_it_spotlights_each_step_target(self):
        player = self._player(self._two_chapters())
        player.start()
        self._play_to_the_end(player)

        self.assertEqual(self.overlay.targets[:2], [self.first, self.second])

    def test_a_step_with_no_target_clears_the_spotlight_rather_than_leaving_the_last_one(self):
        chapters = (TutorialChapter(name="Only", steps=[self._step("a closing remark")]),)
        player = self._player(chapters)
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))

        self.assertEqual(self.overlay.targets, [None])

    def test_it_starts_at_a_named_chapter_without_fast_forwarding(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1)
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))

        self.assertEqual(self.performed, ["plot"])

    def test_starting_beyond_the_last_chapter_is_refused(self):
        player = self._player(self._two_chapters())
        self.assertRaises(IndexError, player.start, 5)

    def test_it_knows_when_it_is_at_the_ends_of_the_tour(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self.assertTrue(player.at_start())
        self.assertFalse(player.at_end())

        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self.assertFalse(player.at_start())

        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (1, 0)))
        self.assertTrue(player.at_end())

    # ------------------------------------------------------------------ back

    def test_back_re_narrates_without_re_running_the_action(self):
        # the crux of Back: pressing "Add orientation" a second time would add a second one
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        player.next_step()
        self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) == 2))
        self.assertEqual(self.performed, ["load", "material"])

        player.back_step()
        interaction.process_events(3)

        self.assertEqual(player.position, (0, 0))
        self.assertEqual(self.bubble.shown[-1][1], "load")
        self.assertEqual(self.performed, ["load", "material"], "back must not perform anything again")

    def test_back_crosses_into_the_previous_chapter(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self._next(player)
        self.assertTrue(self._pump_until(lambda: player.position == (0, 1)))
        self._next(player)
        self.assertTrue(self._pump_until(lambda: not player.is_busy))

        player.back_step()
        interaction.process_events(3)

        self.assertEqual(player.position, (0, 1))

    def test_back_at_the_very_start_does_nothing(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))

        player.back_step()
        interaction.process_events(3)

        self.assertEqual(player.position, (0, 0))

    # ------------------------------------------------------------------ stopping

    def test_stop_ends_the_tour_without_reporting_it_as_finished(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))

        player.stop()
        QTest.qWait(100)

        self.assertFalse(player.is_running)
        self.assertEqual(self.finished, [], "stopping is not the same as reaching the end")

    # ------------------------------------------------------------------ waiting

    def test_a_step_waits_for_its_predicate_before_narrating(self):
        ready = {"now": False}
        chapters = (
            TutorialChapter(
                name="Results",
                steps=[
                    self._step(
                        "the calculation has finished",
                        await_=lambda _ctx: ready["now"],
                        await_timeout_s=5.0,
                        await_text="Calculating…",
                    )
                ],
            ),
        )
        player = self._player(chapters)
        player.start()

        self.assertTrue(self._pump_until(lambda: bool(self.bubble.waiting)))
        self.assertEqual(self.bubble.waiting, ["Calculating…"])
        self.assertEqual(self.bubble.shown, [], "it should not narrate until the wait is over")
        self.assertTrue(player.is_busy)
        # busy is raised as soon as the step starts, then re-announced with the wait's own message
        self.assertEqual(self.busy[0][0], True)
        self.assertIn((True, "Calculating…"), self.busy)

        ready["now"] = True
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self.assertFalse(player.is_busy)
        self.assertEqual(self.busy[-1][0], False)

    def test_next_is_ignored_while_a_step_is_waiting(self):
        # advancing mid-wait would run the following action against an interface that had not
        # finished reacting to this one
        ready = {"now": False}
        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[
                    self._recording_step("slow", await_=lambda _ctx: ready["now"], await_timeout_s=5.0),
                    self._recording_step("after"),
                ],
            ),
        )
        player = self._player(chapters)
        player.start()
        self.assertTrue(self._pump_until(lambda: player.is_busy))

        player.next_step()
        player.back_step()
        QTest.qWait(100)

        self.assertEqual(player.position, (0, 0))
        self.assertEqual(self.performed, ["slow"])

        ready["now"] = True
        self.assertTrue(self._pump_until(lambda: not player.is_busy))
        player.next_step()
        self.assertTrue(self._pump_until(lambda: self.performed == ["slow", "after"]))

    def test_a_wait_that_times_out_reports_it_and_carries_on(self):
        chapters = (
            TutorialChapter(
                name="Results",
                steps=[self._step("never ready", await_=lambda _ctx: False, await_timeout_s=0.05)],
            ),
        )
        player = self._player(chapters)
        player.start()

        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self.assertEqual(len(self.failures), 1)
        self.assertIn("still not ready", self.failures[0][1])
        self.assertFalse(player.is_busy, "a timed-out wait must not leave navigation locked")

    # ------------------------------------------------------------------ failure

    def test_a_failing_action_is_reported_and_the_step_is_still_narrated(self):
        def explode(_context):
            raise RuntimeError("the button went away")

        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[self._step("broken", action=explode, title="Broken step"), self._recording_step("after")],
            ),
        )
        player = self._player(chapters)
        player.start()

        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self.assertEqual(self.failures, [("Broken step", "the button went away")])

        player.next_step()
        self.assertTrue(self._pump_until(lambda: self.performed == ["after"]), "the rest of the tour should still run")

    def test_an_unresolvable_target_is_reported_and_the_step_is_still_narrated(self):
        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[self._step("gone", target=lambda ctx: ctx["nope"], title="Missing target")],
            ),
        )
        player = self._player(chapters)
        player.start()

        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self.assertEqual(len(self.failures), 1)
        self.assertIn("could not find what to highlight", self.failures[0][1])
        self.assertEqual(self.overlay.targets, [None])

    # ------------------------------------------------------------------ fast forward

    def test_fast_forward_runs_earlier_actions_without_narrating_them(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1, fast_forward=True)

        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        # every action ran, so the interface is in the right state for the chapter jumped to...
        self.assertEqual(self.performed, ["load", "material", "plot"])
        # ...but only the chapter asked for was narrated
        self.assertEqual(len(self.bubble.shown), 1)
        self.assertIn("Setting the interface up", self.bubble.waiting[0])
        self.assertFalse(player.is_busy)

    def test_fast_forward_locks_navigation_while_it_catches_up(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1, fast_forward=True)

        self.assertTrue(self.busy)
        self.assertEqual(self.busy[0][0], True)
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        self.assertEqual(self.busy[-1][0], False)

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

        ready["now"] = True
        self.assertTrue(self._pump_until(lambda: self.performed == ["slow", "after"]))


if __name__ == "__main__":
    unittest.main()
