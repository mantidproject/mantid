# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import logging
import os
from html import escape
from qtpy.QtWidgets import (
    QMainWindow,
    QVBoxLayout,
    QHBoxLayout,
    QToolBar,
    QPushButton,
    QWidget,
    QLabel,
    QSizePolicy,
    QLineEdit,
    QFrame,
    QShortcut,
    QApplication,
    QStyle,
)
from qtpy.QtWidgets import QTextBrowser
from qtpy.QtCore import QUrl, Qt, QEvent
from qtpy.QtGui import QIcon, QKeySequence, QDesktopServices


class NavigationTextBrowser(QTextBrowser):
    """QTextBrowser that intercepts extra mouse buttons for navigation."""

    def __init__(self, *args, **kwargs):
        self._logger = logging.getLogger(__name__)
        super().__init__(*args, **kwargs)

    def mousePressEvent(self, event):
        btn = event.button()
        if btn in (Qt.BackButton, Qt.XButton1):
            self._logger.debug("NavigationTextBrowser: Back mouse button pressed")
            if self.isBackwardAvailable():
                self.backward()
                event.accept()
                return  # Consume event
        elif btn in (Qt.ForwardButton, Qt.XButton2):
            self._logger.debug("NavigationTextBrowser: Forward mouse button pressed")
            if self.isForwardAvailable():
                self.forward()
                event.accept()
                return  # Consume event
        # Fallback to default behaviour
        super().mousePressEvent(event)


