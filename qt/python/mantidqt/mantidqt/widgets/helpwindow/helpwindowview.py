from qtpy.QtCore import QUrl
from qtpy.QtWidgets import QVBoxLayout, QWidget, QPushButton, QHBoxLayout
from qtpy.QtWebEngineWidgets import QWebEngineView


class HelpView(QWidget):
    def __init__(self, presenter, parent=None):
        super().__init__(parent)
        self.presenter = presenter

        self.setWindowTitle("Mantid Help Window")
        self.setGeometry(100, 100, 1200, 800)

        # Web view setup
        self.webview = QWebEngineView()
        self.webview.urlChanged.connect(self.presenter.handle_url_changed)

        # Navigation buttons
        self.back_button = QPushButton("Back")
        self.back_button.clicked.connect(self.presenter.handle_go_back)
        self.forward_button = QPushButton("Forward")
        self.forward_button.clicked.connect(self.presenter.handle_go_forward)
        self.home_button = QPushButton("Home")
        self.home_button.clicked.connect(self.presenter.handle_go_home)

        nav_layout = QHBoxLayout()
        nav_layout.addWidget(self.back_button)
        nav_layout.addWidget(self.forward_button)
        nav_layout.addWidget(self.home_button)

        # Layout setup
        main_layout = QVBoxLayout(self)
        main_layout.addLayout(nav_layout)
        main_layout.addWidget(self.webview)

    def set_url(self, url):
        """Set the URL in the webview."""
        self.webview.setUrl(QUrl(url))

    def update_navigation_buttons(self, can_go_back, can_go_forward):
        """Enable or disable navigation buttons."""
        self.back_button.setEnabled(can_go_back)
        self.forward_button.setEnabled(can_go_forward)
