from __future__ import (absolute_import, division, print_function)

from qtpy import QtCore, QtGui, QtWidgets
from qtpy.QtCore import Signal

from Muon.GUI.Common.message_box import *


class BrowseFileWidgetView(QtWidgets.QWidget):
    # signals for use by parent widgets
    loadingStarted = Signal()
    loadingFinished = Signal()
    dataChanged = Signal()

    def __init__(self, parent=None):
        super(BrowseFileWidgetView, self).__init__(parent)
        self.setupUi(self)

        self.copyButton.clicked.connect(self.copy_line_edit_to_clipboard)

        self._store_edit_text = False
        self._stored_edit_text = ""
        self._cached_text = ""

        # To ensure at most one warning window open

        self._warning_window = None


    def setupUi(self, BrowseFileWidget):
        BrowseFileWidget.setObjectName("BrowseFileWidget")
        BrowseFileWidget.resize(462, 45)

        self.gridLayout = QtWidgets.QGridLayout(BrowseFileWidget)
        self.gridLayout.setObjectName("gridLayout")
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.browseButton = QtWidgets.QPushButton(BrowseFileWidget)

        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.browseButton.sizePolicy().hasHeightForWidth())

        self.browseButton.setSizePolicy(sizePolicy)
        self.browseButton.setMinimumSize(QtCore.QSize(100, 25))
        self.browseButton.setObjectName("browseButton")
        self.browseButton.setText("Browse")
        self.horizontalLayout.addWidget(self.browseButton)
        self.filePathEdit = QtWidgets.QLineEdit(BrowseFileWidget)

        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.filePathEdit.sizePolicy().hasHeightForWidth())

        self.filePathEdit.setSizePolicy(sizePolicy)
        self.filePathEdit.setMinimumSize(QtCore.QSize(0, 25))
        self.filePathEdit.setToolTip("")
        self.filePathEdit.setObjectName("filePathEdit")

        self.horizontalLayout.addWidget(self.filePathEdit)
        self.copyButton = QtWidgets.QToolButton(BrowseFileWidget)
        self.copyButton.setMinimumSize(QtCore.QSize(25, 25))
        self.copyButton.setToolTip("Copy current files to clipboard")
        self.copyButton.setText("...")
        self.copyButton.setIcon(QtGui.QIcon(":/copy.png"))
        self.copyButton.setObjectName("copyButton")

        self.horizontalLayout.addWidget(self.copyButton)
        self.gridLayout.addLayout(self.horizontalLayout, 0, 0, 1, 1)
        # QtCore.QMetaObject.connectSlotsByName(BrowseFileWidget)

    def on_browse_clicked(self, slot):
        self.browseButton.clicked.connect(slot)

    def on_file_edit_changed(self, slot):
        self.filePathEdit.returnPressed.connect(slot)

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
        self.browseButton.setEnabled(False)
        self.filePathEdit.setEnabled(False)

    def enable_load_buttons(self):
        self.browseButton.setEnabled(True)
        self.filePathEdit.setEnabled(True)

    def get_file_edit_text(self):
        if self._store_edit_text:
            return self._stored_edit_text
        else:
            return self.filePathEdit.text()

    def set_file_edit(self, text, store=False):
        if store:
            self._store_edit_text = True
            self._stored_edit_text = text
            self.filePathEdit.setText("(... more than 10 files, use copy button)")
        else:
            self.filePathEdit.setText(text)
        self._cached_text = self.get_file_edit_text()

    def clear(self):
        self.set_file_edit("")
        self._store_edit_text = False
        self._cached_text = ""

    def reset_edit_to_cached_value(self):
        print("Reset to cache : ",self._cached_text )
        tmp = self._cached_text
        self.set_file_edit(tmp)
        self._cached_text = tmp

    def warning_popup(self, message):
        mutex = QtCore.QMutex()
        mutex.lock()
        print("WARNING POPUP")
        try:
            self._warning_window.done(1)
        except:
            pass
        self._warning_window = None
        self._warning_window = QtWidgets.QMessageBox.warning(self, "Error", str(message))

        mutex.unlock()