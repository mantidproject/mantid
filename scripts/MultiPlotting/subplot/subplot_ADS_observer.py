# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
from functools import wraps
import sys
from mantid.api import AnalysisDataServiceObserver
from mantid.plots import MantidAxes
from matplotlib.axes import Axes


def _catch_exceptions(func):
    """
    Catch all exceptions in method and print a traceback to stderr
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception:
            sys.stderr.write("Error occurred in handler:\n")
            import traceback
            traceback.print_exc()

    return wrapper


class SubplotADSObserver(AnalysisDataServiceObserver):

    def __init__(self, manager):
        super(SubplotADSObserver, self).__init__()
        self.window = manager.window
        self.canvas = manager.canvas

        self.observeClear(True)
        self.observeDelete(True)
        self.observeReplace(True)

    @_catch_exceptions
    def clearHandle(self):
        """
        Called when the ADS is deleted all of its workspaces
        """
        self.window.emit_close()

    @_catch_exceptions
    def deleteHandle(self, _, workspace):
        """
        Called when the ADS has deleted a workspace. Checks the
        attached axes for any hold a plot from this workspace. If removing
        this leaves empty axes then the parent window is triggered for
        closer
        :param _: The name of the workspace. Unused
        :param workspace: A pointer to the workspace
        """
        # Find the axes with this workspace reference
        all_axes = self.canvas.figure.axes
        if not all_axes:
            return

        # Here we wish to delete any curves linked to the workspace being
        # deleted and if a figure is now empty, close it. We must avoid closing
        # any figures that were created via the script window that are not
        # managed via a workspace.
        # See https://github.com/mantidproject/mantid/issues/25135.
        empty_axes = []
        for ax in all_axes:
            if isinstance(ax, MantidAxes):
                ax.remove_workspace_artists(workspace)
            # We check for axes type below as a pseudo check for an axes being
            # a colorbar. Creating a colorfill plot creates 2 axes: one linked
            # to a workspace, the other a colorbar. Deleting the workspace
            # deletes the colorfill, but the plot remains open due to the
            # non-empty colorbar. This solution seems to work for the majority
            # of cases but could lead to unmanaged figures only containing an
            # Axes object being closed.
            if type(ax) is not Axes:
                empty_axes.append(MantidAxes.is_empty(ax))

        if all(empty_axes):
            self.window.emit_close()
        else:
            self.canvas.draw_idle()

    @_catch_exceptions
    def replaceHandle(self, _, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        If this workspace is attached to this figure then its data is updated
        :param _: The name of the workspace. Unused
        :param workspace: A reference to the new workspace
        """
        redraw = False
        for ax in self.canvas.figure.axes:
            if isinstance(ax, MantidAxes):
                redraw_this = ax.replace_workspace_artists(workspace)
            else:
                continue
            redraw = redraw | redraw_this
        if redraw:
            self.canvas.draw_idle()

    # def __init__(self, subplot):
    #     super(SubplotADSObserver, self).__init__()
    #     self._subplot = subplot
    #
    #     self.observeDelete(True)
    #     self.observeReplace(True)
    #
    # def deleteHandle(self, workspace_name, workspace):
    #     """
    #     Called when the ADS has deleted a workspace. Checks
    #     subplots for anly lines for that workspace and removes them.
    #     :param workspace_name: The name of the workspace
    #     :param workspace: not used
    #     """
    #     try:
    #         self._subplot._rm_ws_from_plots(workspace_name)
    #     except Exception as error:
    #         print(error)
    #
    # def replaceHandle(self, workspace_name, workspace):
    #     """
    #     Called when the ADS has replaced a workspace with one of the same name.
    #     If this workspace is attached tho this figure then its data is updated
    #     :param workspace_name: The name of the workspace. Unused
    #     :param workspace: A reference to the new workspace
    #     """
    #     self._subplot._replaced_ws(workspace)
