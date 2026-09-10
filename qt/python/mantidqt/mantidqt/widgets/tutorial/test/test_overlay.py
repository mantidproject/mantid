# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest

from qtpy.QtCore import Qt
from qtpy.QtTest import QTest
from qtpy.QtWidgets import QPushButton, QVBoxLayout, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.overlay import PADDING, TutorialOverlay


@start_qapplication
class TutorialOverlayTest(unittest.TestCase):
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

        self.overlay = TutorialOverlay(self.window)
        self.overlay.show()
        interaction.process_events(3)

    def tearDown(self):
        self.overlay.detach()
        self.window.close()
        self.window.deleteLater()
        interaction.process_events()

    @staticmethod
    def _settle(rounds=3, wait_ms=120):
        # long enough for the overlay's position tracker (50ms) to have ticked at least twice
        QTest.qWait(wait_ms)
        interaction.process_events(rounds)

    def test_it_covers_the_whole_host(self):
        self.assertEqual(self.overlay.geometry(), self.window.rect())

    def test_it_never_takes_input(self):
        # a tour that could swallow a click would be able to deadlock behind its own decoration
        self.assertTrue(self.overlay.testAttribute(Qt.WA_TransparentForMouseEvents))

    def test_with_no_target_there_is_no_spotlight(self):
        self.overlay.set_target(None)
        self._settle()
        self.assertTrue(self.overlay.target_rect().isEmpty())

    def test_the_spotlight_surrounds_the_target(self):
        self.overlay.set_target(self.first)
        self._settle()

        spotlight = self.overlay.target_rect()
        expected = self.first.geometry().adjusted(-PADDING, -PADDING, PADDING, PADDING)
        self.assertEqual(spotlight.size(), expected.size())
        self.assertTrue(spotlight.contains(self.first.geometry()), "the target should sit inside the spotlight")

    def test_the_spotlight_moves_with_the_target_when_the_host_is_resized(self):
        self.overlay.set_target(self.second)
        self._settle()
        before = self.overlay.target_rect()

        self.window.resize(700, 600)
        self._settle()

        self.assertEqual(self.overlay.geometry(), self.window.rect())
        after = self.overlay.target_rect()
        self.assertNotEqual(before, after, "the button was relaid out, so the spotlight should have followed")
        self.assertTrue(after.contains(self.second.geometry()))

    def test_the_spotlight_follows_a_target_that_moves_without_a_resize(self):
        self.overlay.set_target(self.second)
        self._settle()
        before = self.overlay.target_rect()

        # the kind of move no single event on the host reports: the widget above it goes away and
        # the layout shuffles everything up
        self.first.hide()
        self._settle()

        self.assertNotEqual(before, self.overlay.target_rect())
        self.assertTrue(self.overlay.target_rect().contains(self.second.geometry()))

    def test_a_hidden_target_has_no_spotlight_rather_than_a_stale_one(self):
        self.overlay.set_target(self.second)
        self._settle()
        self.assertFalse(self.overlay.target_rect().isEmpty())

        self.second.hide()
        self._settle()

        self.assertTrue(self.overlay.target_rect().isEmpty())

    def test_a_target_outside_the_host_is_refused(self):
        stranger = QPushButton("Elsewhere")
        self.addCleanup(stranger.deleteLater)
        self.assertRaises(ValueError, self.overlay.set_target, stranger)

    def test_detach_is_safe_to_repeat(self):
        self.overlay.set_target(self.first)
        self.overlay.detach()
        self.overlay.detach()
        self.assertIsNone(self.overlay.target())
        self.assertFalse(self.overlay.isVisible())


if __name__ == "__main__":
    unittest.main()
