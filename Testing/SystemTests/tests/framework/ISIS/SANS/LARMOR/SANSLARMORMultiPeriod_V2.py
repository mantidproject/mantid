# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-few-public-methods

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantid.api import AnalysisDataService
from sans.command_interface.ISISCommandInterface import Set1D, Detector, MaskFile, AssignSample, WavRangeReduction, LARMOR

# test batch mode with sans2d and selecting a period in batch mode
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LARMOR)
class LARMORMultiPeriodEventModeLoadingTest_V2(systemtesting.MantidSystemTest):
    """
    This test checks if the positioning of all workspaces of a
    multi-period event-type file are the same.
    """

    def __init__(self):
        super(LARMORMultiPeriodEventModeLoadingTest_V2, self).__init__()
        self.success = True

    def _get_position_and_rotation(self, workspace):
        instrument = workspace.getInstrument()
        component = instrument.getComponentByName("DetectorBench")
        position = component.getPos()
        rotation = component.getRotation()
        return position, rotation

    def _clean_up(self, base_name, number_of_workspaces):
        for index in range(1, number_of_workspaces + 1):
            workspace_name = base_name + str(index)
            monitor_name = workspace_name + "_monitors"
            AnalysisDataService.remove(workspace_name)
            AnalysisDataService.remove(monitor_name)
        AnalysisDataService.remove("80tubeCalibration_18-04-2016_r9330-9335")

    def _check_if_all_multi_period_workspaces_have_the_same_position(self, base_name, number_of_workspaces):
        reference_name = base_name + str(1)
        reference_workspace = AnalysisDataService.retrieve(reference_name)
        reference_position, reference_rotation = self._get_position_and_rotation(reference_workspace)
        for index in range(2, number_of_workspaces + 1):
            ws_name = base_name + str(index)
            workspace = AnalysisDataService.retrieve(ws_name)
            position, rotation = self._get_position_and_rotation(workspace)
            if position != reference_position or rotation != reference_rotation:
                self.success = False

    def runTest(self):
        LARMOR()
        Set1D()
        Detector("DetectorBench")
        MaskFile("USER_Larmor_163F_HePATest_r13038.txt")
        AssignSample("13038")
        # Different in V2. We need to call the reduction in order to load the data. Hence we add the
        # WaveRangeReduction here.
        WavRangeReduction()
        base_name = "13038_sans_nxs_"
        number_of_workspaces = 4
        self._check_if_all_multi_period_workspaces_have_the_same_position(base_name, number_of_workspaces)
        self._clean_up(base_name, number_of_workspaces)

    def validate(self):
        return self.success
