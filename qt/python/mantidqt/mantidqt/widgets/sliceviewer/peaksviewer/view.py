# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# 3rd party imports
from qtpy.QtCore import QSortFilterProxyModel, Qt
from qtpy.QtWidgets import QGroupBox, QVBoxLayout, QWidget, QCheckBox
from mantidqt.widgets.workspacedisplay.table.view import QTableView, TableWorkspaceDisplayView

# local imports
from .representation.painter import MplPainter
from .actions import PeakActionsView

# standard
from typing import Annotated, Optional, TypeAlias

# Forward declarations
PeaksViewerPresenter: TypeAlias = Annotated[type, "PeaksViewerPresenter"]
SliceViewer: TypeAlias = Annotated[type, "SliceViewer"]


class _LessThanOperatorSortFilterModel(QSortFilterProxyModel):
    """Custom QSortFilterProxyModel. Uses __le__ operator
    defined on the data type itself. The base method has limited
    support for some basic types and resorts to string comparisons
    for the rest: https://doc.qt.io/qt-5/qsortfilterproxymodel.html#lessThan
    """

    def lessThan(self, left_index, right_index):
        """Return True if the data at left_index
        is considered less than the data at the right_index.
        """
        left = left_index.model().data(left_index, self.sortRole())
        right = right_index.model().data(right_index, self.sortRole())
        try:
            return left < right
        except TypeError:
            return True


class _PeaksWorkspaceTableView(TableWorkspaceDisplayView):
    """Specialization of a table view to display peaks
    Designed specifically to be used by PeaksViewerView
    """

    def __init__(self, *args, **kwargs):
        self._key_handler = kwargs.pop("key_handler")
        TableWorkspaceDisplayView.__init__(self, *args, **kwargs)
        self.source_model = self.model()
        self.proxy_model = None

    def keyPressEvent(self, event):
        """
        Override base to call handler as part of event
        """
        # bypass immediate base class to get standard table arrow key behaviour
        QTableView.keyPressEvent(self, event)
        self._key_handler._row_selected()

    def enable_sorting(self, sort_role: int):
        """
        Turn on column sorting by clicking headers
        :param: Role defined as source of data for sorting
        """
        self.setSortingEnabled(True)
        self.proxy_model = _LessThanOperatorSortFilterModel()
        self.proxy_model.setSourceModel(self.source_model)
        self.proxy_model.setSortRole(sort_role)
        self.setModel(self.proxy_model)

    def filter_columns(self, concise, unwanted_columns):
        """
        Hide or show columns of the table view
        :param concise: bool for if the table should be in 'concise mode'
        :param unwanted_columns: list of columns to hide / show by their header name
        """
        header_size = self.source_model.columnCount()
        for i in range(header_size):
            header_name = self.source_model.headerData(i, Qt.Horizontal)
            if header_name in unwanted_columns:
                self.setColumnHidden(i, concise)


