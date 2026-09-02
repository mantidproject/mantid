# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtWidgets import QDialog, QLabel, QPushButton, QVBoxLayout, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.annotator import TutorialAnnotator


@start_qapplication
class TutorialAnnotatorTest(unittest.TestCase):
    def setUp(self):
        self.window = QWidget()
        layout = QVBoxLayout(self.window)
        self.button = QPushButton("In the main window")
        layout.addWidget(self.button)
        self.window.resize(600, 400)
        self.window.show()

        # a second top-level window, as a settings dialog is
        self.dialog = QDialog(self.window)
        dialog_layout = QVBoxLayout(self.dialog)
        self.setting = QPushButton("In the dialog")
        dialog_layout.addWidget(self.setting)
        self.dialog.resize(400, 300)
        self.dialog.show()

        interaction.process_events(3)
        self.annotator = TutorialAnnotator(self.window)
        self.annotator.show()
        interaction.process_events(3)

    def tearDown(self):
        self.annotator.detach()
        self.dialog.close()
        self.window.close()
        self.dialog.deleteLater()
        self.window.deleteLater()
        interaction.process_events()

    def _caption_text(self):
        bubble = self.annotator.active_bubble()
        return bubble.findChild(QLabel, "tutorial_bubble_text").text()

    # ------------------------------------------------------------------ following the target

    def test_it_annotates_the_window_the_target_is_in(self):
        self.annotator.set_target(self.button)
        self.assertIs(self.annotator.active_window(), self.window)

    def test_it_dims_the_primary_itself_not_the_window_around_it(self):
        # the interface is a child of the tutorial shell, which also holds the chapter tabs and the
        # navigation. Dimming its *window* would put the scrim over the controls driving the tour.
        shell = QWidget()
        self.addCleanup(shell.deleteLater)
        shell_layout = QVBoxLayout(shell)
        interface = QWidget()
        interface_layout = QVBoxLayout(interface)
        target = QPushButton("deep inside")
        interface_layout.addWidget(target)
        shell_layout.addWidget(interface)
        shell.resize(500, 400)
        shell.show()
        interaction.process_events(3)

        annotator = TutorialAnnotator(interface)
        self.addCleanup(annotator.detach)
        annotator.set_target(target)

        self.assertIs(annotator.active_window(), interface)
        self.assertIsNot(annotator.active_window(), shell)

    def test_it_follows_a_target_into_another_window(self):
        # the whole reason this exists: a settings dialog is a window of its own, and the
        # interface's overlay cannot reach into it
        self.annotator.set_target(self.button)
        self.annotator.set_target(self.setting)

        self.assertIs(self.annotator.active_window(), self.dialog)

    def test_the_caption_moves_with_it(self):
        self.annotator.show_step("about a setting", title="Settings")
        self.annotator.set_target(self.setting)
        interaction.process_events(3)

        self.assertIs(self.annotator.active_bubble().window(), self.dialog)
        self.assertEqual(self._caption_text(), "about a setting")

    def test_only_one_window_is_annotated_at_a_time(self):
        # a scrim left on a window the tour has moved off, with a caption that no longer applies
        self.annotator.set_target(self.button)
        first_bubble = self.annotator.active_bubble()

        self.annotator.set_target(self.setting)
        interaction.process_events(3)

        self.assertFalse(first_bubble.isVisible())
        self.assertTrue(self.annotator.active_bubble().isVisible())

    def test_it_comes_back_to_the_main_window(self):
        self.annotator.set_target(self.setting)
        self.annotator.set_target(self.button)
        self.assertIs(self.annotator.active_window(), self.window)

    def test_a_step_with_no_target_annotates_the_window_the_tour_belongs_to(self):
        # otherwise a narration-only step would appear over whatever window happened to be last
        self.annotator.set_target(self.setting)
        self.annotator.set_target(None)
        self.assertIs(self.annotator.active_window(), self.window)

    def test_it_reuses_a_windows_panel_rather_than_building_another(self):
        self.annotator.set_target(self.setting)
        first = self.annotator.active_bubble()
        self.annotator.set_target(self.button)
        self.annotator.set_target(self.setting)

        self.assertIs(self.annotator.active_bubble(), first)

    # ------------------------------------------------------------------ the player's surface

    def test_the_spotlight_rect_comes_from_the_active_window(self):
        self.annotator.set_target(self.setting)
        interaction.process_events(3)

        rect = self.annotator.target_rect()
        self.assertFalse(rect.isEmpty())
        self.assertTrue(rect.contains(self.setting.geometry()))

    def test_show_waiting_replaces_the_caption_text(self):
        self.annotator.show_step("a step", title="Step")
        self.annotator.show_waiting("Calculating…")
        self.assertEqual(self._caption_text(), "Calculating…")

    def test_hide_takes_every_panel_down(self):
        self.annotator.set_target(self.setting)
        self.annotator.set_target(self.button)

        self.annotator.hide()
        interaction.process_events(3)

        self.assertIsNone(self.annotator.active_bubble())

    def test_detach_is_safe_to_repeat(self):
        self.annotator.set_target(self.setting)
        self.annotator.detach()
        self.annotator.detach()
        self.assertIsNone(self.annotator.active_window())

    def test_the_whole_surface_is_inert_once_detached(self):
        # the session stops the player before detaching, so nothing should arrive after this - but
        # every method answering the same way is one less thing to check when reading the class
        self.annotator.set_target(self.setting)
        self.annotator.detach()

        self.annotator.set_target(self.button)
        self.annotator.show_step("anything", title="Step")
        self.annotator.show_waiting("Working…")
        self.annotator.place_beside(None)
        self.annotator.show()

        self.assertIsNone(self.annotator.active_window())
        self.assertIsNone(self.annotator.target_rect())
        self.assertIsNone(self.annotator.rect_of(self.button))


if __name__ == "__main__":
    unittest.main()
