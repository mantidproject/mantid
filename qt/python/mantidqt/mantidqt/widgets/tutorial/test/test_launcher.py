# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import tempfile
import unittest

from qtpy.QtCore import QSettings
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QPushButton, QVBoxLayout, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.launcher import TutorialSession, mark_seen, run_tutorial, should_show_on_startup
from mantidqt.widgets.tutorial.step import TutorialChapter, TutorialStep


class FakeSandbox:
    """Stands in for a real interface: a window with a button, and a record of what the tour did
    to it."""

    def __init__(self, performed, built):
        self.window = QWidget()
        layout = QVBoxLayout(self.window)
        self.button = QPushButton("Target")
        layout.addWidget(self.button)
        self.window.resize(500, 400)
        self.performed = performed
        self.torn_down = False
        built.append(self)

    def teardown(self):
        self.torn_down = True
        self.window.close()
        self.window.deleteLater()


@start_qapplication
class SeenFlagTest(unittest.TestCase):
    """QSettings is redirected to a temporary ini file so the developer's own settings are never
    touched by the test - the flag under test is the one an interface really writes."""

    def setUp(self):
        self._tmpdir = tempfile.TemporaryDirectory(prefix="tutorial_settings_", ignore_cleanup_errors=True)
        self.addCleanup(self._tmpdir.cleanup)
        self.settings = QSettings(f"{self._tmpdir.name}/tutorial.ini", QSettings.IniFormat)

    def test_a_tutorial_that_has_never_been_shown_should_be(self):
        self.assertTrue(should_show_on_startup("TexturePlanner", self.settings))

    def test_marking_it_seen_stops_it_appearing_again(self):
        mark_seen("TexturePlanner", self.settings)
        self.assertFalse(should_show_on_startup("TexturePlanner", self.settings))

    def test_the_flag_is_per_interface(self):
        mark_seen("TexturePlanner", self.settings)
        self.assertTrue(should_show_on_startup("SomeOtherInterface", self.settings))

    def test_it_is_stored_beside_the_other_interface_settings(self):
        mark_seen("TexturePlanner", self.settings)
        self.settings.sync()
        self.assertIn("CustomInterfaces/TexturePlanner/tutorial_seen", self.settings.allKeys())

    def test_a_corrupt_value_is_treated_as_not_yet_seen_rather_than_seen(self):
        # QSettings coerces what it cannot parse; erring towards showing the tour is the harmless
        # direction to be wrong in
        self.settings.setValue("CustomInterfaces/TexturePlanner/tutorial_seen", "not a boolean")
        self.assertTrue(should_show_on_startup("TexturePlanner", self.settings))


