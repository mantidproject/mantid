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
        self.navigation = []

    def show_step(self, text, title="", chapter_name="", step_number=0, step_count=0):
        self.shown.append((chapter_name, step_number, step_count, title, text))

    def show_waiting(self, message):
        self.waiting.append(message)

    def place_beside(self, _spotlight):
        pass

    def set_navigation_enabled(self, back=True, next_=True):
        self.navigation.append((back, next_))

    def set_paused(self, paused):
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
        kwargs.setdefault("dwell_ms", 1)
        kwargs.setdefault("settle_ms", 1)
        return TutorialStep(text=text, **kwargs)

    def _recording_step(self, text, **kwargs):
        return self._step(text, action=lambda _ctx: self.performed.append(text), **kwargs)

    def _player(self, chapters):
        player = TutorialPlayer(chapters, self.context, self.overlay, self.bubble, parent=self.window)
        self.finished = []
        self.failures = []
        player.finished.connect(lambda: self.finished.append(True))
        player.step_failed.connect(lambda label, reason: self.failures.append((label, reason)))
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

    # ------------------------------------------------------------------ playing

    def test_it_plays_every_step_of_every_chapter_in_order(self):
        player = self._player(self._two_chapters())
        player.start()

        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(self.performed, ["load", "material", "plot"])
        self.assertEqual([entry[0] for entry in self.bubble.shown], ["Setup", "Setup", "Results"])
        self.assertEqual([(entry[1], entry[2]) for entry in self.bubble.shown], [(1, 2), (2, 2), (1, 1)])
        self.assertFalse(player.is_running)

    def test_it_spotlights_each_step_target(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.finished)))

        self.assertEqual(self.overlay.targets[:2], [self.first, self.second])

    def test_a_step_with_no_target_clears_the_spotlight_rather_than_leaving_the_last_one(self):
        chapters = (TutorialChapter(name="Only", steps=[self._step("a closing remark")]),)
        player = self._player(chapters)
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.finished)))

        self.assertEqual(self.overlay.targets, [None])

    def test_it_starts_at_a_named_chapter_without_fast_forwarding(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1)

        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(self.performed, ["plot"])

    def test_starting_beyond_the_last_chapter_is_refused(self):
        player = self._player(self._two_chapters())
        self.assertRaises(IndexError, player.start, 5)

    # ------------------------------------------------------------------ navigation

    def test_next_interrupts_the_dwell_without_skipping_a_step(self):
        # a long dwell, so nothing advances on its own during this test
        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[
                    self._recording_step("one", dwell_ms=100000),
                    self._recording_step("two", dwell_ms=100000),
                ],
            ),
        )
        player = self._player(chapters)
        player.start()
        self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) == 1))

        player.next_step()
        self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) == 2))

        self.assertEqual(self.performed, ["one", "two"])
        self.assertEqual(player.position, (0, 1))

    def test_back_re_narrates_without_re_running_the_action(self):
        # the crux of Back: pressing "Add orientation" a second time would add a second one
        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[
                    self._recording_step("one", dwell_ms=100000),
                    self._recording_step("two", dwell_ms=100000),
                ],
            ),
        )
        player = self._player(chapters)
        player.start()
        self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) == 1))
        player.next_step()
        self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) == 2))
        self.assertEqual(self.performed, ["one", "two"])

        player.back_step()
        interaction.process_events(3)

        self.assertEqual(player.position, (0, 0))
        self.assertEqual(self.bubble.shown[-1][4], "one")
        self.assertEqual(self.performed, ["one", "two"], "back must not perform anything again")

    def test_back_crosses_into_the_previous_chapter(self):
        chapters = (
            TutorialChapter(name="Setup", steps=[self._recording_step("one", dwell_ms=100000)]),
            TutorialChapter(name="Results", steps=[self._recording_step("two", dwell_ms=100000)]),
        )
        player = self._player(chapters)
        player.start()
        self.assertTrue(self._pump_until(lambda: len(self.bubble.shown) == 1))
        player.next_step()
        self.assertTrue(self._pump_until(lambda: player.position == (1, 0)))

        player.back_step()
        interaction.process_events(3)

        self.assertEqual(player.position, (0, 0))

    def test_back_at_the_very_start_does_nothing(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        player.set_paused(True)

        player.back_step()
        interaction.process_events(3)

        self.assertEqual(player.position, (0, 0))

    def test_back_is_disabled_only_on_the_first_step(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.finished)))

        self.assertEqual([entry[0] for entry in self.bubble.navigation], [False, True, True])

    # ------------------------------------------------------------------ pausing

    def test_pausing_holds_the_current_step(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        player.set_paused(True)

        QTest.qWait(150)

        self.assertTrue(player.is_paused)
        self.assertEqual(player.position, (0, 0))
        self.assertFalse(self.finished)

    def test_resuming_carries_on_from_where_it_paused(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))
        player.set_paused(True)
        QTest.qWait(50)

        player.set_paused(False)

        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(self.performed, ["load", "material", "plot"])

    def test_stop_ends_the_tour_without_reporting_it_as_finished(self):
        player = self._player(self._two_chapters())
        player.start()
        self.assertTrue(self._pump_until(lambda: bool(self.bubble.shown)))

        player.stop()
        QTest.qWait(150)

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

        ready["now"] = True
        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(len(self.bubble.shown), 1)

    def test_a_wait_that_times_out_reports_it_and_carries_on(self):
        chapters = (
            TutorialChapter(
                name="Results",
                steps=[self._step("never ready", await_=lambda _ctx: False, await_timeout_s=0.05)],
            ),
        )
        player = self._player(chapters)
        player.start()

        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(len(self.failures), 1)
        self.assertIn("still not ready", self.failures[0][1])

    # ------------------------------------------------------------------ failure

    def test_a_failing_action_is_reported_and_the_tour_carries_on(self):
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

        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(self.failures, [("Broken step", "the button went away")])
        self.assertEqual(self.performed, ["after"], "the rest of the tour should still run")

    def test_an_unresolvable_target_is_reported_and_the_step_is_still_narrated(self):
        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[self._step("gone", target=lambda ctx: ctx["nope"], title="Missing target")],
            ),
        )
        player = self._player(chapters)
        player.start()

        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(len(self.failures), 1)
        self.assertIn("could not find what to highlight", self.failures[0][1])
        self.assertEqual(len(self.bubble.shown), 1)
        self.assertEqual(self.overlay.targets, [None])

    # ------------------------------------------------------------------ fast forward

    def test_fast_forward_runs_earlier_actions_without_narrating_them(self):
        player = self._player(self._two_chapters())
        player.start(chapter_index=1, fast_forward=True)

        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        # every action ran, so the interface is in the right state for the chapter jumped to...
        self.assertEqual(self.performed, ["load", "material", "plot"])
        # ...but only the chapter asked for was narrated
        self.assertEqual([entry[0] for entry in self.bubble.shown], ["Results"])
        self.assertIn("Setting the interface up", self.bubble.waiting[0])

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
        self.assertTrue(self._pump_until(lambda: bool(self.finished)))
        self.assertEqual(self.performed, ["slow", "after"])


if __name__ == "__main__":
    unittest.main()
