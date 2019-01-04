# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,too-few-public-methods

from __future__ import (absolute_import, division, print_function)
import systemtesting
import mantid  # noqa

from mantid.api import (AnalysisDataService, FileFinder)
from sans.command_interface.ISISCommandInterface import (SANS2D, Set1D, Detector, MaskFile, Gravity,
                                                         UseCompatibilityMode, AssignSample, WavRangeReduction,
                                                         LARMOR, BatchReduce)

# test batch mode with sans2d and selecting a period in batch mode


class SANS2DMultiPeriodSingleTest_V2(systemtesting.MantidSystemTest):

    reduced = ''

    def runTest(self):
        pass
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile('MASKSANS2Doptions.091A')
        Gravity(True)

        AssignSample('5512')
        self.reduced = WavRangeReduction()

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return AnalysisDataService[self.reduced][6].name(),'SANS2DBatch.nxs'


class SANS2DMultiPeriodBatchTest_V2(SANS2DMultiPeriodSingleTest_V2):

    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile('MASKSANS2Doptions.091A')
        Gravity(True)

        csv_file = FileFinder.getFullPath('SANS2D_multiPeriodTests.csv')

        BatchReduce(csv_file, 'nxs', saveAlgs={})
        self.reduced = '5512_SANS2DBatch'


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
        MaskFile('USER_Larmor_163F_HePATest_r13038.txt')
        AssignSample('13038')
        # Different in V2. We need to call the reduction in order to load the data. Hence we add the
        # WaveRangeReduction here.
        WavRangeReduction()
        base_name = "13038_sans_nxs_"
        number_of_workspaces = 4
        self._check_if_all_multi_period_workspaces_have_the_same_position(base_name, number_of_workspaces)
        self._clean_up(base_name, number_of_workspaces)

    def validate(self):
        return self.success
