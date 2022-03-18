# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports

# 3rd party imports

import mantid.api
from qtpy.QtCore import Qt, Signal
from qtpy.QtWidgets import (QHBoxLayout, QSplitter, QWidget)

# local imports
from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView
from mantidqt.widgets.sliceviewer.peaksviewer.workspaceselection import \
    (PeaksWorkspaceSelectorModel, PeaksWorkspaceSelectorPresenter,
     PeaksWorkspaceSelectorView)
from mantidqt.widgets.sliceviewer.peaksviewer.view import PeaksViewerCollectionView
from mantidqt.widgets.sliceviewer.peaksviewer.representation.painter import MplPainter
from mantidqt.widgets.sliceviewer.cutviewer.view import CutViewerView

# Constants
from mantidqt.widgets.observers.observing_view import ObservingView


class SliceViewerView(QWidget, ObservingView):
    """Combines the data view for the slice viewer with the optional peaks viewer."""
    close_signal = Signal()
    rename_signal = Signal(str)

    def __init__(self, presenter, dims_info, can_normalise, parent=None, window_flags=Qt.Window, conf=None):
        super().__init__(parent)

        self.presenter = presenter

        self.setWindowFlags(window_flags)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self._splitter = QSplitter(self)
        self._data_view = SliceViewerDataView(presenter, dims_info, can_normalise, self, conf)
        self._splitter.addWidget(self._data_view)
        #  peaks viewer off by default
        self._peaks_view = None
        self._non_axis_aligned_cut_view = None

        # config the splitter appearance
        splitterStyleStr = """QSplitter::handle{
            border: 1px dotted gray;
            min-height: 10px;
            max-height: 20px;
            }"""
        self._splitter.setStyleSheet(splitterStyleStr)
        self._splitter.setHandleWidth(1)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._splitter)
        self.setLayout(layout)

        # connect up additional peaks signals
        self.data_view.mpl_toolbar.peaksOverlayClicked.connect(self.peaks_overlay_clicked)
        self.data_view.mpl_toolbar.nonAlignedCutsClicked.connect(self.non_axis_aligned_cuts_clicked)
        self.close_signal.connect(self._run_close)
        self.rename_signal.connect(self._on_rename)

    @property
    def data_view(self):
        return self._data_view

    @property
    def dimensions(self):
        return self._data_view.dimensions

    @property
    def peaks_view(self) -> PeaksViewerCollectionView:
        """Lazily instantiates PeaksViewer and returns it"""
        if self._peaks_view is None:
            self._peaks_view = PeaksViewerCollectionView(MplPainter(self.data_view), self.presenter)
            self._splitter.addWidget(self._peaks_view)

        return self._peaks_view

    @property
    def non_axis_aligned_cut_view(self):
        if self._non_axis_aligned_cut_view is None:
            self._non_axis_aligned_cut_view = CutViewerView(MplPainter(self.data_view), self.presenter)
            self._splitter.addWidget(self._non_axis_aligned_cut_view)
        return self._non_axis_aligned_cut_view

    def peaks_overlay_clicked(self):
        """Peaks overlay button has been toggled
        """
        self.presenter.overlay_peaks_workspaces()

    def non_axis_aligned_cuts_clicked(self):
        self.presenter.non_axis_aligned_cut()

    def query_peaks_to_overlay(self, current_overlayed_names):
        """Display a dialog to the user to ask which peaks to overlay
        :param current_overlayed_names: A list of names that are currently overlayed
        :returns: A list of workspace names to overlay on the display
        """
        model = PeaksWorkspaceSelectorModel(mantid.api.AnalysisDataService.Instance(),
                                            checked_names=current_overlayed_names)
        view = PeaksWorkspaceSelectorView(self)
        presenter = PeaksWorkspaceSelectorPresenter(view, model)
        return presenter.select_peaks_workspaces()

    def set_peaks_viewer_visible(self, on):
        """
        Set the visibility of the PeaksViewer.
        :param on: If True make the view visible, else make it invisible
        :return: The PeaksViewerCollectionView
        """
        self.peaks_view.set_visible(on)

    def close(self):
        self.presenter.notify_close()
        super().close()

    def _run_close(self):
        # handles the signal emitted from ObservingView.emit_close
        self.close()

    def _on_rename(self, new_title: str):
        self.setWindowTitle(new_title)
