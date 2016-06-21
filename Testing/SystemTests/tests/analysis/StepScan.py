#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class StepScanWorkflowAlgorithm(stresstesting.MantidStressTest):
    '''Tests the StepScan workflow algorithm'''

    def runTest(self):
        LoadMask(Instrument='HYS',InputFile=r'HYSA_mask.xml',OutputWorkspace='HYSA_mask')
        Load(Filename='HYSA_2934.nxs.h5',OutputWorkspace='HYSA_2934',LoadMonitors='1')
        StepScan(InputWorkspace='HYSA_2934',OutputWorkspace='StepScan',MaskWorkspace='HYSA_mask',
                 XMin='3.25',XMax='3.75',RangeUnit='dSpacing')

    def validate(self):
        return 'StepScan','StepScan.nxs'
