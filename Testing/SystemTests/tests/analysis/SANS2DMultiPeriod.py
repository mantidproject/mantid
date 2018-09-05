#pylint: disable=no-init,too-few-public-methods

# test batch mode with sans2d and selecting a period in batch mode
from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.api import *
import ISISCommandInterface as ii
import SANSBatchMode as bm
import sans.command_interface.ISISCommandInterface as ii2


class SANS2DMultiPeriodSingle(stresstesting.MantidStressTest):

    reduced=''

    def runTest(self):

        ii.SANS2D()
        ii.Set1D()
        ii.Detector("rear-detector")
        ii.MaskFile('MASKSANS2Doptions.091A')
        ii.Gravity(True)

        ii.AssignSample('5512')
        self.reduced = ii.WavRangeReduction()

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return mtd[self.reduced][6].name(),'SANS2DBatch.nxs'


class SANS2DMultiPeriodBatch(SANS2DMultiPeriodSingle):

    def runTest(self):

        ii.SANS2D()
        ii.Set1D()
        ii.Detector("rear-detector")
        ii.MaskFile('MASKSANS2Doptions.091A')
        ii.Gravity(True)

        csv_file = FileFinder.getFullPath('SANS2D_multiPeriodTests.csv')

        bm.BatchReduce(csv_file, 'nxs', saveAlgs={})
        self.reduced = '5512_SANS2DBatch'


class LARMORMultiPeriodEventModeLoading(stresstesting.MantidStressTest):
    """
    This test checks if the positioning of all workspaces of a
    multi-period event-type file are the same.
    """
    def __init__(self):
        super(LARMORMultiPeriodEventModeLoading, self).__init__()
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
            self.assertEqual(position, reference_position)
            self.assertEqual(rotation, reference_rotation)

    def runTest(self):
        ii.LARMOR()
        ii.Set1D()
        ii.Detector("DetectorBench")
        ii.MaskFile('USER_Larmor_163F_HePATest_r13038.txt')
        ii.AssignSample('13038')
        base_name = "13038_sans_nxs_"
        number_of_workspaces = 4
        self._check_if_all_multi_period_workspaces_have_the_same_position(base_name, number_of_workspaces)
        self._clean_up(base_name, number_of_workspaces)

    def validate(self):
        return self.success


class SANS2DMultiPeriodSingleTest_V2(stresstesting.MantidStressTest):

    reduced = ''

    def runTest(self):
        pass
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.Set1D()
        ii2.Detector("rear-detector")
        ii2.MaskFile('MASKSANS2Doptions.091A')
        ii2.Gravity(True)

        ii2.AssignSample('5512')
        self.reduced = ii2.WavRangeReduction()

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
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.Set1D()
        ii2.Detector("rear-detector")
        ii2.MaskFile('MASKSANS2Doptions.091A')
        ii2.Gravity(True)

        csv_file = FileFinder.getFullPath('SANS2D_multiPeriodTests.csv')

        ii2.BatchReduce(csv_file, 'nxs', saveAlgs={})
        self.reduced = '5512_SANS2DBatch'


class LARMORMultiPeriodEventModeLoadingTest_V2(stresstesting.MantidStressTest):
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
        ii2.LARMOR()
        ii2.Set1D()
        ii2.Detector("DetectorBench")
        ii2.MaskFile('USER_Larmor_163F_HePATest_r13038.txt')
        ii2.AssignSample('13038')
        # Different in V2. We need to call the reduction in order to load the data. Hence we add the
        # WaveRangeReduction here.
        ii2.WavRangeReduction()
        base_name = "13038_sans_nxs_"
        number_of_workspaces = 4
        self._check_if_all_multi_period_workspaces_have_the_same_position(base_name, number_of_workspaces)
        self._clean_up(base_name, number_of_workspaces)

    def validate(self):
        return self.success
