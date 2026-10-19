# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
"""The presenter half of adding a tutorial to an interface.

The tour runs against a separate, throwaway copy of the interface (see ``run_tutorial``) so it can
load data, change settings and write files without any of it reaching the window the user is
working in.
"""

from qtpy.QtCore import QTimer

from mantidqt.widgets.tutorial.launcher import run_tutorial, should_show_on_startup


class TutorialPresenter:
    """Base for a presenter that offers a guided tutorial.

    Subclasses set ``TUTORIAL_SETTINGS_KEY`` and ``TUTORIAL_TITLE``, implement
    ``tutorial_chapters`` and ``make_tutorial_sandbox_factory``, call ``setup_tutorial`` at the end
    of their constructor and ``close_tutorial`` when their window closes. The view must provide
    ``set_on_tutorial_clicked`` and ``set_tutorial_button_visible`` (see ``TutorialViewMixin``).
    """

    # Settings key the tutorial's "already shown once" flag is stored under, beside the interface's
    # other settings.
    TUTORIAL_SETTINGS_KEY = ""

    # Title of the tutorial window, which frames a throwaway copy of the interface.
    TUTORIAL_TITLE = ""

    def tutorial_chapters(self):
        raise NotImplementedError

    def make_tutorial_sandbox_factory(self):
        """A no-argument factory for ``run_tutorial``. Import the sandbox inside this method: the
        sandbox builds a presenter, so importing it from the presenter's own module would be a cycle."""
        raise NotImplementedError

    def setup_tutorial(self, view, offer_tutorial: bool = True) -> None:
        """:param offer_tutorial: whether this interface may show the guided tutorial on a first ever
        open, and offer it from the toolbar. False for the throwaway interface the tutorial itself
        drives, which must not offer a tutorial inside a tutorial."""
        self._tutorial_view = view
        self._offer_tutorial = offer_tutorial
        # kept alive for as long as the tour runs: the session owns the sandbox interface it drives
        self._tutorial_session = None
        # set once this interface has been closed, so the deferred first-open offer below becomes a
        # no-op rather than building a tour for a window that has already gone
        self._closed = False
        if offer_tutorial:
            view.set_on_tutorial_clicked(self._on_tutorial_clicked)
        else:
            # the tutorial's own interface: the button would start a tutorial inside the tutorial
            view.set_tutorial_button_visible(False)

        self._offer_tutorial_on_first_open()

    def close_tutorial(self) -> None:
        self._closed = True
        # the tutorial drives a second interface of its own; closing this one must take that with it,
        # or its window would be left behind with nothing to return to
        if self._tutorial_session is not None:
            self._tutorial_session.close()

    def _on_tutorial_clicked(self) -> None:
        """Open the tutorial because the user asked for it.

        Not ``open_tutorial`` connected directly: ``clicked`` carries a bool, which would land in
        ``mark_as_seen`` - and asking for the tutorial must not change whether it appears by itself
        on a first open.
        """
        self.open_tutorial()

    def open_tutorial(self, mark_as_seen: bool = False) -> None:
        if self._closed:
            # the first-open offer is deferred to the event loop, so it can still arrive after the
            # interface it belongs to has been closed. There is nothing left to open a tour over.
            return
        if self._tutorial_session is not None:
            # already running: bring it forward rather than starting a second one. The shell is
            # what has to be raised - the interface it frames is a child widget of it, and raising
            # a child does nothing to the window it is in
            shell = self._tutorial_session.shell
            if shell is not None:
                shell.raise_()
                shell.activateWindow()
            return
        self._tutorial_session = run_tutorial(
            sandbox_factory=self.make_tutorial_sandbox_factory(),
            chapters=self.tutorial_chapters(),
            parent=self._tutorial_view,
            settings_key=self.TUTORIAL_SETTINGS_KEY,
            mark_as_seen=mark_as_seen,
            title=self.TUTORIAL_TITLE,
        )
        self._tutorial_session.finished.connect(self._on_tutorial_finished)

    def _on_tutorial_finished(self) -> None:
        self._tutorial_session = None

    def _offer_tutorial_on_first_open(self) -> None:
        """Show the tutorial the first time this interface is ever opened.

        Deferred to the event loop rather than run here: this is still the presenter's constructor,
        so the window it would appear over has not been shown or laid out yet, and the tutorial
        would have nothing to position itself against.
        """
        if not self._offer_tutorial or not should_show_on_startup(self.TUTORIAL_SETTINGS_KEY):
            return
        QTimer.singleShot(0, lambda: self.open_tutorial(mark_as_seen=True))
