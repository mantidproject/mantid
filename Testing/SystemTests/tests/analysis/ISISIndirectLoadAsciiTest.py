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


class IN10SiliconTest(systemtesting.MantidSystemTest):

    def runTest(self):
        import IndirectNeutron as Main

        instr = 'IN10'
        ana = 'silicon'
        refl = '111'
        run = 'P3OT_350K'
        rejectZ = False
        useM = False
        saveOp = False
        plotOp = False
        Main.InxStart(instr,run,ana,refl,rejectZ,useM,'',plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-2
        self.disableChecking.append("Instrument")
        return 'IN10_P3OT_350K_silicon111_red', 'ISISIndirectLoadAscii_IN10SiliconTest.nxs'

#====================================================================================================


class IN13CaFTest(systemtesting.MantidSystemTest):

    def runTest(self):
        import IndirectNeutron as Main

        instr = 'IN13'
        ana = 'CaF'
        refl = '422'
        run = '16347'
        rejectZ = False
        useM = False
        saveOp = False
        plotOp = False
        Main.IN13Start(instr,run,ana,refl,rejectZ,useM,'',plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-2

        ms.Load(Filename='ISISIndirectLoadAscii_IN13CaFTest.nxs',
                OutputWorkspace='ISISIndirectLoadAscii_IN13CaFTest')
        ms.Load(Filename='ISISIndirectLoadAscii_IN13CaFTest2.nxs',
                OutputWorkspace='ISISIndirectLoadAscii_IN13CaFTest2')

        # check each of the resulting workspaces match
        ws1Match = self.checkWorkspacesMatch('IN13_16347_CaF422_q', 'ISISIndirectLoadAscii_IN13CaFTest2')
        ws2Match = self.checkWorkspacesMatch('IN13_16347_CaF422_ang', 'ISISIndirectLoadAscii_IN13CaFTest')

        return  ws1Match and ws2Match

    # function to check two workspaces match
    # Used when the result of a test produces more than a single workspace
    def checkWorkspacesMatch(self, ws1, ws2):
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
class IN16SiliconTest(systemtesting.MantidSystemTest):

    def runTest(self):
        import IndirectNeutron as Main

        instr = 'IN16'
        ana = 'silicon'
        refl = '111'
        run = '65722'
        rejectZ = True
        useM = False
        saveOp = False
        plotOp = False
        Main.IbackStart(instr,run,ana,refl,rejectZ,useM,'',plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-2
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        return 'IN16_65722_silicon111_red', 'ISISIndirectLoadAscii_IN16SiliconTest.nxs'
