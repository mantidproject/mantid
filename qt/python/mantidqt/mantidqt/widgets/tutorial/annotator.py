# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""Puts the spotlight and the caption on whichever window the tour is pointing into.

An interface is rarely one window. The moment a tour wants to explain a settings dialog it has to
annotate something that is not the window it started in, and an overlay is tied to the widget it
dims - it cannot reach across into another top-level window.

So this owns one overlay and one caption *per window*, creates them as they are needed, and shows
only the pair belonging to whatever is currently spotlighted. It exposes exactly the surface the
player uses, so the player neither knows nor cares how many windows are involved.
"""

from qtpy.QtCore import QObject

from mantidqt.widgets.tutorial.bubble import TutorialBubble
from mantidqt.widgets.tutorial.overlay import TutorialOverlay


class TutorialAnnotator(QObject):
    """Overlay and caption for the tour, following the target from window to window.

    :param primary: the window the tour belongs to. Used whenever there is nothing to point at, so
        a step that only narrates appears over the interface rather than wherever the last target
        happened to live.
    """

    def __init__(self, primary, parent=None):
        super().__init__(parent)
        self._primary = primary
        self._panels = {}  # window -> (overlay, bubble)
        self._active = None
        # the caption is re-shown when the tour moves between windows, so the last one is kept
        self._text = ""
        self._title = ""

    # ------------------------------------------------------------------ the player's surface

    def set_target(self, widget):
        self._activate(self._host_for(widget))
        overlay, _bubble = self._active
        overlay.set_target(widget)

    def _host_for(self, widget):
        """Which widget should carry the scrim for ``widget``.

        Anything inside the primary is annotated on the primary itself, *not* on its window. The
        interface being toured is a child of the tutorial shell, and dimming the whole window would
        put the scrim over the chapter tabs and the navigation buttons - the controls the user needs
        to drive the tour, and the reason the shell holds them outside the interface in the first
        place. Anything else is annotated on its own window, which is how a dialog gets reached.
        """
        if widget is None:
            return self._primary
        if widget is self._primary or self._primary.isAncestorOf(widget):
            return self._primary
        return widget.window()

    def target_rect(self):
        if self._active is None:
            return None
        overlay, _bubble = self._active
        return overlay.target_rect()

    def show_step(self, text, title=""):
        self._text, self._title = text, title
        self._activate(self._primary if self._active is None else None)
        _overlay, bubble = self._active
        bubble.show_step(text=text, title=title)

    def show_waiting(self, message):
        self._activate(self._primary if self._active is None else None)
        _overlay, bubble = self._active
        bubble.show_waiting(message)

    def place_beside(self, spotlight):
        if self._active is None:
            return
        _overlay, bubble = self._active
        bubble.place_beside(spotlight)

    def show(self):
        self._activate(self._primary)

    def hide(self):
        for overlay, bubble in self._panels.values():
            overlay.hide()
            bubble.hide()
        self._active = None

    def detach(self):
        for overlay, bubble in self._panels.values():
            overlay.detach()
            bubble.hide()
            bubble.setParent(None)
            bubble.deleteLater()
            overlay.setParent(None)
            overlay.deleteLater()
        self._panels.clear()
        self._active = None
        self._primary = None

    # ------------------------------------------------------------------ per-window panels

    def active_window(self):
        for window, panel in self._panels.items():
            if panel is self._active:
                return window
        return None

    def active_bubble(self):
        return None if self._active is None else self._active[1]

    def _activate(self, window):
        """Make ``window``'s panel the one on screen, building it if this is the first visit."""
        if window is None:
            return
        panel = self._panels.get(window)
        if panel is None:
            panel = (TutorialOverlay(window), TutorialBubble(window))
            self._panels[window] = panel
        if panel is self._active:
            return

        # only one window is annotated at a time: leaving the previous overlay up would dim a
        # window the tour has moved on from, with a caption on it that no longer applies
        if self._active is not None:
            previous_overlay, previous_bubble = self._active
            previous_overlay.set_target(None)
            previous_overlay.hide()
            previous_bubble.hide()

        self._active = panel
        overlay, bubble = panel
        overlay.show()
        bubble.show_step(text=self._text, title=self._title)
        bubble.show()
        bubble.raise_()
