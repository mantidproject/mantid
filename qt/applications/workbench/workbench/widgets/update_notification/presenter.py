from qtpy.QtCore import QObject, Signal, Qt
from qtpy.QtWidgets import QMessageBox
from mantid.api import AlgorithmObserver, AlgorithmManager
from mantid.kernel import ConfigService, logger


class _UpdateSignalBridge(QObject):
    updateAvailable = Signal(str)


class UpdateNotificationObserver(AlgorithmObserver):
    def __init__(self, bridge: _UpdateSignalBridge):
        super().__init__()
        self._bridge = bridge
        self.alg = None

    def setAlg(self, alg):
        self.alg = alg

    def finishHandle(self):
        # if not self.alg.getProperty("IsNewVersionAvailable").value:
        #     return
        # latest = self.alg.getPropertyValue("MostRecentVersion")
        latest = "7.0.0"
        self._bridge.updateAvailable.emit(latest)


class UpdateNotificationPresenter:
    def __init__(self, parent):
        self._parent = parent
        self._bridge = _UpdateSignalBridge()
        self._bridge.updateAvailable.connect(self._show_prompt, Qt.QueuedConnection)
        self._observer = None  # keep alive for the life of the async call

    def check_for_update(self):
        try:
            alg = AlgorithmManager.create("CheckMantidVersion")
            alg.setChild(True)
            alg.setAlgStartupLogging(False)
            self._observer = UpdateNotificationObserver(self._bridge)
            self._observer.setAlg(alg)
            self._observer.observeFinish(alg)
            alg.execute()
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
