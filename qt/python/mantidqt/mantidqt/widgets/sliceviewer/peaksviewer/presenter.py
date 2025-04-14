# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# local imports
from .model import create_peaksviewermodel, PeaksViewerModel
from .view import PeaksViewerView, PeaksViewerCollectionView
from mantidqt.widgets.sliceviewer.models.adsobsever import SliceViewerADSObserver

# 3rd party
from mantid.kernel import logger
from mantidqt.widgets.workspacedisplay.table.presenter_standard import TableWorkspaceDataPresenterStandard, create_table_item

# standard
from enum import Enum
from typing import List, Union


class PeaksWorkspaceDataPresenter(TableWorkspaceDataPresenterStandard):
    """Override create_item method to format table columns more
    appropriately
    """

    # Format specifier for floats in the table
    FLOAT_FORMAT_STR = "{:.5f}"
    # Qt model classes can store data against different roles.
    # Defines a custom role to be used for sorting with QSortFilterProxy.
    # See https://doc.qt.io/qt-5/qsortfilterproxymodel.html#sortRole-prop
    DATA_SORT_ROLE = 2001
    HIDDEN_COLUMNS = [
        "RunNumber",
        "DetID",
        "Wavelength",
        "Energy",
        "TOF",
        "DSpacing",
        "BinCount",
        "Row",
        "Col",
        "QLab",
        "QSample",
        "TBar",
        "IntHKL",
        "IntMNP",
    ]

    def create_item(self, data, _):
        """Create a table item to display the data. The data is always readonly
        here.
        """
        if isinstance(data, float):
            display_data = self.FLOAT_FORMAT_STR.format(data)
        else:
            display_data = str(data)
        item = create_table_item(display_data, editable=False)
        item.setData(data, self.DATA_SORT_ROLE)
        return item


class PeaksViewerPresenter:
    """Controls a PeaksViewerView with a given model to display
    the peaks table and interaction controls for single workspace.
    """

    class Event(Enum):
        PeaksListChanged = 1
        OverlayPeaks = 2
        SlicePointChanged = 3
        ClearPeaks = 4
        PeakSelected = 5

    def __init__(self, model: PeaksViewerModel, view: PeaksViewerView):
        """
        Constructs the view for the given PeaksWorkspace
        :param model: A handle to the view-model wrapper for PeaksWorkspace to be displayed
        :param view: A view object with a subscribe method to register this presenter
                     as a listener for view events
        """
        super().__init__()
        self._model = model
        self._raise_error_if_workspace_incompatible(model.peaks_workspace)
        self._peaks_table_presenter = PeaksWorkspaceDataPresenter(model, view.table_view)
        self._view = view
        view.subscribe(self)
        view.set_title(model.peaks_workspace.name())
        view.set_peak_color(model.fg_color)
        self.view.enable_calculate_hkl(self.model.can_calculate_hkl(view.frame))

        self.notify(PeaksViewerPresenter.Event.PeaksListChanged)

    @property
    def model(self):
        return self._model

    @property
    def view(self):
        return self._view

    def notify(self, event: Event):
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
            self._peaks_list_changed()
        elif event == PresenterEvent.ClearPeaks:
            self._clear_peaks()
        else:
            from mantid.kernel import logger

            logger.warning("PeaksViewer: Unknown event detected: {}".format(event))

    def add_peak(self, pos):
        self.model.add_peak(pos, self._view.frame)

    def delete_peak(self, pos):
        self.model.delete_peak(pos, self._view.frame)

    def _clear_peaks(self):
        """Clear all peaks from this view"""
        self.view.clear_table_selection()
        self.model.clear_peak_representations()

    def _overlay_peaks(self):
        """
        Respond to request to overlay PeaksWorkspace.
          - Query current slicing information
          - Compute peaks representations
          - Draw overlays.
        """
        self._clear_peaks()
        self.model.draw_peaks(self._view.sliceinfo, self._view.painter, self._view.frame)
        self.view.call_canvas_draw()

    def _peak_selected(self):
        """
        Respond to the selection change of a peak in the list
        """
        selected_index = self._view.selected_index
        if selected_index is None or not self.model.has_representations_drawn():
            return

        # Two step:
        #   - first update slice point so we are in the correct plane
        #   - find and set limits required to "zoom" to the selected peak
        self._view.set_slicepoint(self.model.slicepoint(selected_index, self._view.sliceinfo, self._view.frame))
        self._view.set_axes_limits(*self.model.viewlimits(selected_index), auto_transform=False)

    def _peaks_list_changed(self):
        """
        Respond to a change in the peaks list in the model
        """
        self._peaks_table_presenter.refresh()
        self.view.table_view.enable_sorting(PeaksWorkspaceDataPresenter.DATA_SORT_ROLE)

    def concise_checkbox_changes(self, concise):
        """
        Respond to a change in the concise view check box state
        :param concise: bool to set concise or expanded view
        """
        self.view.table_view.filter_columns(concise, PeaksWorkspaceDataPresenter.HIDDEN_COLUMNS)

    def calc_hkl_checkbox_changes(self, calc_hkl):
        """
        Respond to a change in the calc_hkl check box state
        :param calc_hkl: bool to set calc_hkl or not
        """
        self.model.set_calculate_hkl(calc_hkl)
        self.notify(PeaksViewerPresenter.Event.OverlayPeaks)

    # private api
    @staticmethod
    def _raise_error_if_workspace_incompatible(ws):
        """
        :param ws: A reference to an object to check for compatability
        """
        if not hasattr(ws, "getNumberPeaks"):
            raise ValueError("Expected a PeaksWorkspace. Found {}.".format(type(ws)))


