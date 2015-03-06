import stresstesting
from mantid.simpleapi import *
from mantid import config
from ISISCommandInterface import *

class SANS2DMultiPeriodAddFiles(stresstesting.MantidStressTest):
    
  def requiredMemoryMB(self):
    """Requires 2.5Gb"""
    return 2500

  def runTest(self):
    
    SANS2D()
    Set1D()
    Detector("rear-detector")
    MaskFile('MASKSANS2Doptions.091A')
    Gravity(True)

    add_runs( ('5512', '5512') ,'SANS2D', 'nxs', lowMem=True)

    #one period of a multi-period Nexus file
    AssignSample('5512-add.nxs', period=7)
    
    WavRangeReduction(2, 4, DefaultTrans)

    os.remove(os.path.join(config['defaultsave.directory'],'SANS2D00005512-add.nxs'))
    os.remove(os.path.join(config['defaultsave.directory'],'SANS2D00005512.log'))
    
  def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Instrument')
    self.disableChecking.append('Axes')
    
    return '5512p7rear_1D_2.0_4.0Phi-45.0_45.0','SANS2DMultiPeriodAddFiles.nxs'
