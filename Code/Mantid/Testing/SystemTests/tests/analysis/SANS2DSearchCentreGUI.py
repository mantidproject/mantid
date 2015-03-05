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
import SANS2DReductionGUI as sansgui

class SANS2DGUISearchCentre(sansgui.SANS2DGUIReduction):

  def checkCentreResult(self):
      self.checkFloat(i.ReductionSingleton().get_beam_center('rear')[0], 0.165)
      self.checkFloat(i.ReductionSingleton().get_beam_center('rear')[1], -0.145 )

  def runTest(self):
      self.singleModePrepare()
      
      i.FindBeamCentre(rlow=41,rupp=280,MaxIter=3,xstart=float(150)/1000.,ystart=float(-160)/1000., tolerance=0.0001251)
      self.checkCentreResult()
      # clean up
      
      i.ReductionSingleton.clean(isis_reducer.ISISReducer)
      i.ReductionSingleton().set_instrument(isis_instrument.SANS2D())
      i.ReductionSingleton().user_settings =isis_reduction_steps.UserFile(sansgui.MASKFILE)
      i.ReductionSingleton().user_settings.execute(i.ReductionSingleton())

  def validate(self):
      # there is no workspace to be checked against
      return True

if __name__ == "__main__":
    test = SANS2DGUISearchCentre()
    test.execute()

  






























