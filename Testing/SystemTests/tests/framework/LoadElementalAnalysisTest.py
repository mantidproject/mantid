# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from mantid.api import AnalysisDataService
from mantid.simpleapi import LoadElementalAnalysisData, GroupWorkspaces

class LoadElementalAnalysisTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(LoadElementalAnalysisTest, self).__init__()

    def requiredFiles(self):
        return ['ral09999.rooth2010.dat', 'ral09999.rooth2020.dat', 'ral09999.rooth2099.dat',
                'ral09999.rooth3010.dat', 'ral09999.rooth3020.dat', 'ral09999.rooth3099.dat',
                'ral09999.rooth4010.dat', 'ral09999.rooth4020.dat', 'ral09999.rooth4099.dat',
                'ral09999.rooth5010.dat', 'ral09999.rooth5020.dat', 'ral09999.rooth5099.dat']

    def cleanup(self):
        AnalysisDataService.clear()

    def runTest(self):
        LoadElementalAnalysisData(Run='9999',
                            OutputWorkspace='9999')

    def validate(self):
        self.tolerance = 0.0001

        return ['9999', 'ElementalAnalysisLoad.nxs']