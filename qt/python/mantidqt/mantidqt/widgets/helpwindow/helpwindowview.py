# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import logging
from html import escape
from qtpy.QtWidgets import (
    QMainWindow,
    QVBoxLayout,
    QToolBar,
    QPushButton,
    QWidget,
    QLabel,
    QSizePolicy,
)
from qtpy.QtWebEngineWidgets import QWebEngineView, QWebEngineSettings
from qtpy.QtCore import QUrl, Qt
from qtpy.QtGui import QIcon


class HelpWindowView(QMainWindow):
    _logger = logging.getLogger(__name__)

    def __init__(self, presenter, interceptor=None):
        super().__init__()
        self.presenter = presenter
        self._logger.debug("Initializing HelpWindowView.")
        self.setWindowTitle("Python Help Window")
        self.resize(1024, 768)

        # Central Widget and Layout
        container = QWidget()
        layout = QVBoxLayout(container)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setCentralWidget(container)

        # Create the QWebEngineView
        self.browser = QWebEngineView()
        layout.addWidget(self.browser)

        # Configure Web Engine Settings
        settings = self.browser.settings()
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessRemoteUrls, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.ScrollAnimatorEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.PluginsEnabled, False)
        # settings.setAttribute(QWebEngineSettings.JavascriptEnabled, True) # JS is enabled by default!

        # If the interceptor is not None, apply it to the current profile
        if interceptor is not None:
            profile = self.browser.page().profile()
            profile.setUrlRequestInterceptor(interceptor)
            self._logger.debug(f"HelpWindow: Applied URL interceptor: {type(interceptor).__name__}")

        # Toolbar with navigation buttons
        self.toolbar = QToolBar("Navigation")
        self.toolbar.setMovable(False)
        self.addToolBar(Qt.ToolBarArea.TopToolBarArea, self.toolbar)

        # --- Store references to buttons ---
        self.backButton = QPushButton()
        self.forwardButton = QPushButton()
        self.reloadButton = QPushButton()
        self.homeButton = QPushButton()
        # ---------------------------------

        # Back
        self.backButton.setIcon(QIcon.fromTheme("go-previous", QIcon(":/qt-project.org/styles/commonstyle/images/left-arrow-32.png")))
        self.backButton.setToolTip("Go Back")
        self.backButton.clicked.connect(self.browser.back)
        self.toolbar.addWidget(self.backButton)

        # Forward
        self.forwardButton.setIcon(QIcon.fromTheme("go-next", QIcon(":/qt-project.org/styles/commonstyle/images/right-arrow-32.png")))
        self.forwardButton.setToolTip("Go Forward")
        self.forwardButton.clicked.connect(self.browser.forward)
        self.toolbar.addWidget(self.forwardButton)

        # Home
        self.homeButton.setIcon(QIcon.fromTheme("go-home", QIcon(":/qt-project.org/styles/commonstyle/images/home-32.png")))
        self.homeButton.setToolTip("Go to Home Page")
        self.homeButton.clicked.connect(self.on_home_clicked)
        self.toolbar.addWidget(self.homeButton)

        # Reload
        self.reloadButton.setIcon(QIcon.fromTheme("view-refresh", QIcon(":/qt-project.org/styles/commonstyle/images/refresh-32.png")))
        self.reloadButton.setToolTip("Reload Current Page")
        self.reloadButton.clicked.connect(self.browser.reload)
        self.toolbar.addWidget(self.reloadButton)

        spacer = QWidget()
        spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        self.toolbar.addWidget(spacer)

        # Status Label (Icon + Text)
        self.statusLabel = QLabel("Status: Initializing...")
        self.statusLabel.setToolTip("Indicates whether documentation is loaded locally (Offline) or from the web (Online)")

        self.statusLabel.setStyleSheet("QLabel { padding-left: 5px; padding-right: 5px; margin-left: 5px; }")
        self.toolbar.addWidget(self.statusLabel)

        # Connect signals for enabling/disabling buttons
        self.browser.urlChanged.connect(self.update_navigation_buttons)
        # Connect loadFinished to the new handler
        self.browser.loadFinished.connect(self.handle_load_finished)
        self.update_navigation_buttons()

    def handle_load_finished(self, ok: bool):
        """Slot handling the loadFinished signal. Shows generic error on failure."""
        self._logger.debug(f"Load finished signal received. Success: {ok}")
        self.update_navigation_buttons()  # Update nav state regardless

        if not ok:
            # Load failed for reasons OTHER than local file not found
            failed_url = self.browser.url()
            failed_url_str = escape(failed_url.toString())
            # Use self._logger consistently
            self._logger.error(f"Failed to load page (WebEngine signal): {failed_url_str}")

            # Display a more generic error page now
            error_html = f"""<!DOCTYPE html>
            <html><head><title>Error Loading Page</title>
            <style>
                body {{ font-family: sans-serif; padding: 2em; color: #333; }}
                h2 {{ color: #d9534f; }}
                code {{
                    background-color: #f0f0f0; padding: 0.2em 0.4em;
                    border-radius: 3px; font-family: monospace;
                    word-break: break-all;
                }}
            </style>
            </head>
            <body>
                <h2>Page Load Error</h2>
                <p>Sorry, the help page could not be loaded:</p>
                <p><code>{failed_url_str}</code></p>
                <p>This could be due to network issues, online server errors, local file permission problems, or other loading errors.</p>
            </body></html>
            """

            # Use baseUrl of the failed request for context
            self.browser.setHtml(error_html, baseUrl=failed_url)

    def show_file_not_found_error(self, error_message: str, requested_url: str):
        """Displays a specific error page when the Model/Presenter signals a local file wasn't found."""
        self._logger.debug(f"Displaying FileNotFoundError page for requested relative URL: {requested_url}")
        error_message_html = escape(error_message)
        requested_url_html = escape(requested_url)

        # Try to get home link
        home_link_html = ""
        try:
            home_url_obj = self.presenter.get_home_url_for_view()
            if home_url_obj and home_url_obj.isValid():
                # Add target="_self" to ensure it loads in the help window
                home_link_html = f"<p><a href='{escape(home_url_obj.toString())}' target='_self'>Go to Home Page</a></p>"
        except AttributeError:
            self._logger.warning("Presenter does not have get_home_url_for_view method.")
        except Exception as e:
            self._logger.error(f"Error getting home URL for error page: {e}")

        error_html = f"""<!DOCTYPE html>
        <html>
        <head><title>File Not Found</title>
        <style>
            body {{ font-family: sans-serif; padding: 2em; color: #333; }}
            h2 {{ color: #d9534f; }}
            code {{
                background-color: #f0f0f0; padding: 0.2em 0.4em;
                border-radius: 3px; font-family: monospace;
                word-break: break-all;
            }}
        </style>
        </head>
        <body>
            <h2>Help File Not Found</h2>
            <p>Sorry, the requested local documentation file could not be found.</p>
            <p>Requested: <code>{requested_url_html}</code></p>
            <p>Details: <code>{error_message_html}</code></p>
            <p>Please check that your local documentation is correctly built
               and the path is correctly configured in Mantid ('docs.html.root').</p>
            {home_link_html}
        </body>
        </html>
        """

        # Use an empty QUrl or the home URL as the base for setHtml
        base_url_for_error = QUrl()
        try:
            base_url_for_error = self.presenter.get_home_url_for_view() or QUrl()
        except Exception:
            pass
        self.browser.setHtml(error_html, baseUrl=base_url_for_error)

    def show_generic_error(self, error_message: str, requested_url: str):
        """Displays a generic error page based on message from presenter (e.g., URL build errors)."""
        # Use self._logger consistently
        self._logger.debug(f"Displaying generic error page for: {requested_url}")
        error_message_html = escape(error_message)
        requested_url_html = escape(requested_url)
        error_html = f"""<!DOCTYPE html>
        <html><head><title>Error</title>
        <style>
            body {{ font-family: sans-serif; padding: 2em; color: #333; }}
            h2 {{ color: #d9534f; }}
            code {{
                background-color: #f0f0f0; padding: 0.2em 0.4em;
                border-radius: 3px; font-family: monospace;
                word-break: break-all;
            }}
        </style>
        </head>
        <body>
            <h2>Help Window Error</h2>
            <p>An error occurred while trying to display help for:</p>
            <p><code>{requested_url_html}</code></p>
            <p>Details: {error_message_html}</p>
        </body></html>
        """
        self.browser.setHtml(error_html)

    def set_status_indicator(self, modeText: str, isLocal: bool):
        """
        Updates the status label text and icon.
        """
        if isLocal:
            tooltip = f"Showing {modeText} from local disk."
        else:
            tooltip = f"Showing {modeText} from the web."

        colorStyle = "color: green;"
        self.statusLabel.setStyleSheet(f"QLabel {{ padding-left: 5px; padding-right: 5px; margin-left: 5px; {colorStyle} }}")
        self.statusLabel.setText(f"{modeText}")
        self.statusLabel.setToolTip(tooltip)
        self._logger.debug(f"HelpWindow View: Status set to '{modeText}' (Local: {isLocal})")

    def update_navigation_buttons(self, ok: bool = True):
        """
        Enable/disable back/forward buttons based on browser history.
        """
        history = self.browser.history() if hasattr(self.browser, "history") else None
        canGoBack = history.canGoBack() if history else False
        canGoForward = history.canGoForward() if history else False
        self.backButton.setEnabled(canGoBack)
        self.forwardButton.setEnabled(canGoForward)
        self.reloadButton.setEnabled(True)

    def on_home_clicked(self):
        """
        Notifies the Presenter that the user wants to go "Home."
        The Presenter decides whether it's local index.html or online docs.
        """
        self._logger.debug("Home button clicked.")
        self.presenter.show_home_page()

    def set_page_url(self, url: QUrl):
        """
        The Presenter calls this to load the desired doc page.
        """
        if url.isValid() and url != self.browser.url():
            self._logger.debug(f"Loading URL: {url.toString()}")
            self.browser.setUrl(url)
        elif not url.isValid():
            self._logger.warning(f"Attempted to load invalid URL: {url.toString()}")
        else:
            self._logger.debug(f"URL already loaded: {url.toString()}")

    def display(self):
        """
        Show the window on screen.
        """
        self._logger.debug("Displaying window.")
        self.show()

    def closeEvent(self, event):
        """
        Handle window close: notify the Presenter so it can do cleanup if needed.
        """
        self._logger.debug("Close event triggered.")
        self.presenter.on_close()
        super().closeEvent(event)
