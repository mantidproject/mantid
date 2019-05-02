# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import AnalysisDataServiceObserver
from functools import wraps
import mantid.simpleapi as mantid

class SubplotADSObserver(AnalysisDataServiceObserver):

    def __init__(self, context):
        super(SubplotADSObserver, self).__init__()
        self._context= context

        #self.observeClear(True)
        self.observeDelete(True)
        self.observeReplace(True)

    def deleteHandle(self, workspace_name, _):
        """
        Called when the ADS has deleted a workspace. Checks
        subplots for anly lines for that workspace and removes them.
        :param workspace_name: The name of the workspace
        :param _: not used
        """
        self._context._rm_ws_from_plots(workspace_name)

    def replaceHandle(self, workspace_name, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        If this workspace is attached tho this figure then its data is updated
        :param workspace_name: The name of the workspace. Unused
        :param workspace: A reference to the new workspace
        """
        self._context._replaced_ws( workspace)
