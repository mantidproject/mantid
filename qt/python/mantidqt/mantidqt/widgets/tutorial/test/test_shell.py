# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtWidgets import QLabel, QMainWindow, QTabBar, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.shell import TutorialShell
from mantidqt.widgets.tutorial.step import TutorialChapter, TutorialStep


def _chapters():
    return (
        TutorialChapter(name="Sample setup", steps=[TutorialStep(text="a"), TutorialStep(text="b")], description="Load a sample"),
        TutorialChapter(name="Results", steps=[TutorialStep(text="c")]),
        TutorialChapter(name="Exporting", steps=[TutorialStep(text="d")]),
    )


@start_qapplication
class TutorialShellTest(unittest.TestCase):
    def setUp(self):
        self.chapters = _chapters()
        # a QMainWindow, because that is what a Mantid interface is and the shell has to adopt one
        self.interface = QMainWindow()
        self.interface.setCentralWidget(QLabel("the interface"))
        self.interface.resize(1200, 800)
        # a parent, as the real launcher passes: the tutorial belongs to the window that opened it
        self.owner = QWidget()
        self.shell = TutorialShell(self.chapters, self.interface, parent=self.owner, title="Tutorial")
        self.shell.show()
        interaction.process_events(3)

    def tearDown(self):
        self.shell.close()
        self.shell.deleteLater()
        self.owner.deleteLater()
        interaction.process_events()

    def _tab_bar(self):
        return self.shell.findChild(QTabBar, "tutorial_chapter_tabs")

    # ------------------------------------------------------------------ framing

    def test_it_is_a_window_of_its_own_despite_having_a_parent(self):
        # a QWidget given a parent is a child widget by default, which would draw the whole tutorial
        # as a small panel in the corner of the window that launched it
        self.assertTrue(self.shell.isWindow(), "the tutorial must be a window, not a panel inside its parent")
        self.assertIs(self.shell.parentWidget(), self.owner, "but still owned by the window it belongs to")
        self.assertEqual(self.shell.windowTitle(), "Tutorial")

    def test_it_opens_big_enough_to_show_the_interface_it_frames(self):
        # the interface arrives sized by its .ui file; the shell has to be at least that big plus
        # room for its own chrome, or framing a real interface achieves nothing
        self.assertGreaterEqual(self.shell.width(), self.interface.width())
        self.assertGreater(self.shell.height(), self.interface.height())

    def test_a_tiny_interface_still_gets_a_usable_window(self):
        small = QMainWindow()
        small.setCentralWidget(QLabel("small"))
        small.resize(120, 80)
        shell = TutorialShell(self.chapters, small, title="Tutorial")
        self.addCleanup(shell.deleteLater)

        self.assertGreaterEqual(shell.width(), 900)
        self.assertGreaterEqual(shell.height(), 600)

    def test_it_adopts_the_interface_as_a_child(self):
        self.assertIs(self.interface.parentWidget(), self.shell)
        self.assertTrue(self.interface.isVisible())

    def test_the_interface_sits_between_the_tabs_and_the_controls(self):
        tabs = self._tab_bar()
        controls = self.shell.findChild(QWidget, "tutorial_controls")
        interface_top = self.interface.mapTo(self.shell, self.interface.rect().topLeft()).y()
        interface_bottom = interface_top + self.interface.height()

        self.assertLessEqual(tabs.geometry().bottom(), interface_top)
        self.assertGreaterEqual(controls.geometry().top(), interface_bottom)

    def test_the_controls_are_outside_the_interface_so_the_scrim_never_covers_them(self):
        controls = self.shell.findChild(QWidget, "tutorial_controls")
        self.assertIsNot(controls.parentWidget(), self.interface)
        self.assertFalse(self.interface.isAncestorOf(controls))
        self.assertFalse(self.interface.isAncestorOf(self._tab_bar()))

    def test_it_shows_a_tab_per_chapter(self):
        tabs = self._tab_bar()
        self.assertEqual([tabs.tabText(i) for i in range(tabs.count())], ["Sample setup", "Results", "Exporting"])
        self.assertIn("Load a sample", tabs.tabToolTip(0))

    # ------------------------------------------------------------------ controls

    def test_back_and_next_emit_their_signals(self):
        for button, signal_name in (
            (self.shell.btn_back, "back_requested"),
            (self.shell.btn_next, "next_requested"),
            (self.shell.btn_apply, "apply_requested"),
        ):
            with self.subTest(button=button.text()):
                fired = []
                getattr(self.shell, signal_name).connect(lambda: fired.append(True))
                button.click()
                interaction.process_events()
                self.assertEqual(fired, [True])

    def test_ending_the_tutorial_emits_close(self):
        fired = []
        self.shell.close_requested.connect(lambda: fired.append(True))
        self.shell.btn_close.click()
        interaction.process_events()
        self.assertEqual(fired, [True])

    def test_closing_the_window_emits_close_too(self):
        # closing a widget hides it rather than destroying it, so without this the session hears
        # nothing and the interface being toured is left alive behind a hidden window
        fired = []
        self.shell.close_requested.connect(lambda: fired.append(True))

        self.shell.close()
        interaction.process_events()

        self.assertEqual(fired, [True])

    def test_choosing_a_tab_reports_the_chapter(self):
        chosen = []
        self.shell.chapter_selected.connect(chosen.append)

        self._tab_bar().setCurrentIndex(2)
        interaction.process_events()

        self.assertEqual(chosen, [2])

    def test_clicking_the_current_chapter_restarts_it(self):
        # a chapter jump is a rebuild, which makes "start this chapter again" the one form of undo
        # the tour can offer that is always exactly right
        chosen = []
        self.shell.chapter_selected.connect(chosen.append)
        self.shell.set_current_chapter(1)

        self.shell._tabs.tabBarClicked.emit(1)
        interaction.process_events()

        self.assertEqual(chosen, [1])

    def test_clicking_a_different_chapter_reports_it_once(self):
        chosen = []
        self.shell.chapter_selected.connect(chosen.append)

        tabs = self._tab_bar()
        tabs.tabBarClicked.emit(2)
        tabs.setCurrentIndex(2)
        interaction.process_events()

        self.assertEqual(chosen, [2], "a move to another chapter must not rebuild twice")

    def test_the_player_moving_the_tab_does_not_report_it_back(self):
        # otherwise crossing into a new chapter would rebuild the interface underneath a tour that
        # was running perfectly well
        chosen = []
        self.shell.chapter_selected.connect(chosen.append)

        self.shell.set_current_chapter(1)
        interaction.process_events()

        self.assertEqual(chosen, [])
        self.assertEqual(self.shell.current_chapter(), 1)

    def test_an_out_of_range_chapter_is_ignored_rather_than_crashing(self):
        self.shell.set_current_chapter(99)
        self.assertEqual(self.shell.current_chapter(), 0)

    # ------------------------------------------------------------------ display

    def test_it_shows_where_the_tour_has_got_to(self):
        self.shell.show_position(1, 1, 1)
        self.assertEqual(self.shell.findChild(QLabel, "tutorial_position").text(), "Step 1 of 1")
        self.assertEqual(self.shell.current_chapter(), 1)

    def test_navigation_can_be_disabled_at_the_start_of_the_tour(self):
        self.shell.set_navigation_enabled(back=False, next_=True)
        self.assertFalse(self.shell.btn_back.isEnabled())
        self.assertTrue(self.shell.btn_next.isEnabled())

    def test_show_me_is_offered_for_a_step_that_does_something(self):
        self.shell.set_action_available(has_action=True, applied=False)
        self.assertTrue(self.shell.btn_apply.isVisible())
        self.assertTrue(self.shell.btn_apply.isEnabled())
        self.assertEqual(self.shell.btn_apply.text(), "Show me")

    def test_show_me_is_spent_once_the_step_has_been_performed(self):
        self.shell.set_action_available(has_action=True, applied=True)
        self.assertTrue(self.shell.btn_apply.isVisible(), "kept visible so the row does not reflow under the pointer")
        self.assertFalse(self.shell.btn_apply.isEnabled())
        self.assertEqual(self.shell.btn_apply.text(), "Done")

    def test_show_me_is_hidden_for_a_step_that_only_explains(self):
        # a permanently dead button beside two live ones reads as something being broken
        self.shell.set_action_available(has_action=False, applied=True)
        self.assertFalse(self.shell.btn_apply.isVisible())

    def test_a_spent_show_me_stays_disabled_when_the_tour_stops_being_busy(self):
        self.shell.set_action_available(has_action=True, applied=True)
        self.shell.set_busy(True, "working")
        self.shell.set_busy(False)
        self.assertFalse(self.shell.btn_apply.isEnabled())

    def test_being_busy_locks_navigation_and_the_tabs(self):
        # pressing Next during a file search would run the next step against an interface that had
        # not finished reacting to this one
        self.shell.set_action_available(has_action=True, applied=False)
        self.shell.set_busy(True, "Looking for the sample file…")

        self.assertFalse(self.shell.btn_apply.isEnabled())
        self.assertFalse(self.shell.btn_next.isEnabled())
        self.assertFalse(self.shell.btn_back.isEnabled())
        self.assertFalse(self._tab_bar().isEnabled())
        self.assertEqual(self.shell.findChild(QLabel, "tutorial_position").text(), "Looking for the sample file…")

        self.shell.set_busy(False)
        self.assertTrue(self.shell.btn_next.isEnabled())
        self.assertTrue(self._tab_bar().isEnabled())

    def test_the_step_counter_comes_back_once_the_tour_stops_being_busy(self):
        # a busy message borrows the counter's label; left there, a finished absorption calculation
        # reads as the tour still working on it
        self.shell.show_position(0, 2, 5)
        self.shell.set_busy(True, "Calculating transmission…")

        self.shell.set_busy(False)

        self.assertEqual(self.shell.findChild(QLabel, "tutorial_position").text(), "Step 2 of 5")

    def test_finishing_disables_next_but_leaves_the_tabs(self):
        self.shell.show_finished()
        self.assertFalse(self.shell.btn_next.isEnabled())
        self.assertFalse(self.shell.btn_apply.isVisible(), "there is nothing left to perform")
        self.assertTrue(self._tab_bar().isEnabled(), "the user should still be able to revisit a chapter")

    # ------------------------------------------------------------------ teardown

    def test_releasing_the_interface_leaves_it_alive_to_be_closed_properly(self):
        self.shell.release_interface()
        self.assertIsNone(self.interface.parentWidget())
        # still a live object, so its own closeEvent can run and clean up after it
        self.assertEqual(self.interface.centralWidget().text(), "the interface")
        self.interface.close()
        self.interface.deleteLater()


if __name__ == "__main__":
    unittest.main()
