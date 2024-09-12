# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
"""
Contains the presenter for displaying the InstrumentWidget
"""

from qtpy.QtCore import Qt

from mantid.api import AnalysisDataService
from mantidqt.widgets.observers.ads_observer import WorkspaceDisplayADSObserver
from mantidqt.widgets.observers.observing_presenter import ObservingPresenter
from .view import InstrumentView


class InstrumentViewPresenter(ObservingPresenter):
    """
    Presenter holding the view widget for the InstrumentView.
    It has no model as its an old widget written in C++ with out MVP
    """

    """
    @param ws : The workspace object OR workspace name.
    """

    def __init__(self, ws, parent=None, window_flags=Qt.Window, ads_observer=None, view: InstrumentView = None):
        super(InstrumentViewPresenter, self).__init__()
        self.ws_name = str(ws)

        self.container = view
        if not self.container:
            workspace = AnalysisDataService.retrieve(self.ws_name)
            workspace.readLock()
            try:
                self.container = InstrumentView(parent=parent, presenter=self, name=self.ws_name, window_flags=window_flags)
            finally:
                workspace.unlock()

        if ads_observer:
            self.ads_observer = ads_observer
        else:
            self.ads_observer = WorkspaceDisplayADSObserver(self, observe_replace=False)

        # TODO FIXME - this may not be a good design.  It violates the OO principles
        # Update the instrument view manager
        InstrumentViewManager.register(self, self.ws_name)

    def current_workspace_equals(self, name):
        return self.ws_name == name

    """
    Replace the workspace being shown by the instrument widget.
    @param new_workspace_name : the name of the new workspace to set
    @param new_window_name : the new title of the window. Optional, if none provided, uses the name of the workspace.
    """

    def replace_workspace(self, new_workspace_name, new_window_name=None):
        self.container.replace_workspace(new_workspace_name, new_window_name)

    def rename_workspace(self, old_name, new_name):
        # rename is handled by the InstrumentWidget inside C++
        pass

    def show_view(self):
        self.container.show()

    def save_image(self, filename):
        self.container.save_image(filename)

    def get_current_tab(self):
        return self.container.get_current_tab()

    def get_render_tab(self):
        return self.container.get_render_tab()

    def get_pick_tab(self):
        return self.container.get_pick_tab()

    def select_render_tab(self):
        self.container.select_tab(0)

    def select_pick_tab(self):
        self.container.select_tab(1)

    def set_bin_range(self, min_x: float, max_x: float):
        """Set the binning range on X-axis"""
        self.container.set_range(min_x, max_x)

    def is_thread_running(self):
        return self.container.is_thread_running()

    def wait(self):
        self.container.wait()

    def close(self, workspace_name):
        """
        extend close()
        :param workspace_name: str
            workspace name
        :return:
        """
        if InstrumentViewManager.last_view == self:
            InstrumentViewManager.last_view = None

        if workspace_name == self.ws_name:
            super(InstrumentViewPresenter, self).close(self.ws_name)
            InstrumentViewManager.remove(self, self.ws_name)


class InstrumentViewManager:
    """
    InstrumentViewManager provide a singleton for client to access "Instrument View"
    in python/iPython console environment
    """

    # static instance to the last InstrumentView instance launched
    last_view = None
    # a dictionary to trace all the InstrumentView instances launched
    # key is the name of the workspace associated with the InstrumentView widget
    view_dict = dict()

    @staticmethod
    def register(instrument_view_obj, ws_name):
        """Register an InstrumentViewPresenter instance"""
        InstrumentViewManager.last_view = instrument_view_obj
        if ws_name in InstrumentViewManager.view_dict:
            InstrumentViewManager.view_dict[ws_name].append(instrument_view_obj)
        else:
            InstrumentViewManager.view_dict[ws_name] = [instrument_view_obj]

    @staticmethod
    def get_instrument_view(ws_name: str):
        """Get an InstrumentView widget by the name of the workspace associated with it"""
        if ws_name not in InstrumentViewManager.view_dict:
            # return None if the workspace does not exist
            return None
        return InstrumentViewManager.view_dict[ws_name]

    @staticmethod
    def remove(view_obj, ws_name: str):
        """Remove a registered InstrumentView"""
        try:
            # delete the record
            if ws_name in InstrumentViewManager.view_dict:
                # find the corresponding object if they have the same workspace name
                loc = InstrumentViewManager.view_dict[ws_name].index(view_obj)
                del InstrumentViewManager.view_dict[ws_name][loc]
                # clear the dictionary entry if it was the last item
                if len(InstrumentViewManager.view_dict[ws_name]) == 0:
                    del InstrumentViewManager.view_dict[ws_name]
        except KeyError as ke:
            # if it does not exist
            raise RuntimeError(
                f"workspace {ws_name} does not exist in dictionary. "
                f"The available includes {InstrumentViewManager.view_dict.items()},"
                f"FYI: {ke}"
            )
