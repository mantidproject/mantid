# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTreeView, QFileSystemModel, QAbstractItemView, QHeaderView
from qtpy.QtCore import *


class RawDataExplorerFileTree(QTreeView):
    """
    List of filters for the file system tree widget.
    """
    _FILE_SYSTEM_FILTERS = ["*.nxs"]

    """
    Index of the column corresponding to the files
    """
    _FILES_COLUMN = 0

    """
    Signal emitted when the index currently selected is changed.
    The arg is the QModelIndex of the new index.
    """
    sig_new_current = Signal(QModelIndex)

    """
    Signal emitted when the user starts or stops accumulating workspaces.
    The arg is whether or not the user is now accumulating.
    """
    sig_accumulate_changed = Signal(bool)

    def __init__(self, parent=None):
        super(RawDataExplorerFileTree, self).__init__(parent)

        self.is_ctrl_being_pressed = False  # indicates if the control key is being held
        self._ignore_next_focus_out = False  # used for focus management when opening a preview
        self._clearing_selection = False  # used when the selection is being cleared, to ignore changes

        file_model = QFileSystemModel()
        file_model.setNameFilters(self._FILE_SYSTEM_FILTERS)
        file_model.setNameFilterDisables(0)
        file_model.setRootPath("/")
        self.setModel(file_model)
        self.header().hideSection(2)
        self.header().setSectionResizeMode(0, QHeaderView.Stretch)
        self.header().setSectionResizeMode(1, QHeaderView.ResizeToContents)
        self.header().setSectionResizeMode(3, QHeaderView.ResizeToContents)
        self.header().setStretchLastSection(False)

        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

    def currentChanged(self, current_index, previous_index):
        if current_index.column() != self._FILES_COLUMN:
            current_index = current_index.sibling(current_index.row(), self._FILES_COLUMN)

        if not self.is_ctrl_being_pressed:
            self.sig_accumulate_changed.emit(False)

        self.sig_new_current.emit(current_index)
        super(RawDataExplorerFileTree, self).currentChanged(current_index, previous_index)

    def selectionChanged(self, selected, deselected):
        super(RawDataExplorerFileTree, self).selectionChanged(selected, deselected)
        if self._clearing_selection:
            self._clearing_selection = False
            return

        if len(selected.indexes()) == 0 and len(deselected.indexes()) != 0:
            # checking if something was deselected

            first_row = deselected.indexes()[0].row()
            # if multiple lines have been deselected, then we are clearing the widget, which is ok
            for index in deselected.indexes():
                if index.row() != first_row:
                    return

            # but it is not allowed for the user to deselect a line, so we select it back
            for index in deselected.indexes():
                self.selectionModel().select(index, QItemSelectionModel.Select)

    def clear_selection(self):
        """
        Clear all selected indices and reset the index
        """
        # set this flag so selectionChanged ignores the changes
        self._clearing_selection = True

        selection_model = self.selectionModel()
        selection_model.clearSelection()
        selection_model.clearCurrentIndex()

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Control:
            self.is_ctrl_being_pressed = True
            self.sig_accumulate_changed.emit(True)
        else:
            super(RawDataExplorerFileTree, self).keyPressEvent(event)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key_Control:
            self.is_ctrl_being_pressed = False
        else:
            super(RawDataExplorerFileTree, self).keyReleaseEvent(event)

    def focusOutEvent(self, event):
        """
        Slot triggered by focusing out of the explorer. Overrode to take care of the case when the widget is updated
        and steals the focus.
        @param event: the event that caused the focusing out
        """
        # if the event is caused by creating a new window, that means the focus is being taken away by our new widget,
        # and since we don't want that as it makes it impossible to navigate with the keyboard, we actually reverse it
        # and give the focus back to the main window and the raw data explorer
        if event.reason() == Qt.ActiveWindowFocusReason and self._ignore_next_focus_out:
            self.set_ignore_next_focus_out(False)
            self.activateWindow()
            self.setFocus()
            return
        super(RawDataExplorerFileTree, self).focusOutEvent(event)

    def set_ignore_next_focus_out(self, new_status):
        """
        Setter for the bool ignore_next_focus_out
        @param new_status: a boolean, the new value of the flag
        """
        self._ignore_next_focus_out = new_status
