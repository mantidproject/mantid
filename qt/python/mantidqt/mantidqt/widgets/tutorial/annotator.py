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

So this owns a *panel* - one overlay and one caption - per window, creates them as they are needed,
and shows only the panel belonging to whatever is currently spotlighted. It exposes exactly the
surface the player uses, so the player neither knows nor cares how many windows are involved.
"""

from qtpy.QtCore import QObject

from mantidqt.widgets.tutorial.bubble import TutorialBubble
from mantidqt.widgets.tutorial.overlay import TutorialOverlay


class _Panel:
    """The overlay and the caption annotating one window.

    They are built, shown, hidden and discarded together and are never addressed apart, which is
    what makes them one thing here rather than two lookups kept in step by hand.
    """

    def __init__(self, window):
        self.window = window
        self.overlay = TutorialOverlay(window)
        self.bubble = TutorialBubble(window)

    def show(self, text, title):
        self.overlay.show()
        # the caption is put on rather than assumed to be there: a panel built when the tour
        # reaches a new window has never been given one
        self.bubble.show_step(text=text, title=title)
        self.bubble.show()
        self.bubble.raise_()

    def hide(self):
        self.overlay.hide()
        self.bubble.hide()

    def dispose(self):
        self.overlay.detach()
        self.bubble.hide()
        for widget in (self.bubble, self.overlay):
            widget.setParent(None)
            widget.deleteLater()


class TutorialAnnotator(QObject):
    """Overlay and caption for the tour, following the target from window to window.

    :param primary: the window the tour belongs to. Used whenever there is nothing to point at, so
        a step that only narrates appears over the interface rather than wherever the last target
        happened to live.
    """

    def __init__(self, primary, parent=None):
        super().__init__(parent)
        self._primary = primary
        self._panels = {}  # window -> _Panel
        self._active = None
        self._text = ""
        self._title = ""

    # ------------------------------------------------------------------ pointing

    def set_target(self, widget):
        panel = self._activate(self._panel_for(self._host_for(widget)))
        if panel is not None:
            panel.overlay.set_target(widget)

    def target_rect(self):
        return None if self._active is None else self._active.overlay.target_rect()

    def rect_of(self, widget):
        """Where ``widget`` is, in the coordinates of the window currently being annotated."""
        return None if self._active is None else self._active.overlay.rect_of(widget)

    # ------------------------------------------------------------------ narrating

    def show_step(self, text, title=""):
        # kept because the tour can move to a window whose panel does not exist yet, and that panel
        # has to open showing the caption that is current rather than a blank box
        self._text, self._title = text, title
        panel = self._narration_panel()
        if panel is not None:
            panel.bubble.show_step(text=text, title=title)

    def show_waiting(self, message):
        panel = self._narration_panel()
        if panel is not None:
            panel.bubble.show_waiting(message)

    def place_beside(self, spotlight, keep_clear=()):
        if self._active is not None:
            self._active.bubble.place_beside(spotlight, keep_clear)

    # ------------------------------------------------------------------ lifecycle

    def show(self):
        self._activate(self._panel_for(self._primary))

    def hide(self):
        for panel in self._panels.values():
            panel.hide()
        self._active = None

    def detach(self):
        """Discard every panel. Safe to call more than once, and leaves the annotator inert."""
        for panel in self._panels.values():
            panel.dispose()
        self._panels.clear()
        self._active = None
        self._primary = None

    # ------------------------------------------------------------------ where it is pointing

    def active_window(self):
        return None if self._active is None else self._active.window

    def active_bubble(self):
        return None if self._active is None else self._active.bubble

    # ------------------------------------------------------------------ panels

    def _host_for(self, widget):
        """Which widget should carry the scrim for ``widget``.

        Anything inside the primary is annotated on the primary itself, *not* on its window. The
        interface being toured is a child of the tutorial shell, and dimming the whole window would
        put the scrim over the chapter tabs and the navigation buttons - the controls the user needs
        to drive the tour, and the reason the shell holds them outside the interface in the first
        place. Anything else is annotated on its own window, which is how a dialog gets reached.

        None once ``detach`` has run - there is no longer anywhere to put a scrim.
        """
        if widget is None or self._primary is None:
            return self._primary
        if widget is self._primary or self._primary.isAncestorOf(widget):
            return self._primary
        return widget.window()

    def _panel_for(self, window):
        """The panel annotating ``window``, built the first time it is visited.

        None once ``detach`` has run: there is no primary left to fall back to, and every method
        that can be reached afterwards treats that as nothing to annotate.
        """
        if window is None:
            return None
        if window not in self._panels:
            self._panels[window] = _Panel(window)
        return self._panels[window]

    def _narration_panel(self):
        """The panel a caption belongs on: wherever the tour is pointing, or the interface itself
        before it has pointed anywhere."""
        if self._active is None:
            return self._activate(self._panel_for(self._primary))
        return self._active

    def _activate(self, panel):
        """Bring ``panel`` on screen and take down whichever was there before. Returns it."""
        if panel is None or panel is self._active:
            return panel

        # only one window is annotated at a time: leaving the previous overlay up would dim a
        # window the tour has moved on from, with a caption on it that no longer applies
        if self._active is not None:
            self._active.overlay.set_target(None)
            self._active.hide()

        self._active = panel
        panel.show(self._text, self._title)
        return panel
