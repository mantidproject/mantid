# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from .helpwindowview import HelpWindowView
from .helpwindowmodel import HelpWindowModel


class HelpWindowPresenter:
    def __init__(self, parent_app=None):
        """
        Initialize the presenter for the Help Window.
        :param parent_app: Optional parent application to tie the lifecycle of this presenter.
        """
        self.model = HelpWindowModel()
        self.view = HelpWindowView(self)
        self.parent_app = parent_app
        self._window_open = False

        if self.parent_app:
            self.parent_app.aboutToQuit.connect(self.cleanup)

    def show_help_window(self):
        """Show the help window."""
        if not self._window_open:
            self._window_open = True
            self.view.display()

    def on_close(self):
        """Handle actions when the window is closed."""
        print("Help window closed.")
        self.cleanup()

    def cleanup(self):
        """Ensure proper cleanup of resources."""
        if self._window_open:
            self._window_open = False
            print("Cleaning up Help Window resources.")
            self.view.close()
