# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from functools import wraps
import sys

from mantid.api import AnalysisDataServiceObserver
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall


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


class InstrumentViewADSObserver(AnalysisDataServiceObserver):
    def __init__(self, delete_callback, clear_callback, rename_callback, replace_callback):
        super(InstrumentViewADSObserver, self).__init__()
        self.delete_callback = QAppThreadCall(delete_callback)
        self.clear_callback = QAppThreadCall(clear_callback)
        self.rename_callback = QAppThreadCall(rename_callback)
        self.replace_callback = QAppThreadCall(replace_callback)

        self.observeDelete(True)
        self.observeRename(True)
        self.observeClear(True)
        self.observeReplace(True)

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
    def replaceHandle(self, workspace_name, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        :param workspace_name: The name of the workspace. Unused
        :param workspace: A reference to the new workspace
        """
        self.replace_callback(workspace_name, workspace)

    @_catch_exceptions
    def clearHandle(self):
        """
        Called when the ADS has been cleared, removes all data.
        """
        self.clear_callback()
