# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

try:
    from mantid.kernel import Logger
except ImportError:
    print("Warning: Mantid Logger not found, using basic print for logging.")

    class Logger:
        def __init__(self, name):
            self._name = name

        def warning(self, msg):
            print(f"WARNING [{self._name}]: {msg}")

        def debug(self, msg):
            print(f"DEBUG [{self._name}]: {msg}")

        def information(self, msg):
            print(f"INFO [{self._name}]: {msg}")

        def error(self, msg):
            print(f"ERROR [{self._name}]: {msg}")


log = Logger("HelpWindowPresenter")

from mantidqt.widgets.helpwindow.helpwindowmodel import HelpWindowModel  # noqa: E402
from mantidqt.widgets.helpwindow.helpwindowview import HelpWindowView  # noqa: E402

from qtpy.QtCore import Qt, QUrl  # noqa: E402


class HelpWindowPresenter:
    def __init__(self, parentApp=None, onlineBaseUrl="https://docs.mantidproject.org/"):
        log.debug(f"Initializing with onlineBaseUrl='{onlineBaseUrl}'")
        try:
            self.model = HelpWindowModel(online_base=onlineBaseUrl)
        except Exception as e:
            log.error(f"Failed to initialize HelpWindowModel: {e}")
            self.model = None
        self.parentApp = parentApp
        self._view = None
        self._windowOpen = False

        if self.parentApp:
            try:
                self.parentApp.aboutToQuit.connect(self.cleanup)
                log.debug("Connected cleanup to parentApp.aboutToQuit signal.")
            except AttributeError:
                log.warning("parentApp provided but does not have aboutToQuit signal.")
            except Exception as e:
                log.error(f"Error connecting aboutToQuit signal: {e}")

    def _ensure_view_created(self):
        """Creates the View instance if it doesn't exist yet."""
        if self._view is None and self.model is not None:
            log.debug("Creating HelpWindowView instance.")
            try:
                interceptor = self.model.create_request_interceptor()
                modeString = self.model.get_mode_string()
                isLocal = self.model.is_local_docs_mode()

                self._view = HelpWindowView(self, interceptor=interceptor)
                self._view.set_status_indicator(modeString, isLocal)
                log.debug("HelpWindowView instance created.")
            except Exception as e:
                log.error(f"Failed to create HelpWindowView: {e}")
                self._view = None
        elif self.model is None:
            log.error("Cannot create view because model initialization failed.")

    def show_help_page(self, relativeUrl):
        """
        Build the doc URL from the Model and tell the View to load it.
        Handles FileNotFoundError from the model for local files.
        Ensures the View is created and visible.
        """
        self._ensure_view_created()
        if not self._view:
            log.error("Cannot show help page, view is not available.")
            return

        log.debug(f"Requesting help page: '{relativeUrl}'")
        try:
            docUrl = self.model.build_help_url(relativeUrl)
            self._view.set_page_url(docUrl)
            self.show_help_window()
        except FileNotFoundError as e:
            log.error(f"File not found for help page '{relativeUrl}': {e}")
            self._view.show_file_not_found_error(str(e), relativeUrl)
            self.show_help_window()
        except Exception as e:
            log.error(f"Error building URL for '{relativeUrl}': {e}")
            self._view.show_generic_error(f"Error generating help URL: {e}", relativeUrl)
            self.show_help_window()

    def show_home_page(self):
        """
        Presenter logic to load the 'Home' page. Handles FileNotFoundError.
        Ensures View exists.
        """
        self._ensure_view_created()
        if not self._view:
            log.error("Cannot show home page, view is not available.")
            return

        log.debug("Requesting home page.")
        try:
            homeUrl = self.model.get_home_url()
            self._view.set_page_url(homeUrl)
        except FileNotFoundError as e:
            log.error(f"File not found for home page: {e}")
            self._view.show_file_not_found_error(str(e), "index.html")
            self.show_help_window()
        except Exception as e:
            log.error(f"Error building home URL: {e}")
            self._view.show_generic_error(f"Error generating home URL: {e}", "index.html")
            self.show_help_window()

    def get_home_url_for_view(self) -> QUrl:
        """Provides the home URL (local or online) to the View for links."""
        if not self.model:
            return QUrl()
        try:
            return self.model.get_home_url()
        except FileNotFoundError:
            log.warning("Home file (index.html) not found for error page link.")
            try:
                online_home = QUrl(self.model._raw_online_base + "/index.html")
                if online_home.isValid():
                    return online_home
            except Exception:
                pass
            return QUrl()
        except Exception as e:
            log.error(f"Error getting home URL for view: {e}")
            return QUrl()

    def show_help_window(self):
        """Ensure the HelpWindowView is created and visible."""
        if self._view is None:
            log.error("Cannot show window, view is not created.")
            self._ensure_view_created()
            if self._view is None:
                return
        if not self._windowOpen:
            log.debug("Displaying help window for the first time.")
            self._windowOpen = True
            self._view.display()
        else:
            log.debug("Raising existing help window.")
            self._view.show()
            self._view.raise_()
            self._view.activateWindow()

    def on_close(self):
        """Called by the View when the window closes interactively."""
        log.debug("View signaled interactive close.")
        self._windowOpen = False

    def cleanup(self):
        """Cleanup resources, close the View if open. Called on app quit."""
        log.debug("Cleanup requested (likely app quitting).")
        if self._view is not None and self._windowOpen:
            log.information("Closing Help Window view during cleanup.")
            self._view.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)
            self._view.close()
            self._windowOpen = False
            self._view = None
        elif self._view is not None:
            log.debug("View exists but wasn't marked open during cleanup, ensuring deletion.")
            self._view.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)
            self._view.close()
            self._view = None
        else:
            log.debug("No view instance to clean up.")
