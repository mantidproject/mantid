from qtpy.QtCore import Qt
from qtpy.QtWidgets import QApplication


def is_dark_mode() -> bool:
    app = QApplication.instance()
    return app is not None and app.styleHints().colorScheme() == Qt.ColorScheme.Dark


THEME_FG = "w" if is_dark_mode() else "k"

DARK_FACECOLOR = "#1d1d1d"
