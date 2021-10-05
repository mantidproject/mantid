# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.drill.model.DrillModel import DrillModel


class DrillModelTest(unittest.TestCase):

    def setUp(self):
        patch = mock.patch(
                'mantidqtinterfaces.drill.model.DrillModel.DrillAlgorithmPool'
                )
        self.mTasksPool = patch.start()
        self.addCleanup(patch.stop)
        self.mTasksPool = self.mTasksPool.return_value

        patch = mock.patch("mantidqtinterfaces.drill.model.DrillModel.config")
        self.mConfig = patch.start()
        config = {"default.facility": "ILL"}
        self.addCleanup(patch.stop)
        self.mConfig.__getitem__.side_effect = config.__getitem__
        # self.mConfig.__getitem__.return_value = "ILL"

        self.model = DrillModel()

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.RundexSettings")
    def test_setInstrument(self, mSettings):
        mSettings.ACQUISITION_MODES = {"i1": ["a1", "a2"]}
        self.model.setAcquisitionMode = mock.Mock()
        # invalid instrument
        self.model.setInstrument("test")
        self.assertIsNone(self.model.instrument)
        self.assertIsNone(self.model.acquisitionMode)
        self.assertIsNone(self.model.algorithm)
        self.model.setAcquisitionMode.assert_not_called()
        # valid instrument
        self.model.setInstrument("i1")
        self.assertEqual(self.model.instrument, "i1")
        self.model.setAcquisitionMode.assert_called_once_with("a1")

    def test_getInstrument(self):
        self.assertEqual(self.model.getInstrument(), self.model.instrument)

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.RundexSettings")
    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillExportModel")
    def test_setAcquisitionMode(self, mExport, mSettings):
        mSettings.ACQUISITION_MODES = {"i1": ["a1", "a2"]}
        mSettings.THREADS_NUMBER = {"a1": 10, "a2": 20}
        self.model._initController = mock.Mock()
        self.model._initProcessingParameters = mock.Mock()
        # invalid aquisition mode
        self.model.instrument = None
        self.model.setAcquisitionMode("a1")
        self.assertIsNone(self.model.acquisitionMode)
        self.model.instrument = "i1"
        self.model.setAcquisitionMode("test_invalid")
        self.assertIsNone(self.model.acquisitionMode)
        # valide acquisition mode
        self.model.setAcquisitionMode("a2")
        self.assertEqual(self.model.acquisitionMode, "a2")
        self.mTasksPool.setMaxThreadCount.assert_called_once_with(20)

    def test_getAcquisitionMode(self):
        self.model.acquisitionMode = "test"
        acquisitionMode = self.model.getAcquisitionMode()
        self.assertEqual(acquisitionMode, "test")

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.RundexSettings")
    def test_getAvailableAcquisitionModes(self, mSettings):
        mSettings.ACQUISITION_MODES = {"i1": ["a1", "a2"]}
        self.model.instrument = "i1"
        self.assertEqual(self.model.getAvailableAcquisitionModes(),
                         ["a1", "a2"])
        self.model.instrument = None
        self.assertEqual(self.model.getAvailableAcquisitionModes(), [])

    def test_getExportModel(self):
        self.model.exportModel = mock.Mock()
        exportModel = self.model.getExportModel()
        self.assertEqual(exportModel, self.model.exportModel)

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.ConfigService")
    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.sys")
    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.os.path")
    def test_setCycleAndExperiment(self, mPath, mSys, mConfig):
        self.model.instrument = "i1"
        mSys.platform = "linux"
        mPath.isDir.return_value = True
        self.model.setCycleAndExperiment("test1", "test2")
        self.assertEqual(self.model.cycleNumber, "test1")
        self.assertEqual(self.model.experimentId, "test2")
        mConfig.appendDataSearchDir.assert_called()

    def test_getCycleAndExperiment(self):
        self.model.cycleNumber = "test1"
        self.model.experimentId = "test2"
        c, e = self.model.getCycleAndExperiment()
        self.assertEqual(c, "test1")
        self.assertEqual(e, "test2")

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillParameterController")
    def test_initController(self, mController):
        self.model.algorithm = None
        self.model._initController()
        mController.assert_not_called()
        self.model.algorithm = "a1"
        self.model._initController()
        mController.assert_called_once_with("a1")

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillParameter")
    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.sapi")
    def test_initProcessingParameters(self, mSapi, mParam):
        p1 = mock.Mock()
        p1.name = "p1"
        p2 = mock.Mock()
        p2.name = "p2"
        alg = mock.Mock()
        alg.getProperties.return_value = [p1, p2]
        mSapi.AlgorithmManager.createUnmanaged.return_value = alg
        self.model.algorithm = "test_alg"
        self.model.controller = "test_controller"
        self.model._initProcessingParameters()
        mSapi.AlgorithmManager.createUnmanaged \
            .assert_called_once_with("test_alg")
        calls = [mock.call("p1"), mock.call().setController("test_controller"),
                 mock.call().initFromProperty(p1),
                 mock.call("p2"), mock.call().setController("test_controller"),
                 mock.call().initFromProperty(p2)]
        mParam.assert_has_calls(calls)

    def test_getParameters(self):
        self.model._parameters = ["p1", "p2"]
        params = self.model.getParameters()
        self.assertEqual(params, ["p1", "p2"])

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillSampleGroup")
    def test_groupSamples(self, mGroup):
        s0 = mock.Mock()
        s0.getGroup.return_value = None
        s1 = mock.Mock()
        s1.getGroup.return_value = None
        s2 = mock.Mock()
        s2.getGroup.return_value = None
        s3 = mock.Mock()
        s3.getGroup.return_value = None
        self.model._samples = [s0, s1, s2, s3]
        self.model.groupSamples([0, 1, 3])
        mGroup.return_value.setName.assert_called_once_with("A")
        calls = [mock.call(s0), mock.call(s1), mock.call(s3)]
        mGroup.return_value.addSample.assert_has_calls(calls)

    def test_ungroupSamples(self):
        g1 = mock.Mock()
        g1.isEmpty.return_value = False
        g2 = mock.Mock()
        g2.isEmpty.return_value = False
        s0 = mock.Mock()
        s0.getGroup.return_value = g1
        s1 = mock.Mock()
        s1.getGroup.return_value = g1
        s2 = mock.Mock()
        s2.getGroup.return_value = g2
        s3 = mock.Mock()
        s3.getGroup.return_value = None
        self.model._samples = [s0, s1, s2, s3]
        self.model._sampleGroups = [g1, g2]
        self.model.ungroupSamples([0])
        g1.delSample.assert_called_once_with(s0)
        g1.reset_mock()
        self.model.ungroupSamples([1, 2])
        g1.delSample.assert_called_once_with(s1)
        g2.delSample.assert_called_once_with(s2)

    def test_addToGroup(self):
        g1 = mock.Mock()
        g1.getName.return_value = "A"
        g2 = mock.Mock()
        g2.getName.return_value = "B"
        s0 = mock.Mock()
        s0.getGroup.return_value = g1
        s1 = mock.Mock()
        s1.getGroup.return_value = g1
        s2 = mock.Mock()
        s2.getGroup.return_value = g2
        self.model._samples = [s0, s1, s2]
        self.model._sampleGroups = {"A": g1, "B": g2}
        self.model.addToGroup([2], "A")
        g2.delSample.assert_called_once_with(s2)
        g1.addSample.assert_called_once_with(s2)

    def test_setGroupMaster(self):
        g1 = mock.Mock()
        s0 = mock.Mock()
        s0.getGroup.return_value = g1
        s1 = mock.Mock()
        s1.getGroup.return_value = g1
        self.model._samples = [s0, s1]
        self.model.setGroupMaster(0, True)
        g1.setMaster.assert_called_once_with(s0)
        g1.reset_mock()
        self.model.setGroupMaster(1, True)
        g1.setMaster.assert_called_once_with(s1)
        g1.reset_mock()
        self.model.setGroupMaster(1, False)
        g1.unsetMaster.assert_called_once()

    def test_getProcessingParameters(self):
        g1 = mock.Mock()
        s1 = mock.Mock()
        s1.getParameterValues.return_value = {"p1": "v1", "p2": "v2"}
        s1.getGroup.return_value = g1
        s2 = mock.Mock()
        s2.getParameterValues.return_value = {}
        s2.getGroup.return_value = g1
        s3 = mock.Mock()
        s3.getParameterValues.return_value = {"p2": "v2'"}
        s3.getGroup.return_value = g1
        s4 = mock.Mock()
        s4.getParameterValues.return_value = {"p2": "DEFAULT"}
        s4.getGroup.return_value = g1
        g1.getMaster.return_value = s1
        self.model._samples = [s1, s2, s3, s4]
        params = {"p2": "value2", "p3": "value3"}
        p2 = mock.Mock()
        p2.getName.return_value = "p2"
        p2.getValue.return_value = "value2"
        p3 = mock.Mock()
        p3.getName.return_value = "p3"
        p3.getValue.return_value = "value3"
        self.model._parameters = [p2, p3]
        params.update({"p1": "v1", "p2": "v2"})
        params["OutputWorkspace"] = "sample_1"
        self.assertDictEqual(self.model.getProcessingParameters(0), params)

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillTask")
    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.RundexSettings")
    def test_processGroupByGroup(self, mSettings, mTask):
        mSettings.GROUPED_COLUMNS = {"a1": ["p1", "p2"]}
        self.model.acquisitionMode = "a1"
        self.model.algorithm = "algo"
        self.model._parameters = []
        g0 = mock.Mock()
        g0.getName.return_value = "g0"
        g1 = mock.Mock()
        g1.getName.return_value = "g1"
        s0 = mock.Mock()
        s0.getGroup.return_value = g0
        s0.getParameterValues.return_value = {"p1": "v0", "p2": "v0",
                                              "p3": "v0"}
        s1 = mock.Mock()
        s1.getGroup.return_value = g0
        s1.getParameterValues.return_value = {"p1": "v1", "p2": "v1",
                                              "p3": "v1"}
        s2 = mock.Mock()
        s2.getGroup.return_value = g1
        s2.getParameterValues.return_value = {"p1": "v2", "p2": "v2",
                                              "p3": "v2"}
        s3 = mock.Mock()
        s3.getGroup.return_value = None
        s3.getParameterValues.return_value = {"p1": "v3", "p2": "v3",
                                              "p3": "v3"}
        g0.getSamples.return_value = [s0, s1]
        g0.getMaster.return_value = s0
        g1.getSamples.return_value = [s2]
        g1.getMaster.return_value = None
        self.model._samples = [s0, s1, s2, s3]
        self.assertFalse(self.model.processGroupByGroup([3]))
        self.assertTrue(self.model.processGroupByGroup([0, 1, 2, 3]))
        calls = [mock.call("g0", "algo", p1='v0,v1', p2='v0,v1', p3="v0"),
                 mock.call("g1", "algo", p1='v2', p2='v2', p3="v2")]
        mTask.assert_has_calls(calls, any_order=True)
        self.model.tasksPool.addProcesses.assert_called_once()

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillTask")
    def test_process(self, mTask):
        self.model.getProcessingParameters = mock.Mock()
        self.model.getProcessingParameters.return_value = {}
        s0 = mock.Mock()
        self.model._samples = [s0]
        self.model.process([0])
        self.model.getProcessingParameters.assert_called_once()
        mTask.assert_called()
        self.model.tasksPool.addProcesses.assert_called_once()
        self.model.tasksPool.reset_mock()
        mTask.reset_mock()
        self.model.process([2])
        mTask.assert_not_called()
        self.model.tasksPool.addProcesses.assert_called_once_with([])

    def test_onProcessingProgress(self):
        self.model.progressUpdate = mock.Mock()
        self.model._onProcessingProgress(0)
        self.model.progressUpdate.emit.assert_called_once()

    def test_onProcessingDone(self):
        self.model.processingDone = mock.Mock()
        self.model._onProcessingDone()
        self.model.processingDone.emit.assert_called_once()

    def test_stopProcess(self):
        self.model.stopProcess()
        self.model.tasksPool.abortProcessing.assert_called_once()

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillRundexIO")
    def test_setIOFile(self, mRundexIO):
        self.model.setIOFile("test")
        mRundexIO.assert_called_once_with("test", self.model)

    def test_resetIOFile(self):
        self.model.rundexIO = "test"
        self.model.resetIOFile()
        self.assertIsNone(self.model.rundexIO)

    def test_getIOFile(self):
        self.model.rundexIO = mock.Mock()
        self.model.rundexIO.getFilename.return_value = "test"
        self.assertEqual(self.model.getIOFile(), "test")

    def test_importRundexData(self):
        self.model.rundexIO = None
        self.model.importRundexData()
        self.model.rundexIO = mock.Mock()
        self.model.importRundexData()
        self.model.rundexIO.load.assert_called_once()

    def test_exportRundexData(self):
        self.model.rundexIO = None
        self.model.exportRundexData()
        self.model.rundexIO = mock.Mock()
        self.model.exportRundexData()
        self.model.rundexIO.save.assert_called_once()

    def test_setVisualSettings(self):
        visualSettings = {"s1": "v1", "s2": "v2"}
        self.model.setVisualSettings(visualSettings)
        self.assertDictEqual(self.model.visualSettings,
                             {"s1": "v1", "s2": "v2"})

    def test_getVisualSettings(self):
        self.model.visualSettings = {"test": "test"}
        self.assertEqual(self.model.getVisualSettings(), {"test": "test"})

    @mock.patch("mantidqtinterfaces.drill.model.DrillModel.DrillSample")
    def test_addSample(self, mSample):
        self.model.newSample = mock.Mock()
        self.assertEqual(self.model._samples, [])
        self.model.addSample(0)
        mSample.assert_called_once()
        self.model.newSample.emit.assert_called_once()
        self.assertEqual(self.model._samples, [mSample.return_value])

    def test_deleteSample(self):
        g1 = mock.Mock()
        g1.getName.return_value = "A"
        s1 = mock.Mock()
        s1.getGroup.return_value = g1
        s2 = mock.Mock()
        s2.getGroup.return_value = g1
        s3 = mock.Mock()
        self.model._samples = [s1, s2, s3]
        self.model._sampleGroups = {"A": g1}
        g1.isEmpty.return_value = False
        self.model.deleteSample(0)
        self.assertEqual(self.model._samples, [s2, s3])
        self.assertEqual(self.model._sampleGroups, {"A": g1})
        g1.isEmpty.return_value = True
        self.model.deleteSample(0)
        self.assertEqual(self.model._samples, [s3])
        self.assertEqual(self.model._sampleGroups, {})

    def test_getSamples(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        self.model._samples = [s0, s1]
        self.assertEqual(self.model.getSamples(), [s0, s1])


if __name__ == "__main__":
    unittest.main()
