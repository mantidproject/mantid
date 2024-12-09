from qtpy.QtWidgets import QMainWindow, QVBoxLayout, QWidget
from qtpy.QtWebEngineWidgets import QWebEngineView


class HelpWindowView(QMainWindow):
    def __init__(self, presenter):
        super().__init__()
        self.presenter = presenter
        self.setWindowTitle("Python Help Window")
        self.resize(800, 600)

        # Web view to display the help documentation
        self.browser = QWebEngineView()
        self.browser.setUrl("https://docs.mantidproject.org/")

        # Layout
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
