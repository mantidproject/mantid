import stresstesting
from mantid.simpleapi import * 
from mantid.api import Workspace
from ISISCommandInterface import *
import numpy
import unittest

## export PYTHONPATH=/apps/workspace/mantid_debug/bin/:/apps/mantid/systemtests/StressTestFramework/:/apps/mantid/mantid/Code/Mantid/scripts/SANS/:/apps/mantid/mantid/Code/Mantid/scripts/reduction


"""
Allowing the reduction to use already loaded workspace will make it easier to 
deal with event mode and producing new workspaces for the reduction of data.
Till 06/2013 the reload option was available, but not implemented. 

In order to protect the system, it is suggested the following integration tests
to ensure that allowing workspaces as input to the reduction will not disturb the 
reduction itself, and it is safe.

LOQReductionShouldAcceptLoadedWorkspace ensure some requirements for the reloading. 
SANS2DReductionShouldAcceptLoadedWorkspace and SANS2DReductionShouldAcceptLoadedWorkspaceRawFile
apply the same requirements for SANS2D instruments. 


LOQReductionShouldAcceptLoadedWorkspaceStressTest, SANS2DReductionShouldAcceptLoadedWorkspaceStressTest
and SANS2DReductionShouldAcceptLoadedWorkspace are wrappers to make unittest.TestCase to fit the stresstesting
framework. 

The other tests are here to ensure the results of providing directly workspaces will be the same that loading
from files. 

"""

class LOQReductionShouldAcceptLoadedWorkspace(unittest.TestCase):
    """
    The following tests is to ensure that the reload obeys the following requirement: 
     * If reload is True the real data will be always reloaded from the file
     * If reload is False, it will be used, if it pass the following tests: 
       * The instrument components have not been moved
    """
    def setUp(self):
        self.load_run = '54431.raw'
        config["default.instrument"] = "LOQ"
        LOQ()
        MaskFile("MASK.094AA")
        self.control_name = '54431main_1D_2.2_10.0'
        self.inst_comp = 'main-detector-bank'

    def tearDown(self):
        mtd.clear()

    def test_accept_loaded_workspace_only_if_reload_false(self):
        my_workspace = Load(self.load_run)
        #set the value for my_workspace to ensure it is the one used
        aux = my_workspace.dataY(0)
        aux[10]=5
        my_workspace.setY(0,aux)
        # ask to use the loaded workspace
        AssignSample(my_workspace,reload=False)
                
        ws_name = ReductionSingleton().get_sample().get_wksp_name()
        
        self.assertTrue(ws_name, my_workspace.name())
        
        self.assertTrue(my_workspace.dataY(0)[10],5)
        # ensure that it is able to execute the reduction
        Reduce()
        self.assertTrue(self.control_name in mtd)


    def test_accept_loaded_workspace_but_reload_the_data_file_if_reload_true(self):
        my_workspace = Load(self.load_run)
        #set the value for my_workspace to ensure it is the one used
        aux = my_workspace.dataY(0)
        aux[10]=5
        my_workspace.setY(0,aux)
        # ask to use the loaded workspace
        AssignSample(my_workspace,reload=True)
                
        ws_name = ReductionSingleton().get_sample().get_wksp_name()
        # it is different, because, it will compose the name using its rule, 
        # wich, for sure, will be different of my_workspace.
        self.assertFalse(ws_name==my_workspace.name())
        self.assertFalse(mtd[ws_name].dataY(0)[10]==5)
        # it is not necessary to ensure the Reduce occurs

    def test_should_not_accept_loaded_workspace_if_moved(self):
        my_workspace = Load(self.load_run)
        MoveInstrumentComponent(my_workspace,self.inst_comp,X=2,Y=1,Z=0)
        ## attempt to use a workspace that has been moved
        self.assertRaises(RuntimeError, AssignSample, my_workspace, False)


    def test_should_not_accept_loaded_workspace_if_moved_2(self):
        # assign sample loads and move the workspace to the defined center
        AssignSample(self.load_run)

        # this makes it load this worksapce and generates an output workspace
        ws_name = ReductionSingleton().get_sample().get_wksp_name()
        # the workspace is renamed, so it seems another workspace
        my_workspace = RenameWorkspace(ws_name)
        ## trying to assing it again to AssingSample must fail
        self.assertRaises(RuntimeError, AssignSample, my_workspace, False)

