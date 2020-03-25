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

    def __init__(self, painter, sliceinfo_provider, parent=None):
        """
        :param painter: An object responsible for draw the peaks representations
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super(PeaksViewerView, self).__init__(parent)
        self._painter = painter
        self._sliceinfo_provider = sliceinfo_provider
        self._group_box = None
        self._presenter = None
        self._table_view = None
        self._setup_ui()

    @property
    def sliceinfo(self):
        """Return information regarding the current slice"""
        return self._sliceinfo_provider.get_sliceinfo()

    @property
    def table_view(self):
        return self._table_view

    @property
    def selected_index(self):
        # construction ensures we can only have 0 or 1 items selected
        selected = self.table_view.selectedItems()
        if not selected:
            return None

        return self.table_view.row(selected[0])

    def clear_peaks(self, peaks):
        """Clear all peaks from display"""
        for peak in peaks:
            peak.remove(self._painter)

    def draw_peaks(self, peaks):
        """
        Draw a single peak using the supplied painter
        :param peaks: An iterable of PeakRepresentations to display
        """
        for peak in peaks:
            peak.draw(self._painter)

    def snap_to(self, peak):
        """
        Set the peak center as the center of the display
        :param peak: A list
        """
        peak.snap_to(self._painter)
        self._sliceinfo_provider.set_slicevalue(peak.z)

    def update_peaks(self, peaks):
        """
        Update existing peak represetations
        :param peaks: An iterable of PeakRepresentations to display
        """
        for peak in peaks:
            peak.repaint(self._painter)

    def set_title(self, name):
        """
        :param name: Set the name label for the workspace
        """
        self._group_box.setTitle(self.TITLE_PREFIX + name)

    def subscribe(self, presenter):
        """
        :param presenter: An object to handle GUI events.
        """
        self._presenter = presenter
        self._table_view.subscribe(presenter)

    # private api
    def _setup_ui(self):
        """
        Arrange the widgets on the window
        """
        self._group_box = QGroupBox(self)
        self._group_box.setContentsMargins(0, 0, 0, 0)
        self._table_view = TableWorkspaceDisplayView(parent=self)
        self._table_view.setSelectionBehavior(TableWorkspaceDisplayView.SelectRows)
        self._table_view.itemSelectionChanged.connect(self._on_row_selection_changed)

        group_box_layout = QVBoxLayout()
        group_box_layout.addWidget(self._table_view)
        self._group_box.setLayout(group_box_layout)
        widget_layout = QVBoxLayout()
        widget_layout.addWidget(self._group_box)
        self.setLayout(widget_layout)

    def _on_row_selection_changed(self):
        """
        Slot to handle row selection changes. It is assumed only single row
        selection is allowed
        """
        self._presenter.notify(self._presenter.Event.PeakSelectionChanged)


class PeaksViewerCollectionView(QWidget):
    """Display a collection of PeaksViewerView objects in a scrolling view.
    """

    def __init__(self, painter, sliceinfo_provider, parent=None):
        """
        :param painter: An object responsible for draw the peaks representations
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super(PeaksViewerCollectionView, self).__init__(parent)
        self._painter = painter
        self._sliceinfo_provider = sliceinfo_provider
        self._setup_ui()

    def append_peaksviewer(self):
        """
        Append a view widget to end of the current list of views
        :param peaks_view: A reference to a single view widget for a PeaksWorkspace
        """
        child_view = PeaksViewerView(self._painter, self._sliceinfo_provider, parent=self)
        self._peaks_layout.addWidget(child_view)
        return child_view

    def remove_peaksviewer(self, widget):
        """
        Remove a PeaksViewer from the collection
        :param widget: A reference to the PeaksViewerView
        """
        layout = self._peaks_layout
        item = layout.takeAt(layout.indexOf(widget))
        item.widget().deleteLater()

    # private api
    def _setup_ui(self):
        """
        Arrange widgets on the window.
        """
        # create vertical layouts for outer widget
        outer_layout = QVBoxLayout()
        self.setLayout(outer_layout)
        self._peaks_layout = outer_layout
