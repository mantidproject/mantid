from qtpy.QtCore import QFileSystemWatcher, QTimer, Signal
from qtpy.QtWidgets import QWidget


class DNSFileSelectorWatcher(QWidget):
    """
    Watches a directory for new files.
    """
    def __init__(self, parent):
        super().__init__(parent)
        self.fs_watcher = QFileSystemWatcher()
        self.autoload_timer = QTimer()
        self.autoload_timer.timeout.connect(self._autoload_files_changed)

    sig_files_changed = Signal()

    def _auto_load_triggered_files_changed(self):
        self.sig_files_changed.emit()

    def _start_autoload_timer(self):
        """
        Adds a 5 seconds delay to the file watcher, so glob is not triggered
        for every single file.
        """
        if not self.autoload_timer.isActive():
            self.autoload_timer.start(5000)

    def _autoload_files_changed(self):
        self.autoload_timer.stop()
        self._auto_load_triggered_files_changed()

    def start_watcher(self, data_dir=None):
        """
        Watches data_dir for change of files starts autoload timer
        if files are changed.
        """
        if data_dir:
            self.fs_watcher.addPath(data_dir)
        self.fs_watcher.directoryChanged.connect(self._start_autoload_timer)

    def stop_watcher(self):
        data_dir = self.fs_watcher.directories()
        if data_dir:
            self.fs_watcher.removePaths(data_dir)
        self.fs_watcher.directoryChanged.disconnect()
