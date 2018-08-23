from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets, QtCore, QtGui
from qtpy.QtCore import Signal


class BrowseFileWidgetView(QtWidgets.QWidget):
    # signals for use by parent widgets
    loadingStarted = Signal()
    loadingFinished = Signal()
    dataChanged = Signal()

    def __init__(self, parent=None):
        super(BrowseFileWidgetView, self).__init__(parent)

        self.horizontal_layout = None
        self.browse_button = None
        self.file_path_edit = None
        self.copy_button = None
        self.cancel_button = None

        self.setup_interface_layout()

        self.copy_button.clicked.connect(self.copy_line_edit_to_clipboard)

        self._store_edit_text = False
        self._stored_edit_text = ""
        self._cached_text = ""

        # progress tracking if many files being loaded
        self.progress_bar = None
        self.progress_widget = None

    def setup_interface_layout(self):
        self.setObjectName("BrowseFileWidget")
        self.resize(500, 100)

        self.browse_button = QtWidgets.QPushButton(self)

        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.browse_button.sizePolicy().hasHeightForWidth())

        self.browse_button.setSizePolicy(size_policy)
        self.browse_button.setMinimumSize(QtCore.QSize(100, 50))
        self.browse_button.setObjectName("browseButton")
        self.browse_button.setText("Browse")

        self.file_path_edit = QtWidgets.QLineEdit(self)

        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.file_path_edit.sizePolicy().hasHeightForWidth())

        self.file_path_edit.setSizePolicy(size_policy)
        self.file_path_edit.setMinimumSize(QtCore.QSize(200, 50))
        self.file_path_edit.setToolTip("")
        self.file_path_edit.setObjectName("filePathEdit")

        self.copy_button = QtWidgets.QToolButton(self)
        self.copy_button.setMinimumSize(QtCore.QSize(50, 50))
        self.copy_button.setToolTip("Copy current files to clipboard")
        self.copy_button.setText("...")
        self.copy_button.setIcon(QtGui.QIcon(":/copy.png"))
        self.copy_button.setObjectName("copyButton")

        self.horizontal_layout = QtWidgets.QHBoxLayout(self)
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.browse_button)
        self.horizontal_layout.addWidget(self.file_path_edit)
        self.horizontal_layout.addWidget(self.copy_button)

        self.setLayout(self.horizontal_layout)

    def on_browse_clicked(self, slot):
        self.browse_button.clicked.connect(slot)

    def on_file_edit_changed(self, slot):
        self.file_path_edit.returnPressed.connect(slot)

    def copy_line_edit_to_clipboard(self):
        cb = QtWidgets.QApplication.clipboard()
        cb.clear(mode=cb.Clipboard)
        cb.setText(self.get_file_edit_text(), mode=cb.Clipboard)

    def show_file_browser_and_return_selection(self, file_filter, search_directories, multiple_files=False):
        default_directory = search_directories[0]
        if multiple_files:
            chosen_files, _ = QtWidgets.QFileDialog.getOpenFileNames(self, "Select files", default_directory,
                                                                     file_filter)
            return [str(chosen_file) for chosen_file in chosen_files]
        else:
            chosen_file, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Select file", default_directory,
                                                                   file_filter)
            return [str(chosen_file)]

    def disable_loading(self):
        self.disable_load_buttons()
        self.loadingStarted.emit()

    def enable_loading(self):
        self.enable_load_buttons()
        self.loadingFinished.emit()
        self.dataChanged.emit()

    def notify_loading_started(self):
        self.loadingStarted.emit()

    def notify_loading_finished(self):
        self.loadingFinished.emit()
        self.dataChanged.emit()

    def disable_load_buttons(self):
        self.browse_button.setEnabled(False)
        self.file_path_edit.setEnabled(False)

    def enable_load_buttons(self):
        self.browse_button.setEnabled(True)
        self.file_path_edit.setEnabled(True)

    def get_file_edit_text(self):
        if self._store_edit_text:
            return self._stored_edit_text
        else:
            return self.file_path_edit.text()

    def set_file_edit(self, text, store=False):
        if store:
            self._store_edit_text = True
            self._stored_edit_text = text
            self.file_path_edit.setText("(... more than 10 files, use copy button)")
        else:
            self.file_path_edit.setText(text)
        self._cached_text = self.get_file_edit_text()

    def clear(self):
        self.set_file_edit("")
        self._store_edit_text = False
        self._cached_text = ""

    def reset_edit_to_cached_value(self):
        print("Reset to cache : ", self._cached_text)
        tmp = self._cached_text
        self.set_file_edit(tmp)
        self._cached_text = tmp

    def warning_popup(self, message):
        # TODO : fix this to limit the number of warnings
        QtWidgets.QMessageBox.warning(self, "Error", str(message))

    def show_progress_bar(self, cancel_slot):
        self.progress_bar = QtWidgets.QProgressBar(self)
        self.progress_bar.setMinimumSize(QtCore.QSize(150, 50))

        self.cancel_button = QtWidgets.QToolButton(self)
        self.cancel_button.setMinimumSize(QtCore.QSize(50, 50))
        self.cancel_button.setText("X")
        # TODO : get correct icon
        #self.cancel_button.setIcon(QtGui.QIcon(":/copy.png"))
        self.cancel_button.setObjectName("cancelButton")

        self.cancel_button.clicked.connect(cancel_slot)

        self.horizontal_layout.addWidget(self.cancel_button)
        self.horizontal_layout.addWidget(self.progress_bar)
        self.setLayout(self.horizontal_layout)

    def remove_progress_bar(self):
        if self.progress_bar:
            self.horizontal_layout.removeWidget(self.progress_bar)
            self.progress_bar.deleteLater()
            self.cancel_button.deleteLater()
            self.progress_bar = None
            self.cancel_button = None
            self.setLayout(self.horizontal_layout)

    def set_progress_bar(self, progress):
        if self.progress_bar:
            self.progress_bar.setValue(progress)