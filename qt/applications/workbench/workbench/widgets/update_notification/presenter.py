from qtpy.QtCore import QObject, Signal, Qt
from qtpy.QtWidgets import QMessageBox
from mantid.api import AlgorithmObserver, AlgorithmManager
from mantid.kernel import ConfigService, logger


class _UpdateSignalBridge(QObject):
    updateAvailable = Signal(str)


class UpdateNotificationObserver(AlgorithmObserver):
    """
    This class is a bridge to allow the AlgorithmObserver to emit a Qt signal when an update is available.
    The AlgorithmObserver is not a QObject and cannot emit signals directly, so we use this bridge to
    communicate between the observer and the presenter.
    """

    def __init__(self, bridge: _UpdateSignalBridge):
        super().__init__()
        self._bridge = bridge
        self.alg = None

    def setAlg(self, alg):
        self.alg = alg

    def finishHandle(self):
        if not self.alg.getProperty("IsNewVersionAvailable").value:
            return
        latest = self.alg.getPropertyValue("MostRecentVersion")
        self._bridge.updateAvailable.emit(latest)


class UpdateNotificationPresenter:
    """
    This class is responsible for checking for updates and showing a notification to the user if an update is available.
    It uses the UpdateNotificationObserver to observe the CheckMantidVersion algorithm and emit a signal if
    an update is available. The presenter then shows a QMessageBox to the user with the option to update now or remind later.
    """

    def __init__(self, parent):
        self._parent = parent
        self._bridge = _UpdateSignalBridge()
        self._bridge.updateAvailable.connect(self._show_prompt, Qt.QueuedConnection)
        self._observer = None  # keep alive for the life of the async call
        self.alg = None

    def check_for_update(self):
        try:
            self.alg = AlgorithmManager.create("CheckMantidVersion")
            self.alg.initialize()
            self.alg.setChild(True)
            self.alg.setAlgStartupLogging(False)
            self._observer = UpdateNotificationObserver(self._bridge)
            self._observer.setAlg(self.alg)
            self._observer.observeFinish(self.alg)
            self.alg.execute()
        except Exception as e:
            logger.error(f"Failed to check for updates: {e}")

    def _show_prompt(self, latest_version):
        box = QMessageBox(self._parent)
        box.setIcon(QMessageBox.Information)
        box.setWindowTitle("Update available")
        box.setText(f"Mantid Workbench {latest_version} is available!")
        box.setInformativeText(
            "Would you like to open the download page now? This will close the Mantid Workbench if you choose to update now."
        )
        update_btn = box.addButton("Update now", QMessageBox.AcceptRole)
        box.addButton("Remind me later", QMessageBox.RejectRole)
        box.exec()
        if box.clickedButton() == update_btn:
            from qtpy.QtGui import QDesktopServices
            from qtpy.QtCore import QUrl

            download_url = ConfigService.getString("CheckMantidVersion.DownloadURL")
            try:
                QDesktopServices.openUrl(QUrl(download_url))
            except Exception as exc:
                logger.error(f"Failed to open download URL: {exc}")

            try:
                self._parent.close()
            except Exception as exc:
                logger.error(f"Failed to close Workbench: {exc}")