class HelpWindowView(QMainWindow):
    _logger = logging.getLogger(__name__)

    def __init__(self, presenter, interceptor=None, parent=None):
        super().__init__(parent)
        self.presenter = presenter
        self._logger.debug("Initializing HelpWindowView.")
        self.setWindowTitle("Python Help Window")
        # Use Tool window flag to allow floating above modal dialogs
        self.setWindowFlags(Qt.Window | Qt.Tool)
        self.resize(1024, 768)

        # Central Widget and Layout
        container = QWidget()
        layout = QVBoxLayout(container)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setCentralWidget(container)

        # Create the custom QTextBrowser (will be added to layout later)
        self.browser = NavigationTextBrowser()

        # Configure QTextBrowser settings
        self.browser.setReadOnly(True)
        self.browser.setOpenExternalLinks(True)  # Open external links in system browser
        self.browser.setOpenLinks(True)  # Enable automatic link navigation for local files

        # Note: interceptor parameter is ignored for QTextBrowser (no request interception needed)
        if interceptor is not None:
            self._logger.debug(f"HelpWindow: Interceptor {type(interceptor).__name__} not needed for QTextBrowser")

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
        self.backButton.setIcon(self.style().standardIcon(QStyle.SP_ArrowBack))
        self.backButton.setToolTip("Go Back")
        self.backButton.clicked.connect(self.browser.backward)
        self.toolbar.addWidget(self.backButton)

        # Forward
        self.forwardButton.setIcon(self.style().standardIcon(QStyle.SP_ArrowForward))
        self.forwardButton.setToolTip("Go Forward")
        self.forwardButton.clicked.connect(self.browser.forward)
        self.toolbar.addWidget(self.forwardButton)

        # Home
        self.homeButton.setIcon(QIcon(":/home.png"))
        self.homeButton.setToolTip("Go to Home Page")
        self.homeButton.clicked.connect(self.on_home_clicked)
        self.toolbar.addWidget(self.homeButton)

        # Reload
        self.reloadButton.setIcon(self.style().standardIcon(QStyle.SP_BrowserReload))
        self.reloadButton.setToolTip("Reload Current Page")
        self.reloadButton.clicked.connect(self.browser.reload)
        self.toolbar.addWidget(self.reloadButton)

        spacer = QWidget()
        spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        self.toolbar.addWidget(spacer)

        # Find button in toolbar
        self.findButton = QPushButton()
        self.findButton.setIcon(QIcon(":/search.png"))
        self.findButton.setToolTip("Find in Page (Ctrl+F)")
        self.findButton.clicked.connect(self.show_find_toolbar)
        self.toolbar.addWidget(self.findButton)

        # Status Label (Icon + Text)
        self.statusLabel = QLabel("Status: Initializing...")
        self.statusLabel.setToolTip("Indicates whether documentation is loaded locally (Offline) or from the web (Online)")

        self.statusLabel.setStyleSheet("QLabel { padding-left: 5px; padding-right: 5px; margin-left: 5px; }")
        self.toolbar.addWidget(self.statusLabel)

        # Create Find Toolbar (initially hidden)
        self.setup_find_toolbar(layout)

        # Add browser after find toolbar
        layout.addWidget(self.browser)

        # Setup keyboard shortcuts
        self.setup_shortcuts()

        # Connect signals for enabling/disabling buttons
        self.browser.sourceChanged.connect(self.update_navigation_buttons)
        # Connect backwardAvailable and forwardAvailable signals
        self.browser.backwardAvailable.connect(self.backButton.setEnabled)
        self.browser.forwardAvailable.connect(self.forwardButton.setEnabled)
        self.update_navigation_buttons()

        QApplication.instance().installEventFilter(self)

    def setup_find_toolbar(self, parent_layout):
        """Create and setup the find toolbar widget."""
        self.find_frame = QFrame()
        self.find_frame.setFrameStyle(QFrame.StyledPanel)
        # Shrink size of the toolbar – subtle background, thinner borders
        self.find_frame.setStyleSheet(
            """
            QFrame {
                background-color: #fafafa;
                border: 1px solid #ccc;
                border-radius: 2px;
            }
            QLineEdit {
                padding: 2px 4px;
            }
        """
        )

        # Limit the visual height of the find toolbar
        self.find_frame.setMaximumHeight(32)
        self.find_frame.setVisible(False)

        find_layout = QHBoxLayout(self.find_frame)
        # Compact margins & spacing
        find_layout.setContentsMargins(4, 2, 4, 2)
        find_layout.setSpacing(4)

        # Find label
        find_label = QLabel("Find:")
        find_layout.addWidget(find_label)

        # Find input
        self.find_input = QLineEdit()
        self.find_input.setPlaceholderText("Search…")
        self.find_input.setFixedHeight(24)
        self.find_input.returnPressed.connect(self.find_next)
        self.find_input.textChanged.connect(self.find_text_changed)
        find_layout.addWidget(self.find_input)

        # Previous button
        self.find_prev_button = QPushButton("Prev")
        self.find_prev_button.setToolTip("Find Previous (Shift+F3)")
        self.find_prev_button.setFixedHeight(24)
        self.find_prev_button.clicked.connect(self.find_previous)
        find_layout.addWidget(self.find_prev_button)

        # Next button
        self.find_next_button = QPushButton("Next")
        self.find_next_button.setToolTip("Find Next (F3)")
        self.find_next_button.setFixedHeight(24)
        self.find_next_button.clicked.connect(self.find_next)
        find_layout.addWidget(self.find_next_button)

        # Close button
        self.find_close_button = QPushButton("×")
        self.find_close_button.setFixedSize(20, 20)
        self.find_close_button.setToolTip("Close (Escape)")
        self.find_close_button.setStyleSheet("QPushButton { font-weight: bold; }")
        self.find_close_button.clicked.connect(self.hide_find_toolbar)
        find_layout.addWidget(self.find_close_button)

        parent_layout.addWidget(self.find_frame)

    def setup_shortcuts(self):
        """Setup keyboard shortcuts for find functionality."""
        # Ctrl+F to show find
        self.show_find_shortcut = QShortcut(QKeySequence("Ctrl+F"), self)
        self.show_find_shortcut.activated.connect(self.show_find_toolbar)

        # Escape to hide find
        self.hide_find_shortcut = QShortcut(QKeySequence("Escape"), self)
        self.hide_find_shortcut.activated.connect(self.hide_find_toolbar)

        # F3 for find next
        self.find_next_shortcut = QShortcut(QKeySequence("F3"), self)
        self.find_next_shortcut.activated.connect(self.find_next)

        # Shift+F3 for find previous
        self.find_prev_shortcut = QShortcut(QKeySequence("Shift+F3"), self)
        self.find_prev_shortcut.activated.connect(self.find_previous)

    def show_find_toolbar(self):
        """Show the find toolbar and focus the input field."""
        self.find_frame.setVisible(True)
        self.find_input.setFocus()
        self.find_input.selectAll()
        self._logger.debug("Find toolbar shown")

    def hide_find_toolbar(self):
        """Hide the find toolbar and clear any search highlights."""
        self.find_frame.setVisible(False)
        # Clear search by searching for empty string
        self.browser.find("")
        self.browser.setFocus()
        self._logger.debug("Find toolbar hidden")

    def find_text_changed(self):
        """Called when find text changes - performs live search."""
        search_text = self.find_input.text()
        if search_text:
            self.browser.find(search_text)
        else:
            # Clear highlights when text is empty
            self.browser.find("")

    def find_next(self):
        """Find next occurrence of the search text."""
        search_text = self.find_input.text()
        if search_text:
            self.browser.find(search_text)
            self._logger.debug(f"Finding next: {search_text}")

    def find_previous(self):
        """Find previous occurrence of the search text."""
        search_text = self.find_input.text()
        if search_text:
            # Use QTextDocument.FindBackward flag for previous search
            self.browser.find(search_text, QTextBrowser.FindBackward)
            self._logger.debug(f"Finding previous: {search_text}")

    def handle_load_finished(self, ok: bool):
        """Slot handling resource load completion. Shows generic error on failure."""
        self._logger.debug(f"Resource load finished. Success: {ok}")
        self.update_navigation_buttons()  # Update nav state regardless

        if not ok:
            # Load failed
            failed_url = self.browser.source()
            failed_url_str = escape(failed_url.toString())
            self._logger.error(f"Failed to load page: {failed_url_str}")

            # Display a generic error page
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
                <p>This could be due to network issues, server errors, or local file permission problems.</p>
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
            <p>Please check that your local documentation is correctly built.</p>
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

    def update_navigation_buttons(self, src: QUrl = None):
        """
        Enable/disable back/forward buttons based on browser history.
        """
        self.backButton.setEnabled(self.browser.isBackwardAvailable())
        self.forwardButton.setEnabled(self.browser.isForwardAvailable())
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
        For local files, loads in QTextBrowser. For online URLs, opens in system browser.
        """
        if url.isValid():
            self._logger.debug(f"Loading URL: {url.toString()}")

            # Handle local files directly in QTextBrowser
            if url.isLocalFile():
                if url != self.browser.source():
                    doc_dir = url.toLocalFile()
                    if os.path.isfile(doc_dir):
                        doc_dir = os.path.dirname(doc_dir)
                    # Set the search paths so QTextBrowser can find relative resources
                    self.browser.setSearchPaths([doc_dir])
                    self._logger.debug(f"Set search path to: {doc_dir}")
                    self.browser.setSource(url)
                else:
                    self._logger.debug(f"URL already loaded: {url.toString()}")
            # Handle online URLs by opening in system browser
            elif url.scheme() in ("http", "https"):
                self._logger.debug(f"Opening online documentation in system browser: {url.toString()}")
                QDesktopServices.openUrl(url)
                # Show a message in the help window
                self.browser.setHtml(
                    f"<html><body style='padding: 20px; font-family: sans-serif;'>"
                    f"<h2>Online Documentation</h2>"
                    f"<p>Online documentation has been opened in your default web browser.</p>"
                    f"<p>If the browser did not open automatically, please visit:</p>"
                    f"<p><a href='{escape(url.toString())}'>{escape(url.toString())}</a></p>"
                    f"</body></html>"
                )
            else:
                self._logger.warning(f"Unsupported URL scheme: {url.scheme()}")
        else:
            self._logger.warning(f"Attempted to load invalid URL: {url.toString()}")

    def display(self):
        """
        Show the window on screen and bring it to front.
        """
        self._logger.debug("Displaying window.")
        self.show()
        self.raise_()
        self.activateWindow()

    def closeEvent(self, event):
        """
        Handle window close: notify the Presenter so it can do cleanup if needed.
        """
        self._logger.debug("Close event triggered.")
        self.presenter.on_close()
        super().closeEvent(event)

    def eventFilter(self, obj, event):
        if event.type() == QEvent.MouseButtonPress and obj is not self.browser:
            btn = event.button()
            if btn in (Qt.BackButton, Qt.XButton1):
                self._logger.debug("Global eventFilter: Back mouse button detected (non-browser widget)")
                if self.browser.isBackwardAvailable():
                    self.browser.backward()
                    return True
            elif btn in (Qt.ForwardButton, Qt.XButton2):
                self._logger.debug("Global eventFilter: Forward mouse button detected (non-browser widget)")
                if self.browser.isForwardAvailable():
                    self.browser.forward()
                    return True
        return super().eventFilter(obj, event)
