# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataServiceObserver, WorkspaceGroup
from functools import wraps
import sys


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


class MuonContextADSObserver(AnalysisDataServiceObserver):
    def __init__(self, delete_callback, clear_callback, replace_callback, delete_group_callback=None):
        super(MuonContextADSObserver, self).__init__()
        self.delete_callback = delete_callback
        self.clear_callback = clear_callback
        self.replace_callback = replace_callback
        self.delete_group_callback = delete_group_callback

        self.observeDelete(True)
        self.observeRename(True)
        self.observeReplace(True)
        self.observeClear(True)

    @_catch_exceptions
    def deleteHandle(self, workspace_name, workspace):
        """
        Called when the ADS has deleted a workspace. Removes that workspace from the context and cleans up.
        :param workspace_name: The name of the workspace (not used)
        :param workspace: The workspace object
        """
        if not isinstance(workspace, WorkspaceGroup):
            self.delete_callback(workspace)
        elif self.delete_group_callback is not None:
            self.delete_group_callback(workspace)

    @_catch_exceptions
    def renameHandle(self, old_workspace_name, new_workspace_name):
        """
        Called when the ADS has renamed a workspace. Currently treats this the same as deletion.
        :param workspace_name: The name of the workspace
        :param workspace: not used
        """
        self.delete_callback(old_workspace_name)

    @_catch_exceptions
    def clearHandle(self):
        """
        Called when the ADS has been cleared, removes all data and rests the GUI
        """
        self.clear_callback()

    @_catch_exceptions
    def replaceHandle(self, name, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        If this workspace is attached to this figure then its data is updated
        :param name: The name of the workspace.
        :param workspace: A reference to the new workspace (Unused)
        """
        self.replace_callback(workspace)

    def unsubscribe(self):
        self.observeDelete(False)
        self.observeRename(False)
        self.observeClear(False)
        self.observeReplace(False)
