# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AnalysisDataServiceObserver
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


class FittingADSObserver(AnalysisDataServiceObserver):
    def __init__(self, delete_callback, clear_callback, replace_callback, rename_callback):
        super(FittingADSObserver, self).__init__()
        self.delete_callback = delete_callback
        self.clear_callback = clear_callback
        self.replace_callback = replace_callback
        self.rename_callback = rename_callback

        self.observeDelete(True)
        self.observeRename(True)
        self.observeReplace(True)
        self.observeClear(True)

    @_catch_exceptions
    def deleteHandle(self, workspace_name, workspace):
        """
        Called when the ADS deletes a workspace, removes it from the dict of tracked workspaces.
        :param workspace_name: name of the workspace
        :param workspace: reference to the workspace (not used)
        """
        self.delete_callback(workspace_name)

    @_catch_exceptions
    def renameHandle(self, old_workspace_name, new_workspace_name):
        """
        Called when the ADS renames a workspace, updates the dict with the new name.
        :param old_workspace_name: original name of the workspace
        :param new_workspace_name: new name for the workspace
        """
        self.rename_callback(old_workspace_name, new_workspace_name)

    @_catch_exceptions
    def clearHandle(self):
        """
        Called when the ADS has been cleared, removes all data.
        """
        self.clear_callback()

    @_catch_exceptions
    def replaceHandle(self, name, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        Updates the workspace stored in the dict.
        :param name: The name of the workspace.
        :param workspace: A reference to the new workspace
        """
        self.replace_callback(name, workspace)

    def unsubscribe(self):
        self.observeDelete(False)
        self.observeRename(False)
        self.observeClear(False)
