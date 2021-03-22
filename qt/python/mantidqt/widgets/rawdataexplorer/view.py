# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from mantidqt.utils.qt import load_ui
from qtpy.QtWidgets import QFileDialog, QFileSystemModel, QWidget, QTreeWidget
# from mantidqt.widgets.previewSelectorWidget import PreviewSelectorWidget
from qtpy.QtCore import *

from mantidqt.widgets.instrumentview.api import get_instrumentview

class RawDataExplorerView(QWidget):
    """The view for the raw data explorer widget."""

    file_tree_path_changed = Signal(str)

    instrument_changed = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

        self.ui = load_ui(__file__, 'rawdataexplorer.ui', baseinstance=self)

        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setup()

    def closeEvent(self, event):
        self.deleteLater()
        super(RawDataExplorerView, self).closeEvent(event)

    def setup(self):
        self.browse.clicked.connect(self.show_directory_manager)

        # self.instrumentSelector.instrumentSelectionChanged.connect(self.instrument_changed.emit)
        # self.instrumentSelector.instrumentSelectionChanged.connect(self.instrumentChanged.emit)

    def set_file_model(self, file_model):
        """
        Set the QFileSystemModel behind the tree view
        @param file_model : the file model, a QFileSystemModel object
        """
        self.fileTree.setModel(file_model)

    def show_directory_manager(self):
        """
        Open a new directory manager window so the user can select a directory.
        """
        file_tree_path = QFileDialog().getExistingDirectory(parent=self,
                                                            caption="Select a directory",
                                                            directory="/home",
                                                            options=QFileDialog.DontUseNativeDialog |
                                                                    QFileDialog.ShowDirsOnly)
        self.file_tree_path_changed.emit(file_tree_path)

    def show_ws(self, ws_to_show):
        iview = get_instrumentview(ws_to_show)
        iview.show_view()
