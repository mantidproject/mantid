# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)
from mantid.api import AnalysisDataServiceObserver


class MuonContextADSObserver(AnalysisDataServiceObserver):
    def __init__(self, delete_callback):
        super(MuonContextADSObserver, self).__init__()
        self.delete_callback = delete_callback

        self.observeDelete(True)

    def deleteHandle(self, workspace_name, workspace):
        """
        Called when the ADS has deleted a workspace. Removes that workspace from the context and cleans up.
        :param workspace_name: The name of the workspace
        :param workspace: not used
        """
        try:
            self.delete_callback(workspace_name)
        except Exception as e:
            print(e)
