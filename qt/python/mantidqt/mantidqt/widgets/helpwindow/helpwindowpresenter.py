from qtpy.QtWidgets import QApplication
import sys
from .helpwindowview import HelpWindowView
from .helpwindowmodel import HelpWindowModel


class HelpWindowPresenter:
    def __init__(self):
        self.model = HelpWindowModel()
        self.view = HelpWindowView(self)
        self.app = QApplication(sys.argv)

    def show_help_window(self):
        """Show the help window."""
        self.view.display()
        sys.exit(self.app.exec_())

    def on_close(self):
        """Handle actions when the window is closed."""
        print("Help window closed.")
