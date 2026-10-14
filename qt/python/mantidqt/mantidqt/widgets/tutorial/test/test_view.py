# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest
from unittest.mock import MagicMock

from qtpy.QtWidgets import QHBoxLayout, QMainWindow, QToolBar, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.tutorial import interaction
from mantidqt.widgets.tutorial.view import TutorialViewMixin


class ToolbarView(TutorialViewMixin, QMainWindow):
    def __init__(self):
        super().__init__()
        self.toolbar = QToolBar(self)
        self.addToolBar(self.toolbar)
        self.add_tutorial_button(self.toolbar)


class LayoutView(TutorialViewMixin, QWidget):
    def __init__(self):
        super().__init__()
        self.add_tutorial_button(QHBoxLayout(self))


@start_qapplication
class TutorialViewMixinTest(unittest.TestCase):
    def _show(self, view):
        view.show()
        interaction.process_events(3)
        self.addCleanup(view.deleteLater)
        self.addCleanup(view.close)
        return view

    def test_hiding_the_button_in_a_toolbar_sticks(self):
        # a toolbar re-shows the widget of any visible action, so this is the case that needs the action
        view = self._show(ToolbarView())

        view.set_tutorial_button_visible(False)
        interaction.process_events(3)

        self.assertFalse(view.btn_tutorial.isVisible())

    def test_hiding_the_button_in_a_layout(self):
        view = self._show(LayoutView())

        view.set_tutorial_button_visible(False)

        self.assertFalse(view.btn_tutorial.isVisible())

    def test_clicking_the_button_calls_the_slot(self):
        view = self._show(LayoutView())
        slot = MagicMock()
        view.set_on_tutorial_clicked(slot)

        view.btn_tutorial.click()

        slot.assert_called_once()


if __name__ == "__main__":
    unittest.main()
