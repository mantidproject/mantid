#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *

#====================================================================================================
class MolDynCdlTest(stresstesting.MantidStressTest):

    def runTest(self):
        from mantid.simpleapi import MolDyn

        MolDyn(Filename='DISF_NaF.cdl',
               Functions=['Fqt-total', 'Sqw-total'],
               Plot='None',
               Save=False,
               OutputWorkspace='ISISIndirectSimulationTest_MolDynCdl')


    def validate(self):
        self.tolerance = 1e-2
        self.disableChecking.append("Instrument")

        from mantid.simpleapi import Load

        Load(Filename='ISISIndirectSimulation_MolDynCDL.nxs',OutputWorkspace='ISISIndirectSimulation_MolDynCDL')
        Load(Filename='ISISIndirectSimulation_MolDynCDL_SQW.nxs',OutputWorkspace='ISISIndirectSimulation_MolDynCDL_SQW')

        # check each of the resulting workspaces match
        ws1Match = self.checkWorkspacesMatch('DISF_NaF_Fqt-total', 'ISISIndirectSimulation_MolDynCDL')
        ws2Match = self.checkWorkspacesMatch('DISF_NaF_Sqw-total', 'ISISIndirectSimulation_MolDynCDL_SQW')

        return  ws1Match and ws2Match


    def checkWorkspacesMatch(self, ws1, ws2):
        """
        Function to check two workspaces match
        Used when the result of a test produces more than a single workspace
        """

        from mantid.simpleapi import SaveNexus, AlgorithmManager

        checker = AlgorithmManager.create("CheckWorkspacesMatch")
        checker.setLogging(True)
        checker.setPropertyValue("Workspace1", ws1)
        checker.setPropertyValue("Workspace2", ws2)
        checker.setPropertyValue("Tolerance", str(self.tolerance))
        checker.setPropertyValue("CheckInstrument","0")

        checker.execute()

        if checker.getPropertyValue("Result") != 'Success!':
            print self.__class__.__name__
            SaveNexus(InputWorkspace=ws2,Filename=self.__class__.__name__+'-mismatch.nxs')
            return False

        return True


#====================================================================================================
class MolDynDatTest(stresstesting.MantidStressTest):

    def runTest(self):
        from mantid.simpleapi import MolDyn

        MolDyn(Filename='WSH_test.dat',
               Plot='None',
               Save=False,
               OutputWorkspace='WSH_test_iqt')


    def validate(self):
        self.tolerance = 1e-2
        self.disableChecking.append("Instrument")

        return 'WSH_test_iqt', 'ISISIndirectSimulation_MolDynDAT.nxs'

