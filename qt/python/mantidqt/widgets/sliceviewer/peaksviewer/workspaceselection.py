# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports

# 3rdparty imports
from mantid.api import IPeaksWorkspace
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QTableWidget, QTableWidgetItem, QVBoxLayout


class PeaksWorkspaceSelectorModel(object):
    """Data model for selecting PeaksWorkspaces from an object capable of providing
    a access to the available list of workspaces.
    """

    def __init__(self, workspace_provider, checked_names=None):
        """
        :param workspace_provider: An object allowing access to available workspaces
        :param checked_names: An optional list of str names of workspaces to check by default
        """
        self._workspace_provider = workspace_provider
        self._checked_names = checked_names if checked_names is not None else []

    def names_and_statuses(self):
        """
        :return: a list of 2-tuples where each tuple contains (workspace name:str, checked status :bool)
        """
        ws_provider = self._workspace_provider
        checked_names = self._checked_names
        names = ws_provider.getObjectNames()
        name_status = []
        for name in names:
            try:
                ws = ws_provider[name]
            except KeyError:
                # assume it has been deleted since we asked for the name
                pass
            if isinstance(ws, IPeaksWorkspace):
                name_status.append((name, name in checked_names))

        return name_status


class PeaksWorkspaceSelectorPresenter(object):
    """Present the list of available PeaksWorkspaces to the user and allow them
    to select them.
    """

    def __init__(self, view, model):
        """
        :param view: A view to present the information in the model
        :param model: A model containing the list of PeaksWorkspaces
        """
        self._model = model
        self._view = view

        view.subscribe(self)
        view.set_peaks_workspaces(model.names_and_statuses())

    def select_peaks_workspaces(self):
        """
        :returns: A list of the selected workspaces. Can be empty to indicate no selection
        """
        view = self._view
        result = view.exec_()
        if result == view.Accepted:
            return view.selected_peaks_workspaces()
        else:
            return []


class PeaksWorkspaceSelectorView(QDialog):
    NAME_COLUMN_TITLE = "Workspace"
    NAME_COLUMN_INDEX = 0
    CHECKED_COLUMN_TITLE = "Overlay?"
    CHECKED_COLUMN_INDEX = 1

    def __init__(self, parent=None):
        """
        :param parent: An optional parent
        """
        super(PeaksWorkspaceSelectorView, self).__init__(parent)
        self._presenter = None
        self._setup_ui()

    def selected_peaks_workspaces(self):
        """
        Returns the list of workspace names checked in the widget
        """
        table = self._table
        names = []
        nrows = table.rowCount()
        for row in range(nrows):
            check_item = table.item(row, self.CHECKED_COLUMN_INDEX)
            if check_item.checkState() == Qt.Checked:
                name_item = table.item(row, self.NAME_COLUMN_INDEX)
                names.append(name_item.text())

        return names

    def set_peaks_workspaces(self, names_and_statuses):
        """
        :param names_and_statuses: a list of 2-tuples where each tuple contains (workspace name:str, checked status :bool)
        """
        table = self._table
        table.clearContents()

        for row, (name, checked) in enumerate(names_and_statuses):
            table.insertRow(row)
            name_item = QTableWidgetItem(name)
            name_item.setFlags(Qt.ItemIsEnabled)
            table.setItem(row, self.NAME_COLUMN_INDEX, name_item)
            overlay_item = QTableWidgetItem()
            overlay_item.setFlags(Qt.ItemIsEnabled | Qt.ItemIsUserCheckable)
            overlay_item.setCheckState(Qt.Checked if checked else Qt.Unchecked)
            table.setItem(row, self.CHECKED_COLUMN_INDEX, overlay_item)

    def subscribe(self, presenter):
        """Register an object to listen to events from the GUI
        :param presenter: An object that will receive GUI events
        """
        self._presenter = presenter

    # private
    def _setup_ui(self):
        """Setup the widgets on the screen
        """
        # table
        nrows, ncols = 0, 2
        self._table = QTableWidget(nrows, ncols, self)
        self._table.setHorizontalHeaderLabels([self.NAME_COLUMN_TITLE, self.CHECKED_COLUMN_TITLE])

        # buttons
        self._button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self._button_box.accepted.connect(self.accept)
        self._button_box.rejected.connect(self.reject)

        layout = QVBoxLayout()
        layout.addWidget(self._table)
        layout.addWidget(self._button_box)
        self.setLayout(layout)
