# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QMainWindow, QVBoxLayout, QToolBar, QPushButton, QWidget
from qtpy.QtWebEngineWidgets import QWebEngineView, QWebEngineSettings
from qtpy.QtCore import QUrl
from qtpy.QtGui import QIcon


class HelpWindowView(QMainWindow):
    def __init__(self, presenter, interceptor=None):
        super().__init__()
        self.presenter = presenter
        self.setWindowTitle("Python Help Window")
        self.resize(800, 600)

        # Create the QWebEngineView
        self.browser = QWebEngineView()

        # Allow local file:// docs to load remote resources (fonts, scripts, etc.)
        QWebEngineSettings.globalSettings().setAttribute(QWebEngineSettings.LocalContentCanAccessRemoteUrls, True)

        # If the interceptor is not None, apply it to the current profile
        if interceptor is not None:
            profile = self.browser.page().profile()
            profile.setUrlRequestInterceptor(interceptor)

        # Toolbar with navigation buttons
        self.toolbar = QToolBar("Navigation")
        self.addToolBar(self.toolbar)

        # Back
        back_button = QPushButton()
        back_button.setIcon(QIcon.fromTheme("go-previous"))
        back_button.setToolTip("Go Back")
        back_button.clicked.connect(self.browser.back)
        self.toolbar.addWidget(back_button)

        # Forward
        forward_button = QPushButton()
        forward_button.setIcon(QIcon.fromTheme("go-next"))
        forward_button.setToolTip("Go Forward")
        forward_button.clicked.connect(self.browser.forward)
        self.toolbar.addWidget(forward_button)

        # Home
        home_button = QPushButton()
        home_button.setIcon(QIcon.fromTheme("go-home"))
        home_button.setToolTip("Go Home")
        home_button.clicked.connect(self.on_home_clicked)
        self.toolbar.addWidget(home_button)

        # Reload
        reload_button = QPushButton()
        reload_button.setIcon(QIcon.fromTheme("view-refresh"))
        reload_button.setToolTip("Reload")
        reload_button.clicked.connect(self.browser.reload)
        self.toolbar.addWidget(reload_button)

        # Layout
        layout = QVBoxLayout()
        layout.addWidget(self.browser)
        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

    def on_home_clicked(self):
        """
        Notifies the Presenter that the user wants to go "Home."
        The Presenter decides whether it's local index.html or online docs.
        """
        self.presenter.show_home_page()

    def set_page_url(self, url: QUrl):
        """The Presenter calls this to load the desired doc page."""
        self.browser.setUrl(url)

    def display(self):
        """Show the window on screen."""
        self.show()

    def closeEvent(self, event):
        """Handle window close: notify the Presenter so it can do cleanup."""
        self.presenter.on_close()
        super().closeEvent(event)
