#pylint: disable=no-init,too-few-public-methods

# test batch mode with sans2d and selecting a period in batch mode
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
import stresstesting
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


class SANS2DMultiPeriodAddFiles(stresstesting.MantidStressTest):

    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        pass
        ii.SANS2D()
        ii.Set1D()
        ii.Detector("rear-detector")
        ii.MaskFile('MASKSANS2Doptions.091A')
        ii.Gravity(True)
        ii.add_runs( ('5512', '5512') ,'SANS2D', 'nxs', lowMem=True)

        #one period of a multi-period Nexus file
        ii.AssignSample('5512-add.nxs', period=7)

        ii.WavRangeReduction(2, 4, ii.DefaultTrans)
        paths = [os.path.join(config['defaultsave.directory'],'SANS2D00005512-add.nxs'),
                 os.path.join(config['defaultsave.directory'],'SANS2D00005512.log')]
        for path in paths:
            if os.path.exists(path):
                os.remove(path)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Axes')

        return '5512p7rear_1D_2.0_4.0Phi-45.0_45.0','SANS2DMultiPeriodAddFiles.nxs'


class LARMORMultiPeriodAddEventFiles(stresstesting.MantidStressTest):
    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        ii.LARMOR()
        ii.Set1D()
        ii.Detector("DetectorBench")
        ii.MaskFile('USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt')
        ii.Gravity(True)
        ii.add_runs( ('13065', '13065') ,'LARMOR', 'nxs', lowMem=True)

        ii.AssignSample('13065-add.nxs')
        ii.WavRangeReduction(2, 4, ii.DefaultTrans)

        # Clean up
        to_clean = ["13065_sans_nxs",
                    "13065p1rear_1D_2.0_4.0_incident_monitor",
                    "13065p2rear_1D_2.0_4.0_incident_monitor",
                    "13065p3rear_1D_2.0_4.0_incident_monitor",
                    "13065p4rear_1D_2.0_4.0_incident_monitor",
                    "80tubeCalibration_1-05-2015_r3157-3160"]
        for workspace in to_clean:
            DeleteWorkspace(workspace)

        paths = [os.path.join(config['defaultsave.directory'],'LARMOR00013065-add.nxs'),
                 os.path.join(config['defaultsave.directory'],'SANS2D00013065.log')]  # noqa
        for path in paths:
            if os.path.exists(path):
                os.remove(path)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Axes')

        return "13065p1rear_1D_2.0_4.0" , "LARMORMultiPeriodAddEventFiles.nxs"


class SANS2DMultiPeriodAddFiles_V2(stresstesting.MantidStressTest):

    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.Set1D()
        ii2.Detector("rear-detector")
        ii2.MaskFile('MASKSANS2Doptions.091A')
        ii2.Gravity(True)
        ii2.AddRuns(('5512', '5512'), 'SANS2D', 'nxs', lowMem=True)

        # one period of a multi-period Nexus file
        ii2.AssignSample('5512-add.nxs', period=7)

        ii2.WavRangeReduction(2, 4, ii2.DefaultTrans)
        paths = [os.path.join(config['defaultsave.directory'], 'SANS2D00005512-add.nxs'),
                 os.path.join(config['defaultsave.directory'], 'SANS2D00005512.log')]
        for path in paths:
            if os.path.exists(path):
                os.remove(path)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Axes')

        return '5512p7rear_1D_2.0_4.0Phi-45.0_45.0', 'SANS2DMultiPeriodAddFiles.nxs'


class LARMORMultiPeriodAddEventFilesTest_V2(stresstesting.MantidStressTest):
    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.LARMOR()
        ii2.Set1D()
        ii2.Detector("DetectorBench")
        ii2.MaskFile('USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt')
        ii2.Gravity(True)
        ii2.AddRuns(('13065', '13065'), 'LARMOR', 'nxs', lowMem=True)

        ii2.AssignSample('13065-add.nxs')
        ii2.WavRangeReduction(2, 4, ii2.DefaultTrans)

        # Clean up
        for element in AnalysisDataService.getObjectNames():
            if AnalysisDataService.doesExist(element) and element != "13065p1rear_1D_2.0_4.0":
                AnalysisDataService.remove(element)

        paths = [os.path.join(config['defaultsave.directory'], 'LARMOR00013065-add.nxs'),
                 os.path.join(config['defaultsave.directory'], 'SANS2D00013065.log')]  # noqa
        for path in paths:
            if os.path.exists(path):
                os.remove(path)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Axes')

        return "13065p1rear_1D_2.0_4.0", "LARMORMultiPeriodAddEventFiles.nxs"