@start_qapplication
class TutorialSessionTest(unittest.TestCase):
    def setUp(self):
        self.performed = []
        self.built = []
        self.sessions = []

    def tearDown(self):
        for session in self.sessions:
            session.close()
        interaction.process_events(3)

    def _factory(self):
        return FakeSandbox(self.performed, self.built)

    def _chapters(self):
        def record(name):
            return lambda sandbox: sandbox.performed.append(name)

        return (
            TutorialChapter(
                name="Setup",
                steps=[
                    TutorialStep(text="first", action=record("load"), target=lambda s: s.button, settle_ms=1),
                    TutorialStep(text="second", action=record("material"), settle_ms=1),
                ],
            ),
            TutorialChapter(
                name="Results",
                steps=[TutorialStep(text="third", action=record("plot"), settle_ms=1)],
            ),
        )

    def _session(self, chapters=None):
        session = TutorialSession(self._factory, chapters or self._chapters())
        self.sessions.append(session)
        self.finished = []
        session.finished.connect(lambda: self.finished.append(True))
        return session

    @staticmethod
    def _pump_until(predicate, timeout_ms=5000):
        waited = 0
        while not predicate() and waited < timeout_ms:
            QTest.qWait(5)
            waited += 5
        return predicate()

    def _wait_ready(self, session):
        """Wait until the tour is showing a step and will accept navigation.

        ``session.player`` is briefly None during a chapter jump, while the old interface has been
        torn down and the new one not yet built, so this has to tolerate that rather than assume a
        player is always there.
        """
        self.assertTrue(self._pump_until(lambda: session.player is not None and not session.player.is_busy))

    def _play_to_the_end(self, session):
        """Press Next until the tour finishes. Nothing advances on its own any more, so a test
        that wants the end of the tour has to walk there like a user."""
        for _ in range(60):
            if not session.player.is_running:
                return True
            self._wait_ready(session)
            session.shell.btn_next.click()
        return not session.player.is_running

    def test_it_builds_and_shows_a_sandbox_rather_than_touching_anything_of_the_users(self):
        session = self._session()
        session.start()

        self.assertEqual(len(self.built), 1)
        self.assertTrue(session.window.isVisible())
        self.assertIs(session.window, self.built[0].window)

    def test_it_plays_the_tour_through_the_sandbox(self):
        session = self._session()
        session.start()

        self.assertTrue(self._play_to_the_end(session))
        self.assertEqual(self.performed, ["load", "material", "plot"])
        self.assertEqual(session.failures, [])

    def test_closing_tears_the_sandbox_down(self):
        session = self._session()
        session.start()
        sandbox = self.built[0]

        session.close()
        interaction.process_events(3)

        self.assertTrue(sandbox.torn_down, "the sandbox window and its workspaces must not outlive the tour")
        self.assertEqual(self.finished, [True])

    def test_closing_twice_is_harmless(self):
        session = self._session()
        session.start()
        session.close()
        session.close()
        self.assertEqual(self.finished, [True], "the tour should report finishing once, not once per call")

    def test_starting_at_a_later_chapter_rebuilds_and_catches_the_interface_up(self):
        session = self._session()
        session.start(chapter_index=1)

        self._wait_ready(session)
        # the earlier chapter's actions still ran, so the chapter is entered against the state it
        # expects rather than an empty interface. Its own first step is explained, not performed.
        self.assertEqual(self.performed, ["load", "material"])

    def test_the_shell_buttons_drive_the_player(self):
        session = self._session()
        session.start()
        self._wait_ready(session)

        session.shell.btn_next.click()
        self.assertTrue(self._pump_until(lambda: session.player.position == (0, 1)))

        self._wait_ready(session)
        session.shell.btn_back.click()
        interaction.process_events(3)
        self.assertEqual(session.player.position, (0, 0))

    def test_the_shell_frames_the_interface_being_toured(self):
        session = self._session()
        session.start()

        self.assertIs(session.window.parentWidget(), session.shell)
        self.assertTrue(session.shell.isVisible())

    def test_show_me_performs_the_step_without_moving_off_it(self):
        session = self._session()
        session.start()
        self._wait_ready(session)
        self.assertEqual(self.performed, [], "a step is explained before it is performed")
        self.assertTrue(session.shell.btn_apply.isEnabled())

        session.shell.btn_apply.click()
        self._wait_ready(session)

        self.assertEqual(self.performed, ["load"])
        self.assertEqual(session.player.position, (0, 0))
        self.assertEqual(session.shell.btn_apply.text(), "Done")
        self.assertFalse(session.shell.btn_apply.isEnabled())

    def test_show_me_is_hidden_for_a_step_with_nothing_to_do(self):
        chapters = (TutorialChapter(name="Setup", steps=[TutorialStep(text="just words", settle_ms=1)]),)
        session = self._session(chapters)
        session.start()
        self._wait_ready(session)

        self.assertFalse(session.shell.btn_apply.isVisible())

    def test_the_shell_tracks_which_step_the_tour_is_on(self):
        from qtpy.QtWidgets import QLabel

        session = self._session()
        session.start()
        self._wait_ready(session)

        position = session.shell.findChild(QLabel, "tutorial_position")
        self.assertEqual(position.text(), "Step 1 of 2")
        self.assertFalse(session.shell.btn_back.isEnabled(), "there is nothing before the first step")

        session.shell.btn_next.click()
        self._wait_ready(session)
        self.assertEqual(session.player.position, (0, 1))
        self.assertEqual(position.text(), "Step 2 of 2")
        self.assertTrue(session.shell.btn_back.isEnabled())

    def test_choosing_a_chapter_tab_rebuilds_the_interface_and_jumps_to_it(self):
        session = self._session()
        session.start()
        self._wait_ready(session)
        self.assertEqual(len(self.built), 1)
        first_sandbox = self.built[0]
        self.performed.clear()

        session.shell._tabs.setCurrentIndex(1)
        self.assertTrue(self._pump_until(lambda: len(self.built) == 2))
        self._wait_ready(session)

        self.assertTrue(first_sandbox.torn_down, "the chapter jump should discard the old interface")
        self.assertEqual(session.player.position, (1, 0))
        # the earlier chapter's actions were replayed so the chapter starts from the right state
        self.assertEqual(self.performed, ["load", "material"])

    def test_a_chapter_jump_keeps_the_window_where_the_user_put_it(self):
        session = self._session()
        session.start()
        self._wait_ready(session)
        session.shell.resize(1000, 720)
        interaction.process_events(3)
        geometry = session.shell.geometry()

        session.shell._tabs.setCurrentIndex(1)
        self.assertTrue(self._pump_until(lambda: len(self.built) == 2))

        self.assertEqual(session.shell.geometry(), geometry)

    def test_the_end_of_the_tour_says_the_users_session_was_untouched(self):
        session = self._session()
        session.start()
        self.assertTrue(self._play_to_the_end(session))

        interaction.process_events(3)
        text = session._bubble.findChild(QWidget, "tutorial_bubble_text").text()
        self.assertIn("has touched it", text)
        self.assertFalse(session.shell.btn_next.isEnabled(), "there is nowhere left to go")

    def test_a_failing_step_is_collected_rather_than_ending_the_tour(self):
        def explode(_sandbox):
            raise RuntimeError("gone")

        chapters = (
            TutorialChapter(
                name="Setup",
                steps=[
                    TutorialStep(text="broken", title="Broken", action=explode, settle_ms=1),
                    TutorialStep(text="fine", settle_ms=1),
                ],
            ),
        )
        session = self._session(chapters)
        session.start()

        self.assertTrue(self._play_to_the_end(session))
        self.assertEqual(session.failures, [("Broken", "gone")])

    def _isolate_default_qsettings(self):
        """Point the process-wide QSettings at a temporary ini file.

        ``run_tutorial`` writes through a bare ``QSettings()`` - which is the whole point, since
        that is what an interface will read - so testing it means redirecting the default rather
        than passing an instance in. On Windows the default format is the registry, which
        ``setPath`` cannot redirect, hence forcing the ini format first.
        """
        from qtpy.QtCore import QCoreApplication

        tmpdir = tempfile.TemporaryDirectory(prefix="tutorial_launcher_", ignore_cleanup_errors=True)
        self.addCleanup(tmpdir.cleanup)
        saved = (QSettings.defaultFormat(), QCoreApplication.organizationName(), QCoreApplication.applicationName())

        QSettings.setDefaultFormat(QSettings.IniFormat)
        QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, tmpdir.name)
        QCoreApplication.setOrganizationName("mantidproject-tutorial-test")
        QCoreApplication.setApplicationName("tutorial_launcher_test")

        def restore():
            QSettings.setDefaultFormat(saved[0])
            QSettings.setPath(QSettings.IniFormat, QSettings.UserScope, tempfile.gettempdir())
            QCoreApplication.setOrganizationName(saved[1])
            QCoreApplication.setApplicationName(saved[2])

        self.addCleanup(restore)

    def test_run_tutorial_marks_it_seen_when_it_appeared_by_itself(self):
        self._isolate_default_qsettings()
        self.assertTrue(should_show_on_startup("TexturePlanner"))

        session = run_tutorial(self._factory, self._chapters(), settings_key="TexturePlanner", mark_as_seen=True)
        self.sessions.append(session)

        self.assertFalse(should_show_on_startup("TexturePlanner"))

    def test_run_tutorial_leaves_the_flag_alone_when_the_user_asked_for_it(self):
        self._isolate_default_qsettings()

        # opening it from the toolbar button should not change whether it appears on startup
        session = run_tutorial(self._factory, self._chapters(), settings_key="TexturePlanner", mark_as_seen=False)
        self.sessions.append(session)

        self.assertTrue(should_show_on_startup("TexturePlanner"))


if __name__ == "__main__":
    unittest.main()
