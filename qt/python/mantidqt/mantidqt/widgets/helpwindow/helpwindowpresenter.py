# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from .helpwindowmodel import HelpWindowModel
from .helpwindowview import HelpWindowView


class HelpWindowPresenter:
    def __init__(self, parent_app=None, local_docs=None, online_base_url="https://docs.mantidproject.org/"):
        self.model = HelpWindowModel(local_docs_base=local_docs, online_base=online_base_url)

        # The View is told whether local docs are in use so it can set up the request interceptor
        use_local = self.model.is_local_docs_enabled()
        self.view = HelpWindowView(self, using_local_docs=use_local)

        self.parent_app = parent_app
        self._window_open = False

        if self.parent_app:
            self.parent_app.aboutToQuit.connect(self.cleanup)

    def showHelpPage(self, relative_url):
        """
        Build the doc URL from the Model and tell the View to load it.
        """
        doc_url = self.model.build_help_url(relative_url)
        self.view.set_page_url(doc_url)
        self.show_help_window()

    def show_help_window(self):
        if not self._window_open:
            self._window_open = True
            self.view.display()

    def on_close(self):
        """Called by the View when the window closes."""
        print("Help window closed.")
        self.cleanup()

    def cleanup(self):
        """Cleanup resources, close the View if open."""
        if self._window_open:
            self._window_open = False
            print("Cleaning up Help Window resources.")
            self.view.close()

    def show_home_page(self):
        """
        Presenter logic to load the 'Home' page, which might be local or online.
        The View calls this when the user hits the Home button.
        """
        home_url = self.model.get_home_url()
        self.view.set_page_url(home_url)
        self.show_help_window()
