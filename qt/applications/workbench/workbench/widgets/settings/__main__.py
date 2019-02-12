from qtpy.QtWidgets import QApplication  # noqa: F402

from workbench.widgets.settings.presenter import SettingsPresenter

app = QApplication([])
settings = SettingsPresenter(None)
settings.view.show()
app.exec_()
