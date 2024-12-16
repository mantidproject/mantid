import os
from qtpy.QtWidgets import QMainWindow, QVBoxLayout, QWidget
from qtpy.QtWebEngineWidgets import QWebEngineView
from qtpy.QtCore import QUrl


class HelpWindowView(QMainWindow):
    def __init__(self, presenter):
        super().__init__()
        self.presenter = presenter
        self.setWindowTitle("Python Help Window")
        self.resize(800, 600)

        self.browser = QWebEngineView()

        # Determine initial URL:
        local_docs_base = os.environ.get("MANTID_LOCAL_DOCS_BASE")
        if local_docs_base and os.path.isdir(local_docs_base):
            # Use local docs if available
            index_path = os.path.join(local_docs_base, "index.html")
            self.browser.setUrl(QUrl.fromLocalFile(index_path))
        else:
            # Fallback to online docs
            self.browser.setUrl(QUrl("https://docs.mantidproject.org/"))

        layout = QVBoxLayout()
        layout.addWidget(self.browser)
        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

    def display(self):
        """Show the help window."""
        self.show()

    def closeEvent(self, event):
        """Handle the close event and notify the presenter."""
        self.presenter.on_close()
        super().closeEvent(event)
