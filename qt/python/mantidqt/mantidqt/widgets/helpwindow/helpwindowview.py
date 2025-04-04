# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import logger
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
    def __init__(self, presenter, interceptor=None):
        super().__init__()
        self.presenter = presenter
        logger.debug("Initializing HelpWindowView.")
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
            logger.debug(f"HelpWindow: Applied URL interceptor: {type(interceptor).__name__}")

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

        # --- Add Status Indicator ---
        # Spacer to push the status label to the right
        spacer = QWidget()
        spacer.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        self.toolbar.addWidget(spacer)

        # Status Label (Icon + Text)
        self.statusLabel = QLabel("Status: Initializing...")  # Initial text
        self.statusLabel.setToolTip("Indicates whether documentation is loaded locally (Offline) or from the web (Online)")
        # Add some padding/margin for aesthetics
        self.statusLabel.setStyleSheet("QLabel { padding-left: 5px; padding-right: 5px; margin-left: 5px; }")
        self.toolbar.addWidget(self.statusLabel)
        # ---------------------------

        # Connect signals for enabling/disabling buttons
        self.browser.urlChanged.connect(self.update_navigation_buttons)
        self.browser.loadFinished.connect(self.update_navigation_buttons)
        self.update_navigation_buttons()

    def set_status_indicator(self, modeText: str, isLocal: bool):
        """
        Updates the status label text and icon.
        """
        if isLocal:
            iconThemeName = "network-offline"
            fallbackIcon = QIcon(":/qt-project.org/styles/commonstyle/images/disconnected-32.png")
            tooltip = f"Showing {modeText} from local disk."
            colorStyle = "color: green;"
        else:
            iconThemeName = "network-wired"
            fallbackIcon = QIcon(":/qt-project.org/styles/commonstyle/images/connected-32.png")
            tooltip = f"Showing {modeText} from the web."
            colorStyle = "color: green;"

        icon = QIcon.fromTheme(iconThemeName, fallbackIcon)  # noqa: F841
        self.statusLabel.setStyleSheet(f"QLabel {{ padding-left: 5px; padding-right: 5px; margin-left: 5px; {colorStyle} }}")
        self.statusLabel.setText(f"{modeText}")
        self.statusLabel.setToolTip(tooltip)
        logger.debug(f"HelpWindow View: Status set to '{modeText}' (Local: {isLocal})")

    def update_navigation_buttons(self, ok: bool = True):
        """
        Enable/disable back/forward buttons based on browser history.
        """
        canGoBack = self.browser.history().canGoBack()
        canGoForward = self.browser.history().canGoForward()
        self.backButton.setEnabled(canGoBack)
        self.forwardButton.setEnabled(canGoForward)
        self.reloadButton.setEnabled(True)

    def on_home_clicked(self):
        """
        Notifies the Presenter that the user wants to go "Home."
        The Presenter decides whether it's local index.html or online docs.
        """
        logger.debug("Home button clicked.")
        self.presenter.show_home_page()

    def set_page_url(self, url: QUrl):
        """
        The Presenter calls this to load the desired doc page.
        """
        if url.isValid() and url != self.browser.url():
            logger.debug(f"Loading URL: {url.toString()}")
            self.browser.setUrl(url)
        elif not url.isValid():
            logger.warning(f"Attempted to load invalid URL: {url.toString()}")
        else:
            logger.debug(f"URL already loaded: {url.toString()}")

    def display(self):
        """
        Show the window on screen.
        """
        logger.debug("Displaying window.")
        self.show()

    def closeEvent(self, event):
        """
        Handle window close: notify the Presenter so it can do cleanup if needed.
        """
        logger.debug("Close event triggered.")
        self.presenter.on_close()
        super().closeEvent(event)
