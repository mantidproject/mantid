# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantid.kernel import *
from mantid.api import *

from Interface.ui.drill.model.DrillModel import DrillModel


class DrillModelTest(unittest.TestCase):

    TECHNIQUE = {"i1": "t1"}

    ACQUISITION_MODES = {"i1": ["a1", "a2"]}

    COLUMNS = {"a1": ["c1", "c2", "CustomOptions"], "a2": ["c3", "c4"]}

    ALGORITHM = {"a1": "a1", "a2": "a2"}

    EXPORT_ALGORITHMS = {"a1": {"ea1": True}, "a2": {"ea2": True}}

    SETTINGS = {"a1": ["str", "int", "float", "bool"], "a2": ["test"]}

    def setUp(self):
        # mock sapi
        patch = mock.patch('Interface.ui.drill.model.DrillModel.sapi')
        self.mSapi = patch.start()
        self.addCleanup(patch.stop)

        # mock properties
        self.mSapi.AlgorithmManager.createUnmanaged.return_value.getProperty \
            .return_value.value = "test"

        # mock parameter controller
        patch = mock.patch('Interface.ui.drill.model.DrillModel.DrillParameterController')
        self.mController = patch.start()
        self.addCleanup(patch.stop)

        # mock config
        d = {'default.instrument': 'i1', 'default.facility': 'ILL'}
        patch = mock.patch('Interface.ui.drill.model.DrillModel.config')
        self.mConfig = patch.start()
        self.mConfig.__getitem__.side_effect = d.__getitem__
        self.addCleanup(patch.stop)

        # mock logger
        patch = mock.patch('Interface.ui.drill.model.DrillModel.logger')
        self.mLogger = patch.start()
        self.addCleanup(patch.stop)

        # mock specifications
        patch = mock.patch.dict('Interface.ui.drill.model.DrillModel.RundexSettings.TECHNIQUE',
                                self.TECHNIQUE,
                                clear=True)
        self.mTechniques = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict('Interface.ui.drill.model.DrillModel.RundexSettings.ACQUISITION_MODES',
                                self.ACQUISITION_MODES,
                                clear=True)
        self.mAcq = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict('Interface.ui.drill.model.DrillModel.RundexSettings.COLUMNS', self.COLUMNS, clear=True)
        self.mColumns = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict('Interface.ui.drill.model.DrillModel.RundexSettings.ALGORITHM',
                                self.ALGORITHM,
                                clear=True)
        self.mAlgo = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict('Interface.ui.drill.model.DrillModel.RundexSettings.EXPORT_ALGORITHMS',
                                self.EXPORT_ALGORITHMS,
                                clear=True)
        self.mAlgo = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict('Interface.ui.drill.model.DrillModel.RundexSettings.SETTINGS',
                                self.SETTINGS,
                                clear=True)
        self.mSettings = patch.start()
        self.addCleanup(patch.stop)

        # mock drill task
        patch = mock.patch('Interface.ui.drill.model.DrillModel.DrillTask')
        self.mDrillTask = patch.start()
        self.addCleanup(patch.stop)

        # mock drill tasks pool
        patch = mock.patch('Interface.ui.drill.model.DrillModel.DrillAlgorithmPool')
        self.mTasksPool = patch.start()
        self.addCleanup(patch.stop)

        self.model = DrillModel()

    def test_setInstrument(self):
        # invalid instrument
        self.mController.reset_mock()
        self.model.setInstrument("test")
        self.assertIsNone(self.model.instrument)
        self.assertIsNone(self.model.acquisitionMode)
        self.assertEqual(self.model.columns, list())
        self.assertIsNone(self.model.algorithm)
        self.assertEqual(self.model.settings, dict())
        self.mController.assert_not_called()

        # valid instrument
        self.mController.reset_mock()
        self.model.setInstrument("i1")
        self.assertEqual(self.model.instrument, "i1")
        self.assertEqual(self.model.acquisitionMode, self.ACQUISITION_MODES["i1"][0])
        self.assertEqual(self.model.columns, self.COLUMNS["a1"])
        self.assertEqual(self.model.algorithm, self.ALGORITHM["a1"])
        self.mController.assert_called_once()

    def test_getInstrument(self):
        self.assertEqual(self.model.getInstrument(), self.model.instrument)

    def test_setAcquisitionMode(self):
        # invalid aquisition mode
        self.mController.reset_mock()
        ac = self.model.acquisitionMode
        al = self.model.algorithm
        co = self.model.columns
        se = self.model.settings
        self.model.setAcquisitionMode("test_invalid")
        self.assertEqual(self.model.acquisitionMode, ac)
        self.assertEqual(self.model.algorithm, al)
        self.assertEqual(self.model.columns, co)
        self.assertEqual(self.model.settings, se)
        self.mController.assert_not_called()

        # valide acquisition mode
        self.mController.reset_mock()
        self.model.setAcquisitionMode("a2")
        self.assertNotEqual(self.model.acquisitionMode, ac)
        self.assertEqual(self.model.acquisitionMode, "a2")
        self.assertNotEqual(self.model.algorithm, al)
        self.assertEqual(self.model.algorithm, self.ALGORITHM["a2"])
        self.assertNotEqual(self.model.columns, co)
        self.assertEqual(self.model.columns, self.COLUMNS["a2"])
        self.assertNotEqual(self.model.settings, se)
        self.assertEqual(self.model.settings, dict.fromkeys(self.SETTINGS["a2"], "test"))
        self.mController.assert_called_once()

    def test_getAcquisitionMode(self):
        self.assertEqual(self.model.getAcquisitionMode(), self.model.acquisitionMode)

    def test_getAvailableAcquisitionModes(self):
        self.model.setInstrument("i1")
        self.assertEqual(self.model.getAvailableAcquisitionModes(), ["a1", "a2"])
        self.model.setInstrument("test")
        self.assertEqual(self.model.getAvailableAcquisitionModes(), [])

    def test_setCycleAndExperiment(self):
        self.model.setCycleAndExperiment("test1", "test2")
        self.assertEqual(self.model.cycleNumber, "test1")
        self.assertEqual(self.model.experimentId, "test2")

    def test_getCycleAndExperiment(self):
        self.model.cycleNumber = "test1"
        self.model.experimentId = "test2"
        c, e = self.model.getCycleAndExperiment()
        self.assertEqual(c, "test1")
        self.assertEqual(e, "test2")

    def test_initController(self):
        self.mController.reset_mock()
        self.model.algorithm = None
        self.model._initController()
        self.mController.assert_not_called()
        self.model.algorithm = "a1"
        self.model._initController()
        self.mController.assert_called_once_with("a1")

    def test_setSettings(self):
        params_a1 = dict.fromkeys(self.SETTINGS["a1"], "test")
        self.assertEqual(self.model.settings, params_a1)
        self.model.setSettings({"str": "test2"})
        self.assertNotEqual(self.model.settings, params_a1)
        self.model.setSettings({"str": "test"})
        self.assertEqual(self.model.settings, params_a1)
        self.model.setSettings({"str2": "test"})
        self.assertEqual(self.model.settings, params_a1)

    def test_getSettings(self):
        self.assertEqual(self.model.getSettings(), dict.fromkeys(self.SETTINGS["a1"], "test"))
        self.assertEqual(self.model.getSettings(), self.model.settings)

    def test_getSettingsTypes(self):
        alg = self.mSapi.AlgorithmManager.createUnmanaged.return_value
        self.mSapi.reset_mock()
        self.model.getSettingsTypes()
        self.mSapi.AlgorithmManager.createUnmanaged.assert_called_once_with(self.model.algorithm)
        alg.initialize.assert_called_once()

        # file property
        self.mSapi.reset_mock()
        prop = mock.Mock(spec=FileProperty)
        prop.allowedValues = ["test", "test"]
        prop.documentation = "test doc"
        alg.getProperty.return_value = prop
        types, values, docs = self.model.getSettingsTypes()
        self.assertDictEqual(types, {"str": "file", "int": "file", "float": "file", "bool": "file"})
        self.assertDictEqual(values, {
            "str": ["test", "test"],
            "int": ["test", "test"],
            "float": ["test", "test"],
            "bool": ["test", "test"]
        })
        self.assertDictEqual(docs, {"str": "test doc", "int": "test doc", "float": "test doc", "bool": "test doc"})

        # workspace property
        self.mSapi.reset_mock()
        prop = mock.Mock(spec=WorkspaceGroupProperty)
        prop.allowedValues = ["test", "test"]
        prop.documentation = "test doc"
        alg.getProperty.return_value = prop
        types, values, docs = self.model.getSettingsTypes()
        self.assertDictEqual(types, {"str": "workspace", "int": "workspace", "float": "workspace", "bool": "workspace"})
        self.assertDictEqual(values, {
            "str": ["test", "test"],
            "int": ["test", "test"],
            "float": ["test", "test"],
            "bool": ["test", "test"]
        })
        self.assertDictEqual(docs, {"str": "test doc", "int": "test doc", "float": "test doc", "bool": "test doc"})
        self.mSapi.reset_mock()
        prop = mock.Mock(spec=MatrixWorkspaceProperty)
        prop.allowedValues = ["test", "test"]
        prop.documentation = "test doc"
        alg.getProperty.return_value = prop
        types, values, docs = self.model.getSettingsTypes()
        self.assertDictEqual(types, {"str": "workspace", "int": "workspace", "float": "workspace", "bool": "workspace"})
        self.assertDictEqual(values, {
            "str": ["test", "test"],
            "int": ["test", "test"],
            "float": ["test", "test"],
            "bool": ["test", "test"]
        })
        self.assertDictEqual(docs, {"str": "test doc", "int": "test doc", "float": "test doc", "bool": "test doc"})

        # combobox property
        self.mSapi.reset_mock()
        prop = mock.Mock(spec=StringPropertyWithValue)
        prop.allowedValues = ["test", "test"]
        prop.documentation = "test doc"
        alg.getProperty.return_value = prop
        types, values, docs = self.model.getSettingsTypes()
        self.assertDictEqual(types, {"str": "combobox", "int": "combobox", "float": "combobox", "bool": "combobox"})
        self.assertDictEqual(values, {
            "str": ["test", "test"],
            "int": ["test", "test"],
            "float": ["test", "test"],
            "bool": ["test", "test"]
        })
        self.assertDictEqual(docs, {"str": "test doc", "int": "test doc", "float": "test doc", "bool": "test doc"})

        # bool property
        self.mSapi.reset_mock()
        prop = mock.Mock(spec=BoolPropertyWithValue)
        prop.allowedValues = ["test", "test"]
        prop.documentation = "test doc"
        alg.getProperty.return_value = prop
        types, values, docs = self.model.getSettingsTypes()
        self.assertDictEqual(types, {"str": "bool", "int": "bool", "float": "bool", "bool": "bool"})
        self.assertDictEqual(values, {
            "str": ["test", "test"],
            "int": ["test", "test"],
            "float": ["test", "test"],
            "bool": ["test", "test"]
        })
        self.assertDictEqual(docs, {"str": "test doc", "int": "test doc", "float": "test doc", "bool": "test doc"})

        # array property
        self.mSapi.reset_mock()
        prop = mock.Mock(spec=FloatArrayProperty)
        prop.allowedValues = ["test", "test"]
        prop.documentation = "test doc"
        alg.getProperty.return_value = prop
        types, values, docs = self.model.getSettingsTypes()
        self.assertDictEqual(types, {
            "str": "floatArray",
            "int": "floatArray",
            "float": "floatArray",
            "bool": "floatArray"
        })
        self.assertDictEqual(values, {
            "str": ["test", "test"],
            "int": ["test", "test"],
            "float": ["test", "test"],
            "bool": ["test", "test"]
        })
        self.assertDictEqual(docs, {"str": "test doc", "int": "test doc", "float": "test doc", "bool": "test doc"})

        # other property
        self.mSapi.reset_mock()
        prop = mock.Mock()
        prop.allowedValues = ["test", "test"]
        prop.documentation = "test doc"
        alg.getProperty.return_value = prop
        types, values, docs = self.model.getSettingsTypes()
        self.assertDictEqual(types, {"str": "string", "int": "string", "float": "string", "bool": "string"})
        self.assertDictEqual(values, {
            "str": ["test", "test"],
            "int": ["test", "test"],
            "float": ["test", "test"],
            "bool": ["test", "test"]
        })
        self.assertDictEqual(docs, {"str": "test doc", "int": "test doc", "float": "test doc", "bool": "test doc"})

    def test_setDefaultSettings(self):
        alg = self.mSapi.AlgorithmManager.createUnmanaged.return_value
        self.mSapi.reset_mock()
        alg.getProperty.return_value.value = "defaultValue"

        self.model._setDefaultSettings()
        self.mSapi.AlgorithmManager.createUnmanaged.assert_called_once_with(self.model.algorithm)
        alg.initialize.assert_called_once()
        self.assertDictEqual(self.model.settings, {
            "str": "defaultValue",
            "int": "defaultValue",
            "float": "defaultValue",
            "bool": "defaultValue"
        })

    def test_changeParameter(self):
        s1 = mock.Mock()
        s2 = mock.Mock()
        s1.getParameters.return_value = {"c1": "v0"}
        s2.getParameters.return_value = {}
        self.model.samples = [s1, s2]
        self.model.changeParameter(0, "c1", "v1")
        s1.changeParameter.assert_called_once_with("c1", "v1")
        self.mController.return_value.addParameter.assert_called_once()

        self.mController.reset_mock()
        self.model.changeParameter(0, "c1", "")
        self.mController.return_value.addParameter.assert_not_called()

    def test_groupSamples(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        s2 = mock.Mock()
        s3 = mock.Mock()
        self.model.samples = [s0, s1, s2, s3]
        self.model.groupSamples([0, 1, 3])
        self.assertDictEqual(self.model.groups, {'A': {s0, s1, s3}})
        self.model.masterSamples['A'] = s3
        self.model.groupSamples([0, 2, 3])
        self.assertDictEqual(self.model.groups, {'A': {s1}, 'B': {s0, s2, s3}})
        self.assertDictEqual(self.model.masterSamples, {})
        self.model.groupSamples([1, 3])
        self.assertDictEqual(self.model.groups, {'A': {s1, s3}, 'B': {s0, s2}})

    def test_ungroupSamples(self):
        s1 = mock.Mock()
        s2 = mock.Mock()
        s3 = mock.Mock()
        s4 = mock.Mock()
        s5 = mock.Mock()
        self.model.samples = [s1, s2, s3, s4, s5]
        self.model.groups = {'A': {s1, s5}, 'B': {s2, s3, s4}}
        self.model.ungroupSamples([1])
        self.assertDictEqual(self.model.groups, {'A': {s1, s5}, 'B': {s3, s4}})
        self.model.masterSamples['A'] = s5
        self.model.masterSamples['B'] = s2
        self.model.ungroupSamples([4])
        self.assertDictEqual(self.model.groups, {'A': {s1}, 'B': {s3, s4}})
        self.assertDictEqual(self.model.masterSamples, {'B': s2})

    def test_getSamplesGroups(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        s2 = mock.Mock()
        s3 = mock.Mock()
        s4 = mock.Mock()
        self.model.samples = [s0, s1, s2, s3, s4]
        self.model.groups = {'A': {s0, s2}, 'B': {s1, s3, s4}}
        groups = self.model.getSamplesGroups()
        self.assertDictEqual(groups, {'A': {0, 2}, 'B': {1, 3, 4}})

    def test_setGroupMaster(self):
        name = self.model.setGroupMaster(1)
        self.assertIsNone(name)
        self.assertEqual(self.model.groups, {})
        self.assertEqual(self.model.masterSamples, {})
        s0 = mock.Mock()
        s1 = mock.Mock()
        s2 = mock.Mock()
        s3 = mock.Mock()
        s4 = mock.Mock()
        self.model.samples = [s0, s1, s2, s3, s4]
        self.model.groups = {'A': {s0, s2}, 'B': {s1, s3, s4}}
        self.model.masterSamples['A'] = s2
        self.model.setGroupMaster(0)
        self.assertDictEqual(self.model.masterSamples, {'A': s0})

    def test_geMasterSamples(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        self.model.samples = [s0, s1]
        self.model.masterSamples = {'A': s0, 'B': s1}
        master = self.model.getMasterSamples()
        self.assertDictEqual(master, {'A': 0, 'B': 1})

    def test_getProcessingParameters(self):
        s1 = mock.Mock()
        s2 = mock.Mock()
        s3 = mock.Mock()
        s4 = mock.Mock()
        s1.getParameters.return_value = {"p1": "v1", "p2": "v2"}
        s2.getParameters.return_value = {}
        s3.getParameters.return_value = {"p2": "v2'"}
        s4.getParameters.return_value = {"p2": "DEFAULT"}
        self.model.samples = [s1, s2, s3, s4]
        self.model.groups = {"A": {s1, s2, s3, s4}}
        self.model.masterSamples = {"A": s1}
        params = dict.fromkeys(self.SETTINGS["a1"], "test")
        params.update({"p1": "v1", "p2": "v2"})
        params["OutputWorkspace"] = "sample_1"
        self.assertDictEqual(self.model.getProcessingParameters(0), params)
        params = dict.fromkeys(self.SETTINGS["a1"], "test")
        params.update({"p1": "v1", "p2": "v2"})
        params["OutputWorkspace"] = "sample_2"
        self.assertDictEqual(self.model.getProcessingParameters(1), params)
        params = dict.fromkeys(self.SETTINGS["a1"], "test")
        params.update({"p1": "v1", "p2": "v2'"})
        params["OutputWorkspace"] = "sample_3"
        self.assertDictEqual(self.model.getProcessingParameters(2), params)
        params = dict.fromkeys(self.SETTINGS["a1"], "test")
        params.update({"p1": "v1"})
        params["OutputWorkspace"] = "sample_4"
        self.assertDictEqual(self.model.getProcessingParameters(3), params)

    def test_process(self):
        s1 = mock.Mock()
        s1.getParameters.return_value = {"c1": "v1"}
        self.model.samples = [s1]
        self.model.process([0])
        self.mDrillTask.assert_called()
        self.model.tasksPool.addProcesses.assert_called_once()
        self.mDrillTask.reset_mock()
        self.model.tasksPool.reset_mock()

        self.model.samples = [s1, s1]
        self.model.process([0, 1])
        self.mDrillTask.assert_called()
        self.model.tasksPool.addProcesses.assert_called()
        self.mDrillTask.reset_mock()
        self.model.tasksPool.reset_mock()

    def test_onTaskStated(self):
        self.model.processStarted = mock.Mock()
        self.model._onTaskStarted(0)
        self.mLogger.information.assert_called_once()
        self.model.processStarted.emit.assert_called_once()

    def test_onTaskSuccess(self):
        self.model.processSuccess = mock.Mock()
        self.model.exportModel = mock.Mock()
        s1 = mock.Mock()
        s1.getParameter.return_value = None
        self.model.samples = [s1]
        self.model._onTaskSuccess(0)
        self.mLogger.information.assert_called_once()
        self.model.processSuccess.emit.assert_called_once()
        self.model.exportModel.run.assert_called_once_with(s1)

    def test_onTaskError(self):
        self.model.processError = mock.Mock()
        self.model._onTaskError(0, "")
        self.mLogger.error.assert_called_once()
        self.model.processError.emit.assert_called_once()

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

    @mock.patch("Interface.ui.drill.model.DrillModel.DrillRundexIO")
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

    def test_getVisualSettings(self):
        self.model.visualSettings = {"test": "test"}
        self.assertEqual(self.model.getVisualSettings(), {"test": "test"})

    def test_getColumnHeaderData(self):
        mAlg = mock.Mock()
        self.mSapi.AlgorithmManager.createUnmanaged.return_value = mAlg
        mProperty = mock.Mock()
        mProperty.documentation = "testDoc"
        mAlg.getProperty.return_value = mProperty
        title, doc = self.model.getColumnHeaderData()
        self.assertEqual(title, ["c1", "c2", "CustomOptions"])
        self.assertEqual(doc, ["testDoc", "testDoc", "testDoc"])

    def test_addSample(self):
        self.assertEqual(self.model.samples, [])
        self.model.addSample(0, mock.Mock())
        self.assertEqual(len(self.model.samples), 1)
        self.model.addSample(0, mock.Mock())
        self.assertEqual(len(self.model.samples), 2)

    def test_deleteSample(self):
        s1 = mock.Mock()
        s2 = mock.Mock()
        s3 = mock.Mock()
        self.model.samples = [s1, s2, s3]
        self.model.groups = {'A': {s2, s3}, 'B': {s1}}
        self.model.masterSamples = {'A': s2}
        self.model.deleteSample(0)
        self.assertEqual(self.model.samples, [s2, s3])
        self.assertDictEqual(self.model.groups, {'A': {s2, s3}})
        self.assertDictEqual(self.model.masterSamples, {'A': s2})
        self.model.deleteSample(0)
        self.assertEqual(self.model.samples, [s3])
        self.assertDictEqual(self.model.groups, {'A': {s3}})
        self.assertDictEqual(self.model.masterSamples, {})

    def test_getRowsContents(self):
        table = self.model.getRowsContents()
        self.assertEqual(table, [])
        s0 = mock.Mock()
        s0.getParameters.return_value = {"c1": "test1", "c2": "test2"}
        s1 = mock.Mock()
        s1.getParameters.return_value = {"c2": "test3"}
        self.model.samples = [s0, s1]
        table = self.model.getRowsContents()
        self.assertEqual(table, [["test1", "test2"], ["", "test3"]])


if __name__ == "__main__":
    unittest.main()
