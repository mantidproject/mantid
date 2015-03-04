import stresstesting

from mantid.simpleapi import *
from ISISCommandInterface import *
from mantid.simpleapi import *
from mantid import config
from SANSBatchMode import *
import os.path

# test batch mode with sans2d and selecting a period in batch mode
class SANS2DBatch(stresstesting.MantidStressTest):
    
  def runTest(self):

    SANS2D()
    Set1D()
    Detector("rear-detector")
    MaskFile('MASKSANS2Doptions.091A')
    Gravity(True)
    
    csv_file = FileFinder.getFullPath('SANS2D_periodTests.csv')
    
    BatchReduce(csv_file, 'nxs', plotresults=False, saveAlgs={'SaveCanSAS1D':'xml','SaveNexus':'nxs'})
        
    os.remove(os.path.join(config['defaultsave.directory'],'5512p7_SANS2DBatch.xml'))
    
  def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Axes')
    self.disableChecking.append('Instrument')
    
    return '5512p7_SANS2DBatch','SANS2DBatch.nxs'

class SANS2DNewSettingsCarriedAcrossInBatchMode(stresstesting.MantidStressTest):
    """
    We want to make sure that any settings saved in the PropertyManager objects
    are used across all iterations of the reduction in Batch mode.  The MASKFILE
    command uses this new way of storing settings in ISIS SANS, and so we'll
    see if the same masks get applied in the second iteration as they do in the
    first.
    """
    def runTest(self):
        config['default.instrument'] = 'SANS2D'
        SANS2D()
        Set1D()
        Detector("rear-detector")
        # This contains two MASKFILE commands, each resulting in a seperate call to MaskDetectors.
        MaskFile('MaskSANS2DReductionGUI_MaskFiles.txt')
        Gravity(True)

        # This does 2 seperate reductions of the same data, but saving the result of each to a different workspace.
        csv_file = FileFinder.getFullPath("SANS2D_mask_batch.csv")
        BatchReduce(csv_file, 'nxs', plotresults=False)

    def validate(self):
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2
        return "iteration_2", "SANS2DNewSettingsCarriedAcross.nxs"
