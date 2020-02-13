# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# 3rd party imports
from mantid.py3compat.enum import Enum

# local imports
from mantidqt.widgets.workspacedisplay.table.presenter \
    import TableWorkspaceDataPresenter


class PeaksViewerPresenter(object):
    """Controls a PeaksViewerView with a given model to display
    the peaks table and interaction controls for single workspace.
    """
    class Event(Enum):
        PeaksListChanged = 1
        OverlayPeaks = 2
        SlicePointChanged = 3

    def __init__(self, model, view):
        """
        Constructs the view for the given PeaksWorkspace
        :param model: A handle to the view-model wrapper for PeaksWorkspace to be displayed
        :param view: A view object with a subscribe method to register this presenter
                     as a listener for view events
        """
        super(PeaksViewerPresenter, self).__init__()
        self._model = model
        self._raise_error_if_workspace_incompatible(model.peaks_workspace)
        self._peaks_table_presenter = \
            TableWorkspaceDataPresenter(model, view.table_view)

        self._view = view
        view.subscribe(self)
        view.set_title(model.peaks_workspace.name())
        self.notify(PeaksViewerPresenter.Event.PeaksListChanged)

    @property
    def model(self):
        return self._model

    def notify(self, event):
        """
        Notification of an event that the presenter should react to
        :param event:
        """
        PresenterEvent = PeaksViewerPresenter.Event
        if event == PresenterEvent.SlicePointChanged:
            self._update_peaks()
        elif event == PresenterEvent.OverlayPeaks:
            self._overlay_peaks()
        elif event == PresenterEvent.PeaksListChanged:
            self._peaks_table_presenter.refresh()
        else:
            from mantid.kernel import logger
            logger.warning("PeaksViewer: Unknown event detected: {}".format(event))

    def _clear_peaks(self):
        """Clear all peaks from this view"""
        self._view.clear_peaks(self.model.take_peak_representations())

    def _overlay_peaks(self):
        """
        Respond to request to overlay PeaksWorkspace.
          - Query current slicing information
          - Compute peaks representations
          - Draw overlays.
        """
        self._clear_peaks()
        peak_representations = self.model.compute_peak_representations(self._view.sliceinfo)
        self._view.draw_peaks(peak_representations)

    def _update_peaks(self):
        """
        Respond to a change that only requires updating PeakRepresentation properties such
        as transparency
        """
        self._view.update_peaks(self.model.update_peak_representations(self._view.sliceinfo))

    # private api
    @staticmethod
    def _raise_error_if_workspace_incompatible(ws):
        """
        :param ws: A reference to an object to check for compatability
        """
        if not hasattr(ws, "getNumberPeaks"):
            raise ValueError("Expected a PeaksWorkspace. Found {}.".format(type(ws)))


class PeaksViewerCollectionPresenter(object):
    """Controls a widget comprising of multiple PeasViewerViews to display and
    interact with multiple PeaksWorkspaces"""
    def __init__(self, view):
        """
        :param view: View displaying the model information
        """
        self._view = view
        self._child_presenters = []

    @property
    def view(self):
        return self._view

    def append_peaksworkspace(self, model):
        """
        Create and append a view for the given workspace
        :param model: A PeakWorkspace model object.
        :returns: The child presenter
        """
        presenter = PeaksViewerPresenter(model, self._view.append_peaksviewer())
        self._child_presenters.append(presenter)
        return presenter

    def notify(self, event):
        """Dispatch notification to all subpresenters"""
        for presenter in self._child_presenters:
            presenter.notify(event)
