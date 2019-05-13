# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets
from qtpy.QtCore import Signal
import Muon.GUI.Common.message_box as message_box


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

        self.setup_interface_layout()

        self._store_edit_text = False
        self._stored_edit_text = ""
        self._cached_text = ""

        self.set_file_edit("No data loaded", False)

        self.file_path_edit.setReadOnly(True)

    def setup_interface_layout(self):
        self.setObjectName("BrowseFileWidget")
        self.resize(500, 100)

        self.browse_button = QtWidgets.QPushButton(self)

        size_policy = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Fixed,
            QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(
            self.browse_button.sizePolicy(
            ).hasHeightForWidth(
            ))

        self.browse_button.setSizePolicy(size_policy)
        self.browse_button.setObjectName("browseButton")
        self.browse_button.setText("Browse")

        self.file_path_edit = QtWidgets.QLineEdit(self)

        size_policy = QtWidgets.QSizePolicy(
            QtWidgets.QSizePolicy.Minimum,
            QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(
            self.file_path_edit.sizePolicy(
            ).hasHeightForWidth(
            ))

        self.file_path_edit.setSizePolicy(size_policy)
        self.file_path_edit.setToolTip("")
        self.file_path_edit.setObjectName("filePathEdit")

        self.setStyleSheet("QLineEdit {background: #d7d6d5}")

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.browse_button)
        self.horizontal_layout.addWidget(self.file_path_edit)

        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)
        if not isinstance(self.horizontal_layout, QtWidgets.QHBoxLayout):
            self.horizontal_layout.setMargin(0)

        self.setLayout(self.horizontal_layout)

    def getLayout(self):
        return self.horizontal_layout

    def on_browse_clicked(self, slot):
        self.browse_button.clicked.connect(slot)

    def on_file_edit_changed(self, slot):
        self.file_path_edit.returnPressed.connect(slot)

    def show_file_browser_and_return_selection(
           self, file_filter, search_directories, multiple_files=False):
        default_directory = search_directories[0]
        if multiple_files:
            chosen_files = QtWidgets.QFileDialog.getOpenFileNames(
                self, "Select files", default_directory, file_filter)
            return [str(chosen_file) for chosen_file in chosen_files]
        else:
            chosen_file = QtWidgets.QFileDialog.getOpenFileName(
                self, "Select file", default_directory, file_filter)
            return [str(chosen_file)]

    def disable_loading(self):
        self.disable_load_buttons()
        self.loadingStarted.emit()

    def enable_loading(self):
        self.enable_load_buttons()
        self.loadingFinished.emit()

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
            return str(self._stored_edit_text)
        else:
            return str(self.file_path_edit.text())

    def set_file_edit(self, text, store=False):
        if store:
            self._store_edit_text = True
            self._stored_edit_text = text
            self.file_path_edit.setText(
                "(... more than 10 files, use right-click -> copy)")
        else:
            self.file_path_edit.setText(text)
        self._cached_text = self.get_file_edit_text()

    def clear(self):
        self.set_file_edit("No data loaded")
        self._store_edit_text = False
        self._cached_text = "No data loaded"

    def reset_edit_to_cached_value(self):
        tmp = self._cached_text
        self.set_file_edit(tmp)
        self._cached_text = tmp

    def warning_popup(self, message):
        message_box.warning(str(message))
