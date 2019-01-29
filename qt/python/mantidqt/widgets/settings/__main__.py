from qtpy.QtWidgets import QApplication  # noqa: F402


app = QApplication([])
from mantidqt.widgets.settings.view import SettingsView
settings = SettingsView()
settings.show()
app.exec_()
