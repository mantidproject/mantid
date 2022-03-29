from qtpy.QtCore import QFileSystemWatcher, QTimer, Signal
from qtpy.QtWidgets import QWidget


class DNSFileSelectorWatcher(QWidget):
    """watches a directory for new files"""

    def __init__(self, parent):
        super().__init__(parent)
        self.fs_watcher = QFileSystemWatcher()
        self.autoload_timer = QTimer()
        self.autoload_timer.timeout.connect(self._autoload_files_changed)

    sig_files_changed = Signal()

    def _auto_load_triggered_files_changed(self):
        self.sig_files_changed.emit()

    def _start_autoload_timer(self):
        """adds a 5 seconds delay to the file watcher, so glob is not triggered
            for every single file """
        if not self.autoload_timer.isActive():
            self.autoload_timer.start(5000)

    def _autoload_files_changed(self):
        self.autoload_timer.stop()
        self._auto_load_triggered_files_changed()

    def start_watcher(self, datadir=None):
        """watches datadir for change of files starts autoload timer
           if files are changed"""
        self.fs_watcher.addPath(datadir)
        self.fs_watcher.directoryChanged.connect(self._start_autoload_timer)

    def stop_watcher(self):
        self.fs_watcher.removePaths(self.fs_watcher.directories())
        self.fs_watcher.directoryChanged.disconnect()
