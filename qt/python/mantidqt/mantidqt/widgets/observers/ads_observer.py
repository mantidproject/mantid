# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import sys
from functools import wraps

from mantid.api import AnalysisDataServiceObserver


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


class WorkspaceDisplayADSObserver(AnalysisDataServiceObserver):
    def __init__(self, presenter, observe_replace=True, observe_group_update=False):
        super(WorkspaceDisplayADSObserver, self).__init__()
        self.presenter = presenter

        self.observeClear(True)
        self.observeDelete(True)
        self.observeReplace(observe_replace)
        self.observeRename(True)
        self.observeGroupUpdate(observe_group_update)

    @_catch_exceptions
    def clearHandle(self):
        """
        Called when the ADS is deleted all of its workspaces
        """
        self.presenter.force_close()

    @_catch_exceptions
    def deleteHandle(self, name, _):
        """
        Called when the ADS has deleted a workspace. Checks the
        attached axes for any hold a plot from this workspace. If removing
        this leaves empty axes then the parent window is triggered for
        closer
        :param _: The name of the workspace. Unused
        :param __: A pointer to the workspace. Unused
        """
        self.presenter.close(name)

    @_catch_exceptions
    def replaceHandle(self, name, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        If this workspace is attached to this figure then its data is updated
        :param _: The name of the workspace. Unused
        :param workspace: A reference to the new workspace
        """
        self.presenter.replace_workspace(name, workspace)

    @_catch_exceptions
    def renameHandle(self, old_name, new_name):
        self.presenter.rename_workspace(old_name, new_name)

    @_catch_exceptions
    def groupUpdateHandle(self, ws_name, workspace):
        self.presenter.group_update(ws_name, workspace)
