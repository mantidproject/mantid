import stresstesting
from mantid.simpleapi import *
from ISISCommandInterface import *

class SANS2DFrontNoGrav(stresstesting.MantidStressTest):
    
  def runTest(self):

    SANS2D()
    MaskFile('MASKSANS2D_094i_RKH.txt')
    SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
    SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
    Gravity(False)
    Set1D()
    

    AssignSample('2500.nxs')

    WavRangeReduction(4.6, 12.85, False)

  def validate(self):
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Axes')
    self.disableChecking.append('Instrument')
    return '2500front_1D_4.6_12.85','SANS2DFrontNoGrav.nxs'