class PeaksViewerCollectionPresenter:
    """Controls a widget comprising of multiple PeasViewerViews to display and
    interact with multiple PeaksWorkspaces"""

    # constants
    # colors for each peaks workspace - it is unlikely there will be more than 3 at once
    FG_COLORS = ["#d62728", "#03ad06", "#17becf", "#e9f02e", "#e377c2"]  # ~red,  # ~green  # ~cyan  # ~yellow  # ~pink
    DEFAULT_BG_COLOR = "0.75"

    def __init__(self, view: PeaksViewerCollectionView):
        """
        :param view: View displaying the model information
        """
        self._view = view
        self._actions_view = view.peak_actions_view
        self._actions_view.subscribe(self)
        self._child_presenters: List[PeaksViewerPresenter] = []
        self._ads_observer = None
        self.setup_ads_observer()

    def setup_ads_observer(self):
        if self._ads_observer is None:
            self._ads_observer = SliceViewerADSObserver(self.replace_handle, self.rename_handle, self.clear_handle, self.delete_handle)

    def clear_observer(self):
        self._ads_observer = None

    @property
    def view(self):
        return self._view

    def append_peaksworkspace(self, name: str, index=-1) -> PeaksViewerPresenter:
        """
        Create and append a view for the given named workspace
        :param name: The name of a PeaksWorkspace.
        :param index: the index to insert the peaksworkspace in the PeaksViewerCollectionView
        :returns: The child presenter
        """
        self.setup_ads_observer()
        presenter = PeaksViewerPresenter(self._create_peaksviewer_model(name), self._view.append_peaksviewer(index))
        self._child_presenters.append(presenter)
        return presenter

    def overlay_peaksworkspaces(self, names_to_overlay, index=-1):
        """
        :param names_to_overlay: The list of names to overlay
        :param index: the index to insert the new peaksworkspace in the PeaksViewerCollectionView
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
            self.append_peaksworkspace(name, index=index)

        self.notify(PeaksViewerPresenter.Event.OverlayPeaks)

        self._actions_view.set_peaksworkspace(self.workspace_names())

        self.view.setVisible(bool(self.workspace_names()))
        if not self.workspace_names():
            self.view.peak_actions_view.deactivate_peak_adding()

    def remove_peaksworkspace(self, name):
        """
        Remove the named workspace from display. No op if no workspace can be found with that name
        :param name: The name of a workspace
        :return: index of removed PeaksViewerView within PeaksViewerCollectionView
        """
        self.setup_ads_observer()
        child_presenters = self._child_presenters
        presenter_to_remove = None
        for child in child_presenters:
            if child.model.get_peaks_workspace_name() == name:
                presenter_to_remove = child
                child.notify(PeaksViewerPresenter.Event.ClearPeaks)
                index = self._view.remove_peaksviewer(child.view)

        child_presenters.remove(presenter_to_remove)

        # update combo box for add/remove peak actions
        self._actions_view.set_peaksworkspace(self.workspace_names())

        # hide if no peak tables remain
        if not child_presenters:
            self._view.hide()

        return index

    def workspace_names(self):
        """
        :return: A list of workspace names for each PeaksWorkspace displayed
        """
        names = []
        for presenter in self._child_presenters:
            names.append(presenter.model.get_peaks_workspace_name())

        return names

    def child_presenter(self, identifier: Union[int, str]) -> PeaksViewerPresenter:
        r"""
        @brief Get one of the presenters corresponding to one workspace
        :param identifier: name of the peaks workspace, or index in the list of peaks workspaces
        """
        if isinstance(identifier, int):
            return self._child_presenters[identifier]
        elif isinstance(identifier, str):
            for index, name in enumerate(self.workspace_names()):
                if identifier == name:
                    return self._child_presenters[index]

    def notify(self, event):
        """Dispatch notification to all subpresenters"""
        self.setup_ads_observer()
        for presenter in self._child_presenters:
            presenter.notify(event)

    def _create_peaksviewer_model(self, name: str) -> PeaksViewerModel:
        """
        Create a model for the given PeaksWorkspace with an appropriate color
        :param name: The name of a PeaksWorkspace.
        :return A new PeaksViewerModel object
        """
        # select first unused color
        used_colors = map(lambda p: p.model.fg_color, self._child_presenters)
        fg_color = None
        for color in self.FG_COLORS:
            if color not in used_colors:
                fg_color = color
                break

        if fg_color is None:
            # use black in this rare case
            fg_color = "#000000"

        return create_peaksviewermodel(name, fg_color, self.DEFAULT_BG_COLOR)

    def replace_handle(self, ws_name, _):
        if ws_name in self.workspace_names():
            # get the index of the removed workspace so we can insert it back into the same position
            index = self.remove_peaksworkspace(ws_name)
            self.overlay_peaksworkspaces(self.workspace_names() + [ws_name], index=index)

    def delete_handle(self, ws_name):
        if ws_name in self.workspace_names():
            self.remove_peaksworkspace(ws_name)

    def clear_handle(self):
        # This is likely handled at a higher level anyway, because SliceViewer closes given a clear all on the ADS.
        for ws_name in self.workspace_names():
            self.remove_peaksworkspace(ws_name)

    def rename_handle(self, ws_name, new_name):
        if ws_name in self.workspace_names():
            # get the index of the removed workspace so we can insert it back into the same position
            index = self.remove_peaksworkspace(ws_name)
            self.overlay_peaksworkspaces(self.workspace_names() + [new_name], index=index)

    #
    # Peak Actions Functionality
    #
    def add_delete_peak(self, pos):
        presenter_active = self.child_presenter(self._actions_view.active_peaksworkspace)
        if presenter_active is None:
            logger.debug("PeaksViewer: No active peak presenter")
        elif self._actions_view.adding_mode_on:
            logger.debug(f"PeaksViewer: Adding peak position {pos}")
            presenter_active.add_peak(pos)
        elif self._actions_view.erasing_mode_on:
            logger.debug(f"PeaksViewer: Deleting peak nearest to position {pos}")
            presenter_active.delete_peak(pos)
        else:
            logger.debug(f"PeaksViewer: Ignoring peak action position {pos}")

    def deactivate_peak_add_delete(self):
        self._actions_view.deactivate_peak_adding()

    def deactivate_zoom_pan(self, active):
        if active:
            self.view.deactivate_zoom_pan()
