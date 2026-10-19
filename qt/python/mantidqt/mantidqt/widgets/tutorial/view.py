# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""The view half of adding a tutorial to an interface: the button that opens it."""

from typing import Callable

from qtpy.QtWidgets import QLayout, QPushButton, QToolBar

from mantidqt.icons import get_icon


class TutorialViewMixin:
    """Gives a view the tutorial button ``TutorialPresenter`` drives.

    Has no ``__init__``, so it can sit in front of any Qt base: list it first, e.g.
    ``class MyView(TutorialViewMixin, QMainWindow, Ui_MyView)``, and call ``add_tutorial_button``
    from the view's constructor.
    """

    def add_tutorial_button(self, container: QToolBar | QLayout) -> None:
        """Add the tutorial button to the end of a toolbar or a layout."""
        self.btn_tutorial = QPushButton()
        self.btn_tutorial.setIcon(get_icon("mdi.school", "black", 1.2))
        self.btn_tutorial.setToolTip("Guided tutorial")
        if isinstance(container, QToolBar):
            # the action, not the button, is what controls visibility here: a toolbar re-shows the
            # widget of any visible action, so hiding the button alone does not stick
            self._tutorial_visibility = container.addWidget(self.btn_tutorial)
        else:
            container.addWidget(self.btn_tutorial)
            self._tutorial_visibility = self.btn_tutorial

    def set_on_tutorial_clicked(self, slot: Callable) -> None:
        self.btn_tutorial.clicked.connect(slot)

    def set_tutorial_button_visible(self, visible: bool) -> None:
        self._tutorial_visibility.setVisible(visible)