class PeaksViewerView(QWidget):
    """Displays a table view of the PeaksWorkspace along with controls
    to interact with the peaks.
    """

    TITLE_PREFIX = "Workspace: "

    def __init__(self, painter: MplPainter, sliceinfo_provider: SliceViewer, parent=None):
        """
        :param painter: An object responsible for draw the peaks representations
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super().__init__(parent)
        self._painter: MplPainter = painter
        self._sliceinfo_provider: SliceViewer = sliceinfo_provider
        self._group_box: Optional[QGroupBox] = None
        self._presenter: Optional[PeaksViewerPresenter] = None  # handle to its presenter
        self._table_view: Optional[_PeaksWorkspaceTableView] = None
        self._concise_check_box: Optional[QCheckBox] = None
        self._setup_ui()

    @property
    def presenter(self):
        return self._presenter

    @property
    def painter(self):
        """Return a reference to the painter used in this view"""
        return self._painter

    @property
    def sliceinfo(self):
        """Return information regarding the current slice"""
        return self._sliceinfo_provider.get_sliceinfo()

    @property
    def frame(self):
        return self._sliceinfo_provider.get_frame()

    @property
    def table_view(self):
        return self._table_view

    @property
    def selected_index(self):
        return self._selected_index()

    def clear_table_selection(self):
        self.table_view.clearSelection()

    def set_axes_limits(self, xlim, ylim, auto_transform):
        """
        Set the view limits on the image axes to the given extents
        :param xlim: 2-tuple of (xmin, xmax)
        :param ylim: 2-tuple of (ymin, ymax)
        :param auto_transform: If True, the limits are transformed into the
                               rectilinear frame using the transform provided
                               by the sliceinfo
        """
        self._sliceinfo_provider.set_axes_limits(xlim, ylim, auto_transform)

    def set_peak_color(self, peak_color):
        """
        Set the color of the peak represented in this view
        :param peak_color: A str describing the color of the displayed peak
        """
        self._group_box.setStyleSheet(f"QGroupBox  {{color: {peak_color}}};")

    def set_slicepoint(self, value):
        """
        Set the slice point to the given value
        :param value: Float giving the current slice value
        """
        self._sliceinfo_provider.set_slicepoint(value)

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
        self._table_view = _PeaksWorkspaceTableView(parent=self, key_handler=self)
        self._table_view.setSelectionBehavior(_PeaksWorkspaceTableView.SelectRows)
        self._table_view.setSelectionMode(_PeaksWorkspaceTableView.SingleSelection)
        self._table_view.clicked.connect(self._on_row_clicked)
        self._table_view.verticalHeader().sectionClicked.connect(self._row_selected)
        self._concise_check_box = QCheckBox(text="Concise View", parent=self)
        self._concise_check_box.setChecked(False)
        self._concise_check_box.stateChanged.connect(self._check_box_clicked)

        group_box_layout = QVBoxLayout()
        group_box_layout.addWidget(self._concise_check_box)
        group_box_layout.addWidget(self._table_view)
        self._group_box.setLayout(group_box_layout)
        widget_layout = QVBoxLayout()
        widget_layout.addWidget(self._group_box)
        self.setLayout(widget_layout)

    def _on_row_clicked(self, _):
        """
        When a peak is clicked check if it is already selected and notify that this
        peak has been selected again. Handles the case when selecting the same
        peak needs to reset another view. Care is taken to avoid emitting the peak
        selection notification when peak selection changes as _on_row_selection_changed
        handles this.
        """
        self._row_selected()

    def _row_selected(self):
        """
        Notify that a different peak has been selected. It is assumed only single row selection is allowed
        """
        self._presenter.notify(self._presenter.Event.PeakSelected)

    def _selected_index(self):
        # construction ensures we can only have 0 or 1 items selected
        selected = self.table_view.selectedIndexes()
        if not selected:
            return None

        return self.table_view.proxy_model.mapToSource(selected[0]).row()

    def _check_box_clicked(self):
        """
        Call presenter method on concise view checkbox state change
        """
        self.presenter.concise_checkbox_changes(self._concise_check_box.isChecked())


class PeaksViewerCollectionView(QWidget):
    """Display a collection of PeaksViewerView objects in a scrolling view."""

    def __init__(self, painter: MplPainter, sliceinfo_provider: "SliceViewer", parent=None):
        """
        :param painter: An object responsible for draw the peaks representations
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super(PeaksViewerCollectionView, self).__init__(parent)
        self._painter = painter
        self._sliceinfo_provider = sliceinfo_provider
        self._peak_actions_view = PeakActionsView(parent=self)
        self._peaks_layout: Optional[QVBoxLayout] = None
        self._setup_ui()

    @property
    def peak_actions_view(self) -> PeakActionsView:
        return self._peak_actions_view

    def append_peaksviewer(self, index=-1) -> PeaksViewerView:
        """
        Append a view widget to end of the current list of views
        :param peaks_view: A reference to a single view widget for a PeaksWorkspace
        :param index: the index to insert the PeaksViewerView within the PeaksViewerCollectionView
        """
        child_view = PeaksViewerView(self._painter, self._sliceinfo_provider, parent=self)
        self._peaks_layout.insertWidget(index, child_view)
        return child_view

    def remove_peaksviewer(self, widget: PeaksViewerView):
        """
        Remove a PeaksViewer from the collection
        :param widget: A reference to the PeaksViewerView
        :return: index of removed PeaksViewerView within PeaksViewerCollectionView
        """
        layout = self._peaks_layout
        index = layout.indexOf(widget)
        item = layout.takeAt(index)
        item.widget().deleteLater()
        return index

    def deactivate_zoom_pan(self):
        self._sliceinfo_provider.deactivate_zoom_pan()

    # private api
    def _setup_ui(self):
        """
        Arrange widgets on the window.
        """
        # create vertical layouts for outer widget
        self._outer_layout = QVBoxLayout()  # contains everything
        self._outer_layout.addWidget(self._peak_actions_view)
        self._peaks_layout = QVBoxLayout()  # contains the tables of peaks
        self._outer_layout.addLayout(self._peaks_layout)
        self.setLayout(self._outer_layout)
