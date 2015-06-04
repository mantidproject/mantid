#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class CalMuonDeadTimeTest(stresstesting.MantidStressTest):
    '''Tests the StepScan workflow algorithm'''

    def runTest(self):
        Load(Filename='EMU30604.nxs',OutputWorkspace='EMU30604')
        CalMuonDeadTime(InputWorkspace='EMU30604',
                        DeadTimeTable='deadTable',
                        FirstGoodData=0.5,
                        LastGoodData=10,
                        DataFitted='fitTable')
        GroupWorkspaces(InputWorkspaces='deadTable,fitTable',
                        OutputWorkspace='EMUCalMuonDeadTime')

    def validate(self):
        return 'EMUCalMuonDeadTime','EMUCalMuonDeadTime.nxs'