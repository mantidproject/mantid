# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import mock

from Interface.ui.drill.main.DrillModel import DrillModel, DrillException


class DrillModelTest(unittest.TestCase):

    TECHNIQUES = {
            "i1": ["t1"]
            }

    COLUMNS = {
            "t1": ["c1", "c2"]
            }

    ALGORITHMS = {
            "t1": "a1"
            }

    SETTINGS = {
            "t1": {
                "str": "test",
                "int": 1,
                "float": "0.9",
                "bool": False
                }
            }

    def setUp(self):
        # mock config
        patch = mock.patch('Interface.ui.drill.main.DrillModel.config')
        self.mConfig = patch.start()
        self.mConfig.__getitem__.return_value = "i1"
        self.addCleanup(patch.stop)

        # mock specifications
        patch = mock.patch.dict(
                'Interface.ui.drill.main.DrillModel.RundexSettings.TECHNIQUES',
                self.TECHNIQUES
                )
        self.mTechniques = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict(
                'Interface.ui.drill.main.DrillModel.RundexSettings.COLUMNS',
                self.COLUMNS
                )
        self.mColumns = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict(
                'Interface.ui.drill.main.DrillModel.RundexSettings.ALGORITHMS',
                self.ALGORITHMS
                )
        self.mAlgo = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict(
                'Interface.ui.drill.main.DrillModel.RundexSettings.SETTINGS',
                self.SETTINGS
                )
        self.mSettings = patch.start()
        self.addCleanup(patch.stop)

        # mock drill task
        patch = mock.patch('Interface.ui.drill.main.DrillModel.DrillTask')
        self.mDrillTask = patch.start()
        self.addCleanup(patch.stop)

        # mock drill tasks pool
        patch = mock.patch(
                'Interface.ui.drill.main.DrillModel.DrillAlgorithmPool'
                )
        self.mTasksPool = patch.start()
        self.addCleanup(patch.stop)

        self.model = DrillModel()

    def test_setSettings(self):
        self.assertEqual(self.model.settings, self.SETTINGS["t1"])
        self.model.setSettings({"str": "test2"})
        self.assertNotEqual(self.model.settings, self.SETTINGS["t1"])
        self.model.setSettings({"str": "test"})
        self.assertEqual(self.model.settings, self.SETTINGS["t1"])
        self.model.setSettings({"str2": "test"})
        self.assertEqual(self.model.settings, self.SETTINGS["t1"])

    def test_getSettings(self):
        self.assertEqual(self.model.getSettings(), self.SETTINGS["t1"])
        self.assertEqual(self.model.getSettings(), self.model.settings)

    def test_getProcessingParameters(self):
        params = dict()
        params.update(self.SETTINGS["t1"])
        params["OutputWorkspace"] = "sample_1"
        self.model.samples = [{}]
        self.assertEqual(self.model.getProcessingParameters(0), params)
        self.model.samples = [{"c1": "test2"}]
        params.update({"c1": "test2"})
        self.assertEqual(self.model.getProcessingParameters(0), params)
        self.model.samples = [{"c1": "test2", "CustomOptions": {"int": 2}}]
        params["int"] = 2
        self.assertEqual(self.model.getProcessingParameters(0), params)

    def test_process(self):
        self.model.samples = [{}]
        self.model.process([0])
        self.mDrillTask.assert_not_called()
        self.model.tasksPool.addProcess.assert_not_called()
        self.mDrillTask.reset_mock()
        self.model.tasksPool.reset_mock()

        self.model.samples = [{"c1": "v1"}]
        self.model.process([0])
        self.mDrillTask.assert_called()
        self.model.tasksPool.addProcess.assert_called_once()
        self.mDrillTask.reset_mock()
        self.model.tasksPool.reset_mock()

        self.model.samples = [{"c1": "v1"}, {"c1": "v1"}]
        self.model.process([0, 1])
        self.mDrillTask.assert_called()
        self.model.tasksPool.addProcess.assert_called()
        self.mDrillTask.reset_mock()
        self.model.tasksPool.reset_mock()

        self.mDrillTask.side_effect = Exception()
        self.model.samples = [{"c1": "v1"}, {"c1": "v1"}]
        try:
            self.model.process([0, 1])
        except DrillException as a:
            assert True
        else:
            assert False

    def test_stopProcess(self):
        self.model.stopProcess()
        self.model.tasksPool.abortProcessing.assert_called_once()


if __name__ == "__main__":
    unittest.main()
