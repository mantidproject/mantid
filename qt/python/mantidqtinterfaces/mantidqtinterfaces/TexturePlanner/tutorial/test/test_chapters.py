# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Plays the whole tutorial against a real Texture Planner.

The tour is written against widget names and presenter methods, so it drifts the moment either is
renamed - and it drifts silently, because nothing else imports it. This is what makes that loud:
every chapter is played end to end, and every step has to perform and to find what it points at.

The observations are in ``subTest`` blocks so one broken step reports itself without hiding the
rest. Building the sandbox is not - a failure there makes every following observation meaningless.
"""

import os
import unittest

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


class RecordingOverlay:
    """Collects what the tour pointed at, instead of painting it."""

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


class RecordingBubble:
    def __init__(self):
        self.shown = []

    def show_step(self, text, title=""):
        self.shown.append((title, text))

    def show_waiting(self, message):
        pass

    def place_beside(self, _spotlight, keep_clear=()):
        pass


def _hurry(step):
    return type(step)(
        text=step.text,
        target=step.target,
        action=step.action,
        title=step.title,
        await_=step.await_,
        await_timeout_s=step.await_timeout_s,
        await_text=step.await_text,
        **FAST,
    )


def _hurried_chapters():
    return tuple(
        type(chapter)(name=chapter.name, description=chapter.description, steps=[_hurry(s) for s in chapter.steps]) for chapter in CHAPTERS
    )


@start_qapplication
class TutorialChaptersTest(unittest.TestCase):
    """One test per chapter, plus the whole tour, because a chapter is what the user can jump to
    and so is what has to work on its own."""

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

        self.overlay = RecordingOverlay()
        self.bubble = RecordingBubble()
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
        player = TutorialPlayer(chapters, self.sandbox, self.overlay, self.bubble, parent=self.sandbox.window)
        player.step_failed.connect(lambda label, reason: self.failures.append((label, reason)))
        player.finished.connect(lambda: self.finished.append(True))

        # what the tour pointed at, per step. Recorded from the player rather than counted off the
        # overlay because a step is presented twice - once explained, once refreshed after it has
        # been performed - and both times set a target.
        def remember(*_args):
            self.spotlit[player.position] = self.overlay.targets[-1] if self.overlay.targets else None
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

    # ------------------------------------------------------------------ per chapter

    def test_each_chapter_plays_on_its_own(self):
        chapters = _hurried_chapters()
        for index, chapter in enumerate(chapters):
            with self.subTest(chapter=chapter.name):
                self.failures = []
                self.finished = []
                # rebuilt for each chapter, exactly as choosing its tab does it
                self._teardown_and_rebuild()
                self._play(chapters, chapter_index=index, fast_forward=index > 0)
                self.assertEqual(self.failures, [], f"steps failed in '{chapter.name}'")

    def _teardown_and_rebuild(self):
        self.sandbox.teardown()
        process_events(3)
        self.sandbox = TutorialSandbox()
        self.sandbox.window.show()
        process_events(3)
        self.overlay = RecordingOverlay()
        self.bubble = RecordingBubble()
        self.spotlit = {}
        self.narrated = []

    # ------------------------------------------------------------------ the whole tour

    def test_the_whole_tour_plays_without_a_step_failing(self):
        self._play(_hurried_chapters())
        for label, reason in self.failures:
            with self.subTest(step=label):
                self.fail(reason)
        self.assertEqual(self.failures, [])

    def test_every_step_that_points_at_something_found_a_live_widget(self):
        chapters = _hurried_chapters()
        self._play(chapters)

        for chapter_index, step_index, chapter, step in walk(chapters):
            if step.target is None:
                continue
            with self.subTest(chapter=chapter.name, step=step.label):
                target = self.spotlit.get((chapter_index, step_index))
                self.assertIsInstance(
                    target,
                    QWidget,
                    "a step that names a target but highlighted nothing means the interface moved under the tour",
                )

    def test_the_tour_really_exports_a_file(self):
        # the export step goes through the interface's own export path, so a tour that stopped
        # actually producing anything would otherwise still look like it was working
        self._play(_hurried_chapters())

        written = os.listdir(self.sandbox.data.save_directory)
        self.assertTrue(written, "the export chapter should have written a real file to the demo directory")

    def test_every_step_was_shown_in_order(self):
        chapters = _hurried_chapters()
        self._play(chapters)

        # first appearance of each step, in the order the tour reached them
        first_seen = []
        for position in self.narrated:
            if position not in first_seen:
                first_seen.append(position)

        expected = [(chapter_index, step_index) for chapter_index, step_index, _chapter, _step in walk(chapters)]
        self.assertEqual(first_seen, expected)


if __name__ == "__main__":
    unittest.main()
