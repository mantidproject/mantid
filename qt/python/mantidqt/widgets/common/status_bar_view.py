from qtpy.QtWidgets import QMainWindow, QStatusBar


class StatusBarView(QMainWindow):
    def __init__(self, parent, central_widget):
        super(StatusBarView, self).__init__(parent)
        self.setCentralWidget(central_widget)
        self.status_bar = QStatusBar(self)
        self.setStatusBar(self.status_bar)
        self.resize(600, 400)