class SANS2DReductionShouldAcceptLoadedWorkspace(LOQReductionShouldAcceptLoadedWorkspace):
    def setUp(self):
        self.load_run = '2500.nxs'
        config["default.instrument"] = "SANS2D"
        SANS2D()
        MaskFile("MASKSANS2D_094i_RKH.txt")
        self.control_name = '2500front_1D_4.6_12.85'
        self.inst_comp = 'rear-detector'

class SANS2DReductionShouldAcceptLoadedWorkspaceRawFile(SANS2DReductionShouldAcceptLoadedWorkspace):
    def setUp(self):
        SANS2DReductionShouldAcceptLoadedWorkspace.setUp(self)
        self.load_run = '5547.raw'
        self.control_name = '5547front_1D_4.6_12.85'

class LOQReductionShouldAcceptLoadedWorkspaceStressTest(stresstesting.MantidStressTest):
  cl = LOQReductionShouldAcceptLoadedWorkspace
  def runTest(self):
    self._success = False
    # Custom code to create and run this single test suite
    suite = unittest.TestSuite()
    suite.addTest( unittest.makeSuite(self.cl, "test"))
    runner = unittest.TextTestRunner()
    # Run using either runner
    res = runner.run(suite)
    if res.wasSuccessful():
        self._success = True

  def validate(self):
    return self._success 

class SANS2DReductionShouldAcceptLoadedWorkspaceStressTest(LOQReductionShouldAcceptLoadedWorkspaceStressTest):
    cl = SANS2DReductionShouldAcceptLoadedWorkspace

class SANS2DReductionShouldAcceptLoadedWorkspaceStressTest2(LOQReductionShouldAcceptLoadedWorkspaceStressTest):
    cl = SANS2DReductionShouldAcceptLoadedWorkspaceRawFile


class LOQTransFitWorkspace2DWithLoadedWorkspace(stresstesting.MantidStressTest):
    def runTest(self):
        config["default.instrument"] = "LOQ"
        LOQ()
        MaskFile('MASK.094AA')
        Gravity(False)
        Set2D()
        Detector("main-detector-bank")
        Sample = LoadRaw('54431.raw')
        AssignSample(Sample,False)
        Can = LoadRaw('54432.raw')
        AssignCan(Can,False)
        LimitsWav(3,4, 0.2, 'LIN')
        TransFit('LOG',3.0,8.0)
        Sample_Trans = LoadRaw('54435.raw')
        Sample_Direct = LoadRaw('54433.raw')
        TransmissionSample(Sample_Trans, Sample_Direct, False)
        Can_Trans = LoadRaw('54434.raw')
        Can_Direct = LoadRaw('54433.raw')
        TransmissionCan(Can_Trans, Can_Direct, False)

        #run the reduction
        WavRangeReduction(3, 4, False, '_suff')

    def validate(self):
        self.disableChecking.append('SpectraMap')
        #when comparing LOQ files you seem to need the following
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '54431main_2D_3.0_4.0_suff','LOQTransFitWorkspace2D.nxs'

class LOQReductionOnLoadedWorkspaceMustProduceTheSameResult_1(stresstesting.MantidStressTest):
    """ It will repeat the test done at LOQCentreNoGrav but using 
    loaded workspaces
    """
    def runTest(self):
        config["default.instrument"] = "LOQ"
        LOQ()

        Set1D()
        Detector("rear-detector")
        MaskFile('MASK.094AA')
        Gravity(False)
        Sample = LoadRaw('54431.raw')
        Trans_Sample = LoadRaw('54435.raw')
        Trans_Direct = LoadRaw('54433.raw')
        Can = LoadRaw('54432.raw')
        CanTrans_Sample = LoadRaw('54434.raw')
        CanTrans_Direct = LoadRaw('54433.raw')
        
        AssignSample(Sample, False)
        TransmissionSample(Trans_Sample, Trans_Direct, False)
        AssignCan(Can, False)
        TransmissionCan(CanTrans_Sample, CanTrans_Direct, False)
        
        FindBeamCentre(60,200, 9)
        
        WavRangeReduction(3, 9, DefaultTrans)
    
    def validate(self):
        return '54431main_1D_3.0_9.0','LOQCentreNoGravSearchCentreFixed.nxs'

