# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import sys
from functools import wraps

import mantid
from mantid.api import AnalysisDataServiceObserver


def _catch_exceptions(func):
    """Catch all exceptions in method and print a traceback to stderr"""

    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception:
            sys.stderr.write("Error occurred in handler:\n")
            import traceback
            traceback.print_exc()

    return wrapper


class SliceViewerADSObserver(AnalysisDataServiceObserver):

    def __init__(self, presenter):
        super(SliceViewerADSObserver, self).__init__()
        self.presenter = presenter
        self.view = self.presenter.view
        self.logger = mantid.kernel.Logger("SliceViewerADSObserver")

        self.observeClear(True)
        self.observeDelete(True)
        self.observeReplace(True)
        self.observeRename(True)

    @_catch_exceptions
    def clearHandle(self):
        """Called when the ADS is deleted all of its workspaces"""
        self.view.emit_close()

    @_catch_exceptions
    def deleteHandle(self, ws_name, _):
        """
        Called when the ADS has deleted a workspace. Checks if
        the deleted workspace is the same as the one used for the
        sliceviewer model. If so, sliceviewer is closed.
        :param ws_name: The name of the workspace.
        :param _: A pointer to the workspace. Unused
        """
        if ws_name == str(self.presenter.model._get_ws()):
            self.view.emit_close()

    @_catch_exceptions
    def replaceHandle(self, _, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        If this workspace is the same as that of the sliceviewer model, the model
        is updated.
        :param _: The name of the workspace. Unused
        :param workspace: A reference to the new workspace
        """
        self.logger.notice("replace" + str(workspace))

    @_catch_exceptions
    def renameHandle(self, oldName, newName):
        """
        Called when the ADS has renamed a workspace.
        If this workspace is the same as that of the sliceviewer model, its name
        is updated.
        :param oldName: The old name of the workspace.
        :param newName: The new name of the workspace
        """
        self.logger.notice("rename" + str(oldName) + str(newName))
