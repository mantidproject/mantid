# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import CloneWorkspace, Load, LoadNexus, MoveInstrumentComponent, RenameWorkspace
from ISISCommandInterface import (
    AssignCan,
    AssignSample,
    CompWavRanges,
    Gravity,
    MaskFile,
    Reduce,
    ReductionSingleton,
    SANS2D,
    Set1D,
    SetDetectorOffsets,
    TransmissionCan,
    TransmissionSample,
    WavRangeReduction,
)
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


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DReductionShouldAcceptLoadedWorkspace(unittest.TestCase):
    """
    The following tests is to ensure that the reload obeys the following requirement:
     * If reload is True the real data will be always reloaded from the file
     * If reload is False, it will be used, if it pass the following tests:
       * The instrument components have not been moved
    """

    def setUp(self):
        self.load_run = "2500.nxs"
        config["default.instrument"] = "SANS2D"
        SANS2D()
        MaskFile("MASKSANS2D_094i_RKH.txt")
        self.control_name = "2500front_1D_4.6_12.85"
        self.inst_comp = "rear-detector"

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
    cl = SANS2DReductionShouldAcceptLoadedWorkspace

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


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DFrontNoGravReloadWorkspace(systemtesting.MantidSystemTest):
    def runTest(self):
        config["default.instrument"] = "SANS2D"
        SANS2D()
        MaskFile("MASKSANS2D_094i_RKH.txt")
        SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(False)
        Set1D()
        Sample = LoadNexus("2500")
        AssignSample(Sample, False)
        WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "2500front_1D_4.6_12.85", "SANS2DFrontNoGrav.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DWaveloopsReloadWorkspace(systemtesting.MantidSystemTest):
    def runTest(self):
        config["default.instrument"] = "SANS2D"
        SANS2D()
        MaskFile("MASKSANS2D.091A")
        Gravity(True)
        Set1D()
        s = Load("992")
        s_t = Load("988")
        direct = Load("987")
        direct_can = CloneWorkspace(direct)
        c = Load("993")
        c_t = Load("989")
        AssignSample(s, False)
        TransmissionSample(s_t, direct, False)
        AssignCan(c, False)
        TransmissionCan(c_t, direct_can, False)

        CompWavRanges([3, 5, 7, 11], False)

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        # testing one of the workspaces that is produced, best not to choose the
        # first one in produced by the loop as this is the least error prone
        return "992rear_1D_7.0_11.0", "SANS2DWaveloops.nxs"
