# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Plays the whole tutorial against a real Texture Planner.

The tour is written against widget names and presenter methods, so it drifts the moment either is
renamed - and it drifts silently, because nothing else imports it. This is what makes that loud:
the tour is played end to end, and every step has to perform and to find what it points at.

The observations are in ``subTest`` blocks so one broken step reports itself without hiding the
rest. Building the sandbox is not - a failure there makes every following observation meaningless.
"""

import os
import unittest
from dataclasses import replace

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ.setdefault("MPLBACKEND", "Agg")

from mantid.api import AnalysisDataService as ADS, FrameworkManager
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial.interaction import process_events
from mantidqt.widgets.tutorial.player import TutorialPlayer
from mantidqt.widgets.tutorial.step import walk

from mantidqtinterfaces.TexturePlanner.tutorial.chapters import CHAPTERS
from mantidqtinterfaces.TexturePlanner.tutorial.sandbox import TutorialSandbox

# the tour waits for the user at every step; a test drives it by pressing Next as fast as each
# step becomes ready. The settle is cut to the minimum, which exercises the same steps in the same
# order without the pauses that only exist to let a layout catch up on screen.
FAST = {"settle_ms": 1}

PLAY_TIMEOUT_MS = 300000


class RecordingAnnotator:
    """Collects what the tour pointed at and said, instead of painting any of it."""

    def __init__(self):
        self.targets = []
        self.shown = []

    def set_target(self, widget):
        self.targets.append(widget)

    def target_rect(self):
        return None

    def rect_of(self, _widget):
        return None

    def show_step(self, text, title=""):
        self.shown.append((title, text))

    def show_waiting(self, message):
        pass

    def place_beside(self, _spotlight, keep_clear=()):
        pass

    def show(self):
        pass

    def hide(self):
        pass

    def detach(self):
        pass


def _hurried_chapters():
    """The real tour, with only the pauses taken out.

    ``replace`` rather than rebuilding each step field by field: a step written by hand here would
    quietly stop matching the real one the moment ``TutorialStep`` gained a field, and the test
    would be playing something subtly different from what ships.
    """
    return tuple(replace(chapter, steps=[replace(step, **FAST) for step in chapter.steps]) for chapter in CHAPTERS)


@start_qapplication
class TutorialChaptersTest(unittest.TestCase):
    """The whole tour played once, and then each chapter entered on its own - because a chapter is
    what the user can jump to, and gets there by a different route (the interface is rebuilt and
    fast-forwarded rather than walked through).

    Playing the tour runs the interface for real, absorption calculation and all, so it is done as
    few times as the two paths allow and everything else is asserted against what that recorded."""

    @classmethod
    def setUpClass(cls):
        # a view holding a FileFinderWidget fails inside setupUi with a message-less RuntimeError
        # unless the framework is already up
        FrameworkManager.Instance()

    def setUp(self):
        self.sandbox = TutorialSandbox()
        self.sandbox.window.show()
        process_events(3)
        self.addCleanup(self._teardown)
        self._reset_recording()

    def _reset_recording(self):
        self.annotator = RecordingAnnotator()
        self.failures = []
        self.finished = []
        self.spotlit = {}
        self.narrated = []

    def _teardown(self):
        owned = self.sandbox.model.workspaces._owned_ws_names
        self.sandbox.teardown()
        process_events(3)
        leaked = [name for name in owned if ADS.doesExist(name)]
        self.assertEqual(leaked, [], "the tutorial's workspaces must not outlive its window")

    def _play(self, chapters, chapter_index=0, fast_forward=False):
        """Play from ``chapter_index`` to the end, driving it the way a user would.

        Each step is shown first, then performed with *Show me*, then left with *Next* - which is
        the path a user actually takes, and the one that exercises both. The waits matter: the
        player ignores navigation while a step is settling or working, so clicking early would
        simply spin.
        """
        player = TutorialPlayer(chapters, self.sandbox, self.annotator, parent=self.sandbox.window)
        player.step_failed.connect(lambda label, reason: self.failures.append((label, reason)))
        player.finished.connect(lambda: self.finished.append(True))

        # what the tour pointed at, per step. Keyed off the player's position rather than counted
        # off the annotator because a step is presented twice - once explained, once refreshed
        # after it has been performed - and both times set a target.
        def remember(*_args):
            self.spotlit[player.position] = self.annotator.targets[-1] if self.annotator.targets else None
            self.narrated.append(player.position)

        player.step_changed.connect(remember)
        player.step_applied.connect(remember)
        player.start(chapter_index, fast_forward=fast_forward)

        waited = 0
        while not self.finished and waited < PLAY_TIMEOUT_MS:
            if player.is_busy:
                QTest.qWait(10)
                waited += 10
                continue
            if not player.is_applied():
                player.apply_step()
                continue
            player.next_step()
        self.assertTrue(self.finished, f"the tour did not finish within {PLAY_TIMEOUT_MS / 1000}s")
        return player

    # ------------------------------------------------------------------ the whole tour

    def test_the_whole_tour_plays_against_a_real_planner(self):
        """One play, every observation it supports.

        Each observation is its own ``subTest`` so a single broken step reports itself instead of
        hiding the ones after it - which is the whole point of a test that exists to catch the tour
        drifting away from a renamed widget.
        """
        chapters = _hurried_chapters()
        self._play(chapters)

        for label, reason in self.failures:
            with self.subTest(step=label):
                self.fail(reason)

        for chapter_index, step_index, chapter, step in walk(chapters):
            if step.target is None:
                continue
            with self.subTest(chapter=chapter.name, step=step.label):
                self.assertIsInstance(
                    self.spotlit.get((chapter_index, step_index)),
                    QWidget,
                    "a step that names a target but highlighted nothing means the interface moved under the tour",
                )

        with self.subTest("every step was shown, in order"):
            # first appearance of each step: a step is presented twice, once explained and once
            # refreshed after it has been performed
            first_seen = []
            for position in self.narrated:
                if position not in first_seen:
                    first_seen.append(position)
            expected = [(chapter_index, step_index) for chapter_index, step_index, _chapter, _step in walk(chapters)]
            self.assertEqual(first_seen, expected)

        with self.subTest("the export step wrote a real file"):
            # it goes through the interface's own export path, so a tour that stopped producing
            # anything would otherwise still look like it was working
            self.assertTrue(
                os.listdir(self.sandbox.data.save_directory),
                "the export chapter should have written a real file to the demo directory",
            )

    # ------------------------------------------------------------------ entering a chapter

    def test_each_chapter_can_be_entered_on_its_own(self):
        # what the chapter tabs do: the interface is rebuilt and the earlier chapters' actions
        # replayed silently, rather than the tour being walked through to get there
        chapters = _hurried_chapters()
        for index, chapter in enumerate(chapters[1:], start=1):
            with self.subTest(chapter=chapter.name):
                self._rebuild_sandbox()
                self._play(chapters, chapter_index=index, fast_forward=True)
                self.assertEqual(self.failures, [], f"steps failed entering '{chapter.name}'")

    def _rebuild_sandbox(self):
        """Start over with a fresh interface, exactly as choosing a chapter tab does."""
        self.sandbox.teardown()
        process_events(3)
        self.sandbox = TutorialSandbox()
        self.sandbox.window.show()
        process_events(3)
        self._reset_recording()


if __name__ == "__main__":
    unittest.main()
