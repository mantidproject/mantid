# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# 3rd party imports
from qtpy.QtWidgets import QGroupBox, QVBoxLayout, QWidget

# local imports
from mantidqt.widgets.workspacedisplay.table.view import TableWorkspaceDisplayView


class PeaksViewerView(QWidget):
    """Displays a table view of the PeaksWorkspace along with controls
    to interact with the peaks.
    """
    TITLE_PREFIX = "Workspace: "

    def __init__(self, parent=None):
        """
        :param parent: An optional parent widget
        """
        super(PeaksViewerView, self).__init__(parent)
        self._group_box = None
        self._presenter = None
        self._table_view = None
        self._setup_ui()

    @property
    def table_view(self):
        return self._table_view

    def subscribe(self, presenter):
        """
        :param presenter: An object to handle GUI events.
        """
        self._presenter = presenter
        self._table_view.subscribe(presenter)

    def set_title(self, name):
        """
        :param name: Set the name label for the workspace
        """
        self._group_box.setTitle(self.TITLE_PREFIX + name)

    # private api
    def _setup_ui(self):
        """
        Arrange the widgets on the window
        """
        self._group_box = QGroupBox(self)
        self._group_box.setContentsMargins(0, 0, 0, 0)
        self._table_view = TableWorkspaceDisplayView(parent=self)
        self._table_view.setSelectionBehavior(TableWorkspaceDisplayView.SelectRows)

        group_box_layout = QVBoxLayout()
        group_box_layout.addWidget(self._table_view)
        self._group_box.setLayout(group_box_layout)
        widget_layout = QVBoxLayout()
        widget_layout.addWidget(self._group_box)
        self.setLayout(widget_layout)


class PeaksViewerCollectionView(QWidget):
    """Display a collection of PeaksViewerView objects in a scrolling view.
    """

    def __init__(self, parent=None):
        """
        :param parent: An optional parent widget
        """
        super(PeaksViewerCollectionView, self).__init__(parent)
        self._setup_ui()

    def append_peaksviewer(self):
        """
        Append a view widget to end of the current list of views
        :param peaks_view: A reference to a single view widget for a PeaksWorkspace
        """
        child_view = PeaksViewerView(self)
        self._peaks_layout.addWidget(child_view)
        return child_view

    # private api
    def _setup_ui(self):
        """
        Arrange widgets on the window.
        """
        # create vertical layouts for outer widget
        outer_layout = QVBoxLayout()
        self.setLayout(outer_layout)
        self._peaks_layout = outer_layout