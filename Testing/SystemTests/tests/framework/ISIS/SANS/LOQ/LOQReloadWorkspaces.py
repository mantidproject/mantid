# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from ISISCommandInterface import *
from mantid.simpleapi import LoadRaw, MoveInstrumentComponent
import unittest

from sans.common.enums import SANSInstrument

"""
Allowing the reduction to use already loaded workspace will make it easier to
deal with event mode and producing new workspaces for the reduction of data.
Till 06/2013 the reload option was available, but not implemented.

In order to protect the system, it is suggested the following integration tests
to ensure that allowing workspaces as input to the reduction will not disturb the
reduction itself, and it is safe.

SANSLOQReductionShouldAcceptLoadedWorkspace ensure some requirements for the reloading.
SANS2DReductionShouldAcceptLoadedWorkspace and SANS2DReductionShouldAcceptLoadedWorkspaceRawFile
apply the same requirements for SANS2D instruments.


SANSLOQReductionShouldAcceptLoadedWorkspaceSystemTest, SANS2DReductionShouldAcceptLoadedWorkspaceSystemTest
and SANS2DReductionShouldAcceptLoadedWorkspace are wrappers to make unittest.TestCase to fit the systemtesting
framework.

The other tests are here to ensure the results of providing directly workspaces will be the same that loading
from files.

"""


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQReductionShouldAcceptLoadedWorkspace(unittest.TestCase):
    """
    The following tests is to ensure that the reload obeys the following requirement:
     * If reload is True the real data will be always reloaded from the file
     * If reload is False, it will be used, if it pass the following tests:
       * The instrument components have not been moved
    """

    def setUp(self):
        self.load_run = "54431.raw"
        config["default.instrument"] = "LOQ"
        LOQ()
        MaskFile("MASK.094AA")
        self.control_name = "54431main_1D_2.2_10.0"
        self.inst_comp = "main-detector-bank"

    def tearDown(self):
        mtd.clear()

    def test_accept_loaded_workspace_only_if_reload_false(self):
        my_workspace = Load(self.load_run)
        # set the value for my_workspace to ensure it is the one used
        aux = my_workspace.dataY(0)
        aux[10] = 5
        my_workspace.setY(0, aux)
        # ask to use the loaded workspace
        AssignSample(my_workspace, reload=False)

        ws_name = ReductionSingleton().get_sample().get_wksp_name()

        self.assertTrue(ws_name, my_workspace.name())

        self.assertTrue(my_workspace.dataY(0)[10], 5)
        # ensure that it is able to execute the reduction
        Reduce()
        self.assertTrue(self.control_name in mtd)

    def test_accept_loaded_workspace_but_reload_the_data_file_if_reload_true(self):
        my_workspace = Load(self.load_run)
        # set the value for my_workspace to ensure it is the one used
        aux = my_workspace.dataY(0)
        aux[10] = 5
        my_workspace.setY(0, aux)
        # ask to use the loaded workspace
        AssignSample(my_workspace, reload=True)

        ws_name = ReductionSingleton().get_sample().get_wksp_name()
        # it is different, because, it will compose the name using its rule,
        # which, for sure, will be different of my_workspace.
        self.assertNotEqual(ws_name, my_workspace.name())
        self.assertNotEqual(mtd[ws_name].dataY(0)[10], 5)
        # it is not necessary to ensure the Reduce occurs

    def test_should_not_accept_loaded_workspace_if_moved(self):
        my_workspace = Load(self.load_run)
        MoveInstrumentComponent(my_workspace, self.inst_comp, X=2, Y=1, Z=0)
        ## attempt to use a workspace that has been moved
        self.assertRaises(RuntimeError, AssignSample, my_workspace, False)

    def test_should_not_accept_loaded_workspace_if_moved_2(self):
        # assign sample loads and move the workspace to the defined center
        AssignSample(self.load_run)

        # this makes it load this worksapce and generates an output workspace
        ws_name = ReductionSingleton().get_sample().get_wksp_name()
        # the workspace is renamed, so it seems another workspace
        my_workspace = RenameWorkspace(ws_name)
        ## trying to assign it again to AssingSample must fail
        self.assertRaises(RuntimeError, AssignSample, my_workspace, False)


