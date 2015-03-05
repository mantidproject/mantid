import stresstesting
from mantid.simpleapi import *
from ISISCommandInterface import *

class LOQTransFitWorkspace2D(stresstesting.MantidStressTest):
    """
        Tests the SANS interface commands TransFit() and TransWorkspace(). Also tests
        a LOQ reduction in 2D with can and transmission files
    """
    
    def runTest(self):
        self.setup()

        #test TransFit()
        TransFit('LOG',3.0,8.0)
        TransmissionSample('54435.raw', '54433.raw')
        TransmissionCan('54434.raw', '54433.raw')

        #run the reduction
        WavRangeReduction(3, 4, False, '_suff')

        #save the results, we'll use them later, remove the other tempory workspaces
        RenameWorkspace(InputWorkspace='54435_trans_sample_3.0_8.0',OutputWorkspace= 'samp')
        RenameWorkspace(InputWorkspace='54434_trans_can_3.0_8.0',OutputWorkspace= 'can')
        DeleteWorkspace(Workspace='54435_trans_sample_3.0_8.0_unfitted')
        DeleteWorkspace(Workspace='54434_trans_can_3.0_8.0_unfitted')
        DeleteWorkspace(Workspace='54431main_2D_3.0_4.0_suff')

        #now test TransWorkspace()
        self.setup()
        #use the results we calculated above
        TransWorkspace('samp', 'can')

        WavRangeReduction(3, 4, False, '_suff')

    def setup(self):
        #DataPath("../Data/LOQ/")
        #UserPath("../Data/LOQ/")
        LOQ()
        MaskFile('MASK.094AA')
        Gravity(False)
        Set2D()
        Detector("main-detector-bank")
        AssignSample('54431.raw')
        AssignCan('54432.raw')
        LimitsWav(3,4, 0.2, 'LIN')

    def validate(self):
        self.disableChecking.append('SpectraMap')
        #when comparing LOQ files you seem to need the following
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '54431main_2D_3.0_4.0_suff','LOQTransFitWorkspace2D.nxs'
