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
from mantidqt.widgets.workspacedisplay.table.model \
    import TableWorkspaceDisplayModel
from mantidqt.widgets.workspacedisplay.table.presenter \
    import TableWorkspaceDataPresenter
from .representation import create_peakrepresentation


class PeaksViewerPresenter(object):
    """Controls a PeaksViewerView with a given model to display
    the peaks table and interaction controls for single workspace.
    """

    class Event(Enum):
        PeaksListChanged = 1

    def __init__(self, model, view):
        """
        Constructs the view for the given PeaksWorkspace
        :param model: A handle to the view-model wrapper for PeaksWorkspace to be displayed
        :param view: A view object with a subscribe method to register this presenter
                     as a listener for view events
        """
        super(PeaksViewerPresenter, self).__init__()
        self._model = model
        peaks_ws = model.peaks_workspace
        self._raise_error_if_workspace_incompatible(peaks_ws)
        self._peaks_table_presenter = \
            TableWorkspaceDataPresenter(TableWorkspaceDisplayModel(peaks_ws),
                                        view.table_view)

        view.subscribe(self)
        view.set_title(peaks_ws.name())
        self.notify(PeaksViewerPresenter.Event.PeaksListChanged)

    def notify(self, event):
        """
        Notification of an event that the presenter should react to
        :param event:
        """
        if event == PeaksViewerPresenter.Event.PeaksListChanged:
            self._peaks_table_presenter.refresh()

    def peaks_info(self):
        """
        Returns a list of PeakRepresentation objects describing the peaks in the model
        """
        info = []
        for peak in self._peaks_table_presenter.model.ws:
            info.append(create_peakrepresentation(peak, self._model.marker_color))

        return info

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
    def __init__(self, models, view):
        """
        :param models: An iterable of PeakWorkspace model objects.
        :param view: View displaying the model information
        """
        self._view = view
        self._child_presenters = []
        for model in models:
            self.append_peaksworkspace(model)

    @property
    def view(self):
        return self._view

    def append_peaksworkspace(self, model):
        """
        Create and append a view for the given workspace
        :param model: A PeakWorkspace model object.
        """
        self._child_presenters.append(PeaksViewerPresenter(model, self._view.append_peaksviewer()))

    def peaks_info(self):
        """
        Returns the position of all of the peaks along with an alpha value
        """
        peaks_info = []
        for child in self._child_presenters:
            peaks_info.extend(child.peaks_info())

        return peaks_info