class SANSLOQReductionShouldAcceptLoadedWorkspaceSystemTest(systemtesting.MantidSystemTest):
    cl = SANSLOQReductionShouldAcceptLoadedWorkspace

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(self.cl, "test"))
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def validate(self):
        return self._success


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQTransFitWorkspace2DWithLoadedWorkspace(systemtesting.MantidSystemTest):
    def runTest(self):
        config["default.instrument"] = "LOQ"
        LOQ()
        MaskFile("MASK.094AA")
        Gravity(False)
        Set2D()
        Detector("main-detector-bank")
        Sample = LoadRaw("54431.raw")
        AssignSample(Sample, False)
        Can = LoadRaw("54432.raw")
        AssignCan(Can, False)
        LimitsWav(3, 4, 0.2, "LIN")
        TransFit("LOG", 3.0, 8.0)
        Sample_Trans = LoadRaw("54435.raw")
        Sample_Direct = LoadRaw("54433.raw")
        TransmissionSample(Sample_Trans, Sample_Direct, False)
        Can_Trans = LoadRaw("54434.raw")
        Can_Direct = LoadRaw("54433.raw")
        TransmissionCan(Can_Trans, Can_Direct, False)

        # run the reduction
        WavRangeReduction(3, 4, False, "_suff")

    def validate(self):
        self.disableChecking.append("SpectraMap")
        # when comparing LOQ files you seem to need the following
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "54431main_2D_3.0_4.0_suff", "LOQTransFitWorkspace2D.nxs"


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQReductionOnLoadedWorkspaceMustProduceTheSameResult_1(systemtesting.MantidSystemTest):
    """It will repeat the test done at SANSLOQCentreNoGrav but using
    loaded workspaces
    """

    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        config["default.instrument"] = "LOQ"
        LOQ()

        Set1D()
        Detector("rear-detector")
        MaskFile("MASK.094AA")
        Gravity(False)
        Sample = LoadRaw("54431.raw")
        Trans_Sample = LoadRaw("54435.raw")
        Trans_Direct = LoadRaw("54433.raw")
        Can = LoadRaw("54432.raw")
        CanTrans_Sample = LoadRaw("54434.raw")
        CanTrans_Direct = LoadRaw("54433.raw")

        AssignSample(Sample, False)
        TransmissionSample(Trans_Sample, Trans_Direct, False)
        AssignCan(Can, False)
        TransmissionCan(CanTrans_Sample, CanTrans_Direct, False)

        FindBeamCentre(60, 200, 9)

        WavRangeReduction(3, 9, DefaultTrans)

    def validate(self):
        self.disableChecking.append("Instrument")
        return "54431main_1D_3.0_9.0", "LOQCentreNoGravSearchCentreFixed.nxs"


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQReductionOnLoadedWorkspaceMustProduceTheSameResult_2(systemtesting.MantidSystemTest):
    """Before ticket #8461 test SANSLOQReductionOnLoadedWorkspaceMustProduceTheSameResult_1 used
    to produce a workspace that matches SANSLOQCentreNoGrav.nxs. This test is created to ensure
    that if we put the same centre that was produced before, we finish in the same result
    for the reduction"""

    def runTest(self):
        config["default.instrument"] = "LOQ"
        LOQ()

        Set1D()
        Detector("rear-detector")
        MaskFile("MASK.094AA")
        Gravity(False)
        Sample = LoadRaw("54431.raw")
        Trans_Sample = LoadRaw("54435.raw")
        Trans_Direct = LoadRaw("54433.raw")
        Can = LoadRaw("54432.raw")
        CanTrans_Sample = LoadRaw("54434.raw")
        CanTrans_Direct = LoadRaw("54433.raw")

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
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")

        return "54431main_1D_3.0_9.0", "LOQCentreNoGrav_V2.nxs"


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQCan2DReloadWorkspace(systemtesting.MantidSystemTest):
    def runTest(self):
        config["default.instrument"] = "LOQ"
        LOQ()
        Set2D()
        Detector("main-detector-bank")
        MaskFile("MASK.094AA")
        # apply some small artificial shift
        SetDetectorOffsets("REAR", -1.0, 1.0, 0.0, 0.0, 0.0, 0.0)
        Gravity(True)
        sample = Load("99630")
        can = Load("99631")
        AssignSample(sample, False)
        AssignCan(can, False)

        WavRangeReduction(None, None, False)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        # when comparing LOQ files you seem to need the following
        self.disableChecking.append("Axes")
        # the change in number is because the run number reported from 99630 is 53615
        return "53615main_2D_2.2_10.0", "SANSLOQCan2D.nxs"
