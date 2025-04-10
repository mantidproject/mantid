# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import logging
from mantidqt.widgets.helpwindow.helpwindowmodel import HelpWindowModel
from mantidqt.widgets.helpwindow.helpwindowview import HelpWindowView
from qtpy.QtCore import Qt


class HelpWindowPresenter:
    _logger = logging.getLogger(__name__)

    def __init__(self, parentApp=None, localDocs=None, onlineBaseUrl="https://docs.mantidproject.org/"):
        self._logger.debug(f"Initializing with localDocs='{localDocs}', onlineBaseUrl='{onlineBaseUrl}'")
        self.model = HelpWindowModel(localDocsBase=localDocs, onlineBase=onlineBaseUrl)
        self.parentApp = parentApp
        self._view = None
        self._windowOpen = False

        if self.parentApp:
            try:
                self.parentApp.aboutToQuit.connect(self.cleanup)
                self._logger.debug("Connected cleanup to parentApp.aboutToQuit signal.")
            except AttributeError:
                self._logger.warning("parentApp provided but does not have aboutToQuit signal.")
            except Exception as e:
                self._logger.error(f"Error connecting aboutToQuit signal: {e}")

    def _ensure_view_created(self):
        """
        Creates the View instance if it doesn't exist yet.
        """
        if self._view is None:
            self._logger.debug("Creating HelpWindowView instance.")
            interceptor = self.model.create_request_interceptor()
            modeString = self.model.get_mode_string()
            isLocal = self.model.is_local_docs_mode()
            self._view = HelpWindowView(self, interceptor=interceptor)
            self._view.set_status_indicator(modeString, isLocal)
            self._logger.debug("HelpWindowView instance created.")

    def show_help_page(self, relativeUrl):
        """
        Build the doc URL from the Model and tell the View to load it.
        Ensures the View is created and visible.
        """
        self._ensure_view_created()
        self._logger.debug(f"Requesting help page: '{relativeUrl}'")
        docUrl = self.model.build_help_url(relativeUrl)
        self._view.set_page_url(docUrl)
        self.show_help_window()

    def show_home_page(self):
        """
        Presenter logic to load the 'Home' page, which might be local or online.
        The View calls this when the user hits the Home button.
        """
        self._ensure_view_created()
        self._logger.debug("Requesting home page.")
        homeUrl = self.model.get_home_url()
        self._view.set_page_url(homeUrl)

    def show_help_window(self):
        self._ensure_view_created()
        if not self._windowOpen:
            self._logger.debug("Displaying help window for the first time.")
            self._windowOpen = True
            self._view.display()
        else:
            self._logger.debug("Raising existing help window.")
            self._view.show()
            self._view.raise_()
            self._view.activateWindow()

    def on_close(self):
        """
        Called by the View when the window closes.
        """
        self._logger.debug("View signaled interactive close.")
        self._windowOpen = False

    def cleanup(self):
        """
        Cleanup resources, close the View if open. Called on app quit.
        """
        self._logger.debug("Cleanup requested (likely app quitting).")
        if self._view:
            if self._windowOpen:
                self._logger.info("Closing Help Window view during cleanup.")
                self._view.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)
                self._view.close()
                self._windowOpen = False
            else:
                self._logger.debug("View exists but wasn't marked open during cleanup, ensuring deletion.")
                self._view.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)
                self._view.close()

            self._view = None
        else:
            self._logger.debug("No view instance to clean up.")
