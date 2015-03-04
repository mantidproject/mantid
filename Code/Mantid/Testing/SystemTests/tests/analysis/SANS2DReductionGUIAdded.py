import sys
import os

if __name__ == "__main__":
  # it is just to allow running this test in Mantid, allowing the following import
  sys.path.append('/apps/mantid/systemtests/StressTestFramework/')

from mantid.simpleapi import *
import ISISCommandInterface as i    
import isis_reducer
import isis_instrument
import isis_reduction_steps
import copy
import SANS2DReductionGUI as sansgui

class SANS2DReductionGUIAddedFiles(sansgui.SANS2DGUIReduction):
    def runTest(self):
        self.initialization()

        self.checkFirstPart()

        # add files (SAMPLE and CAN)
        import SANSadd2
        SANSadd2.add_runs(('22048','22048'),'SANS2D', '.nxs', rawTypes=('.add','.raw','.s*'), lowMem=False)
        SANSadd2.add_runs(('22023','22023'),'SANS2D', '.nxs', rawTypes=('.add','.raw','.s*'), lowMem=False)
        
        # load values: 
        i.SetCentre('155.45','-169.6','rear') 
        i.SetCentre('155.45','-169.6','front')
        SCATTER_SAMPLE, logvalues = i.AssignSample(r'SANS2D00022048-add.nxs', reload = True, period = 1)
        SCATTER_SAMPLE, logvalues = i.AssignCan(r'SANS2D00022023-add.nxs', reload = True, period = 1)
        i.TransmissionSample(r'SANS2D00022041.nxs', r'SANS2D00022024.nxs', period_t=1, period_d=1)
        i.TransmissionCan(r'SANS2D00022024.nxs', r'SANS2D00022024.nxs', period_t=1, period_d=1)

        self.checkAfterLoad()
        
        self.applyGUISettings()
        
        self.applySampleSettings()
        _user_settings_copy = copy.deepcopy(i.ReductionSingleton().user_settings)

        reduced = i.WavRangeReduction(full_trans_wav=False, resetSetup=False)
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')

        self.checkFittingSettings()
        self.cleanReduction(_user_settings_copy)

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be 
        # almost the same
        self.tolerance_is_reller = True
        self.tolerance = 0.35
        return "trans_test_rear","SANSReductionGUI.nxs"
        
        
if __name__ == "__main__":
  test = SANS2DReductionGUIAddedFiles()
  test.execute()
