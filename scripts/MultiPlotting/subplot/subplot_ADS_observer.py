# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import AnalysisDataServiceObserver


class SubplotADSObserver(AnalysisDataServiceObserver):

    def __init__(self, subplot):
        super(SubplotADSObserver, self).__init__()
        self._subplot = subplot

        # self.observeClear(True)
        self.observeDelete(True)
        self.observeReplace(True)

    def deleteHandle(self, workspace_name, workspace):
        """
        Called when the ADS has deleted a workspace. Checks
        subplots for anly lines for that workspace and removes them.
        :param workspace_name: The name of the workspace
        :param workspace: not used
        """
        self._subplot._rm_ws_from_plots(workspace_name)

    def replaceHandle(self, workspace_name, workspace):
        """
        Called when the ADS has replaced a workspace with one of the same name.
        If this workspace is attached tho this figure then its data is updated
        :param workspace_name: The name of the workspace. Unused
        :param workspace: A reference to the new workspace
        """
        self._subplot._replaced_ws(workspace)
