import stresstesting
from mantid.simpleapi import *
from ISISCommandInterface import *

class LOQCentreNoGrav(stresstesting.MantidStressTest):
    
  def runTest(self):

    LOQ()

    Set1D()
    Detector("rear-detector")
    MaskFile('MASK.094AA')
    Gravity(False)

    AssignSample('54431.raw')
    TransmissionSample('54435.raw', '54433.raw')
    AssignCan('54432.raw')
    TransmissionCan('54434.raw', '54433.raw')

    FindBeamCentre(60,200, 9)
    
    WavRangeReduction(3, 9, DefaultTrans)
    
  def validate(self):

    return '54431main_1D_3.0_9.0','LOQCentreNoGravSearchCentreFixed.nxs'

class LOQCentreNoGravDefineCentre(stresstesting.MantidStressTest):
  def runTest(self):

    LOQ()

    Set1D()
    Detector("rear-detector")
    MaskFile('MASK.094AA')
    Gravity(False)
    SetCentre(324.765, 327.670)

    AssignSample('54431.raw')
    TransmissionSample('54435.raw', '54433.raw')
    AssignCan('54432.raw')
    TransmissionCan('54434.raw', '54433.raw')

    WavRangeReduction(3, 9, DefaultTrans)
    
  def validate(self):
    # Need to disable checking of the Spectra-Detector map becauseit isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Axes')
    self.disableChecking.append('Instrument')

    return '54431main_1D_3.0_9.0','LOQCentreNoGrav.nxs'
  