class LOQReductionOnLoadedWorkspaceMustProduceTheSameResult_2(stresstesting.MantidStressTest):
    """Before ticket #8461 test LOQReductionOnLoadedWorkspaceMustProduceTheSameResult_1 used
    to produce a workspace that matches LOQCentreNoGrav.nxs. This test is created to ensure
    that if we put the same centre that was produced before, we finish in the same result
    for the reduction"""
    def runTest(self):
        config["default.instrument"] = "LOQ"
        LOQ()

        Set1D()
        Detector("rear-detector")
        MaskFile('MASK.094AA')
        Gravity(False)
        Sample = LoadRaw('54431.raw')
        Trans_Sample = LoadRaw('54435.raw')
        Trans_Direct = LoadRaw('54433.raw')
        Can = LoadRaw('54432.raw')
        CanTrans_Sample = LoadRaw('54434.raw')
        CanTrans_Direct = LoadRaw('54433.raw')
        
        SetCentre(324.765, 327.670)

        AssignSample(Sample, False)
        TransmissionSample(Trans_Sample, Trans_Direct, False)
        AssignCan(Can, False)
        TransmissionCan(CanTrans_Sample, CanTrans_Direct, False)
        
        WavRangeReduction(3, 9, DefaultTrans)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map becauseit isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return '54431main_1D_3.0_9.0','LOQCentreNoGrav.nxs'
    

class SANSLOQCan2DReloadWorkspace(stresstesting.MantidStressTest):
    
  def runTest(self):
    config["default.instrument"] = "LOQ"          
    LOQ()
    Set2D()
    Detector("main-detector-bank")
    MaskFile('MASK.094AA')
    # apply some small artificial shift
    SetDetectorOffsets('REAR', -1.0, 1.0, 0.0, 0.0, 0.0, 0.0)    
    Gravity(True)
    sample = Load('99630')
    can = Load('99631')
    AssignSample(sample, False)
    AssignCan(can, False)
    
    WavRangeReduction(None, None, False)

    
  def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Instrument')
    #when comparing LOQ files you seem to need the following
    self.disableChecking.append('Axes')
    # the change in number is because the run number reported from 99630 is 53615
    return '53615main_2D_2.2_10.0','SANSLOQCan2D.nxs'

class SANS2DFrontNoGravReloadWorkspace(stresstesting.MantidStressTest):
    
  def runTest(self):
    config["default.instrument"] = "SANS2D"
    SANS2D()
    MaskFile('MASKSANS2D_094i_RKH.txt')
    SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
    SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
    Gravity(False)
    Set1D()
    Sample = LoadNexus('2500')
    AssignSample(Sample, False)
    WavRangeReduction(4.6, 12.85, False)

  def validate(self):
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Axes')
    self.disableChecking.append('Instrument')
    return '2500front_1D_4.6_12.85','SANS2DFrontNoGrav.nxs'

class SANS2DWaveloopsReloadWorkspace(stresstesting.MantidStressTest):
    
  def runTest(self):
    config["default.instrument"] = "SANS2D"
    SANS2D()
    MaskFile('MASKSANS2D.091A')
    Gravity(True)
    Set1D()
    s = Load('992')
    s_t = Load('988')
    direct = Load('987')
    direct_can = CloneWorkspace(direct)
    c = Load('993')
    c_t = Load('989')    
    AssignSample(s,False)
    TransmissionSample(s_t, direct, False)
    AssignCan(c, False)
    TransmissionCan(c_t, direct_can, False)

    CompWavRanges([3, 5, 7, 11], False)
    
  def validate(self):
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Axes')
    self.disableChecking.append('Instrument')
    # testing one of the workspaces that is produced, best not to choose the 
    # first one in produced by the loop as this is the least error prone
    return '992rear_1D_7.0_11.0','SANS2DWaveloops.nxs'


if __name__ == "__main__":
    unittest.main()
