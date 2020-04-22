# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
from enum import Enum

# local imports
from mantidqt.widgets.workspacedisplay.table.presenter \
    import TableWorkspaceDataPresenter
from .model import create_peaksviewermodel


class PeaksViewerPresenter(object):
    """Controls a PeaksViewerView with a given model to display
    the peaks table and interaction controls for single workspace.
    """

    class Event(Enum):
        PeaksListChanged = 1
        OverlayPeaks = 2
        SlicePointChanged = 3
        ClearPeaks = 4
        PeakSelected = 5

    def __init__(self, model, view):
        """
        Constructs the view for the given PeaksWorkspace
        :param model: A handle to the view-model wrapper for PeaksWorkspace to be displayed
        :param view: A view object with a subscribe method to register this presenter
                     as a listener for view events
        """
        super().__init__()
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

    @property
    def view(self):
        return self._view

    def notify(self, event):
        """
        Notification of an event that the presenter should react to
        :param event:
        """
        PresenterEvent = PeaksViewerPresenter.Event
        if event == PresenterEvent.SlicePointChanged or event == PresenterEvent.OverlayPeaks:
            self._overlay_peaks()
        elif event == PresenterEvent.PeakSelected:
            self._peak_selected()
        elif event == PresenterEvent.PeaksListChanged:
            self._peaks_table_presenter.refresh()
        elif event == PresenterEvent.ClearPeaks:
            self._clear_peaks()
        else:
            from mantid.kernel import logger
            logger.warning("PeaksViewer: Unknown event detected: {}".format(event))

    def _clear_peaks(self):
        """Clear all peaks from this view"""
        self.model.clear_peak_representations()

    def _overlay_peaks(self):
        """
        Respond to request to overlay PeaksWorkspace.
          - Query current slicing information
          - Compute peaks representations
          - Draw overlays.
        """
        self._clear_peaks()
        self.model.draw_peaks(self._view.sliceinfo, self._view.painter)

    def _peak_selected(self):
        """
        Respond to the selection change of a peak in the list
        """
        selected_index = self._view.selected_index
        if selected_index is None:
            return

        self._view.set_slicepoint(self.model.slice_center(selected_index, self._view.sliceinfo))
        self.model.zoom_to(selected_index)

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

    def overlay_peaksworkspaces(self, names_to_overlay):
        """
        :param names_to_overlay: The list of names to overlay
        """
        # The final outcome should be the set of names in names_to_overlay
        # being what is displayed. If anything is currently displayed that is
        # not in names_to_overlay then it will be removed from display
        names_already_overlayed = self.workspace_names()

        # first calculate what is to be removed. make a copy to avoid mutation while iterating
        names_to_overlay_final = names_to_overlay[:]
        for name in names_to_overlay:
            if name in names_already_overlayed:
                names_already_overlayed.remove(name)
                names_to_overlay_final.remove(name)
                continue

        # anything left in names_already overlayed then needs to be removed
        if names_already_overlayed:
            for name in names_already_overlayed:
                self.remove_peaksworkspace(name)

        for name in names_to_overlay_final:
            self.append_peaksworkspace(create_peaksviewermodel(name))

        self.notify(PeaksViewerPresenter.Event.OverlayPeaks)

    def remove_peaksworkspace(self, name):
        """
        Remove the named workspace from display. No op if no workspace can be found with that name
        :param name: The name of a workspace
        """
        child_presenters = self._child_presenters
        presenter_to_remove = None
        for child in child_presenters:
            if child.model.peaks_workspace.name() == name:
                presenter_to_remove = child
                child.notify(PeaksViewerPresenter.Event.ClearPeaks)
                self._view.remove_peaksviewer(child.view)

        child_presenters.remove(presenter_to_remove)

    def workspace_names(self):
        """
        :return: A list of workspace names for each PeaksWorkspace displayed
        """
        names = []
        for presenter in self._child_presenters:
            names.append(presenter.model.peaks_workspace.name())

        return names

    def notify(self, event):
        """Dispatch notification to all subpresenters"""
        for presenter in self._child_presenters:
            presenter.notify(event)
