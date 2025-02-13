import os
from qtpy.QtWebEngineWidgets import QWebEngineView
from qtpy.QtWidgets import QMainWindow, QVBoxLayout, QToolBar, QPushButton, QWidget
from qtpy.QtCore import QUrl
from qtpy.QtGui import QIcon


class HelpWindowView(QMainWindow):
    def __init__(self, presenter):
        super().__init__()
        self.presenter = presenter
        self.setWindowTitle("Python Help Window")
        self.resize(800, 600)

        # Web browser widget
        self.browser = QWebEngineView()

        # Determine initial URL
        local_docs_base = os.environ.get("MANTID_LOCAL_DOCS_BASE")
        if local_docs_base and os.path.isdir(local_docs_base):
            index_path = os.path.join(local_docs_base, "index.html")
            self.browser.setUrl(QUrl.fromLocalFile(index_path))
        else:
            self.browser.setUrl(QUrl("https://docs.mantidproject.org/"))

        # Toolbar with navigation buttons
        self.toolbar = QToolBar("Navigation")
        self.addToolBar(self.toolbar)

        # Back button
        back_button = QPushButton()
        back_button.setIcon(QIcon.fromTheme("go-previous"))
        back_button.setToolTip("Go Back")
        back_button.clicked.connect(self.browser.back)
        self.toolbar.addWidget(back_button)

        # Forward button
        forward_button = QPushButton()
        forward_button.setIcon(QIcon.fromTheme("go-next"))
        forward_button.setToolTip("Go Forward")
        forward_button.clicked.connect(self.browser.forward)
        self.toolbar.addWidget(forward_button)

        # Home button
        home_button = QPushButton()
        home_button.setIcon(QIcon.fromTheme("go-home"))
        home_button.setToolTip("Go Home")
        home_button.clicked.connect(self.go_home)
        self.toolbar.addWidget(home_button)

        # Reload button
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

    def go_home(self):
        """Navigate to the home page."""
        self.browser.setUrl(QUrl("https://docs.mantidproject.org/"))

    def display(self):
        """Show the help window."""
        self.show()

    def closeEvent(self, event):
        """Handle the close event and notify the presenter."""
        self.presenter.on_close()
        super().closeEvent(event)
