# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
from __future__ import (absolute_import, division, print_function)
import systemtesting
import mantid.simpleapi as ms

#====================================================================================================


class MolDynCdlTest(systemtesting.MantidSystemTest):

    def runTest(self):
        ms.MolDyn(Data='DISF_NaF.cdl',
                  Functions=['Fqt-total', 'Sqw-total'],
                  OutputWorkspace='ISISIndirectSimulationTest_MolDynCdl')

    def validate(self):
        self.tolerance = 1e-2
        self.disableChecking.append("Instrument")

        ms.Load(Filename='ISISIndirectSimulation_MolDynCDL.nxs',
                OutputWorkspace='ISISIndirectSimulation_MolDynCDL')
        ms.Load(Filename='ISISIndirectSimulation_MolDynCDL_SQW.nxs',
                OutputWorkspace='ISISIndirectSimulation_MolDynCDL_SQW')

        # check each of the resulting workspaces match
        ws1Match = self.checkWorkspacesMatch('DISF_NaF_Fqt-total', 'ISISIndirectSimulation_MolDynCDL')
        ws2Match = self.checkWorkspacesMatch('DISF_NaF_Sqw-total', 'ISISIndirectSimulation_MolDynCDL_SQW')

        return  ws1Match and ws2Match

    def checkWorkspacesMatch(self, ws1, ws2):
        """
        Function to check two workspaces match
        Used when the result of a test produces more than a single workspace
        """

        checker = ms.AlgorithmManager.create("CompareWorkspaces")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1", ws1)
        checker.setPropertyValue("Workspace2", ws2)
        checker.setPropertyValue("Tolerance", str(self.tolerance))
        checker.setPropertyValue("CheckInstrument","0")

        checker.execute()

        if not checker.getProperty("Result"):
            print(self.__class__.__name__)
            ms.SaveNexus(InputWorkspace=ws2,Filename=self.__class__.__name__+'-mismatch.nxs')
            return False

        return True


#====================================================================================================
class MolDynDatTest(systemtesting.MantidSystemTest):

    def runTest(self):
        ms.MolDyn(Data='WSH_test.dat',
                  OutputWorkspace='WSH_test_iqt')

    def validate(self):
        self.tolerance = 1e-2
        self.disableChecking.append("Instrument")

        return 'WSH_test_iqt', 'ISISIndirectSimulation_MolDynDAT.nxs'
