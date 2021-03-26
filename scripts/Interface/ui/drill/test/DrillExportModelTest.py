# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from Interface.ui.drill.model.DrillExportModel import DrillExportModel


class DrillExportModelTest(unittest.TestCase):

    EXPORT_ALGORITHMS = {"a1": {"ea1": True, "ea2": False}, "a2": {"ea2": True}}

    EXPORT_ALGO_CRITERIA = {"ea1": "%param% == 'test'", "ea2": "%param% != 'test'"}

    EXPORT_ALGO_EXTENSION = {"ea1": ".txt", "ea2": ".xml"}

    def setUp(self):
        patch = mock.patch("Interface.ui.drill.model.DrillExportModel.mtd")
        self.mMtd = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch('Interface.ui.drill.model.DrillExportModel.logger')
        self.mLogger = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch('Interface.ui.drill.model.DrillExportModel.config')
        self.mConfig = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict("Interface.ui.drill.model.DrillExportModel"
                                ".RundexSettings.EXPORT_ALGORITHMS",
                                self.EXPORT_ALGORITHMS,
                                clear=True)
        self.mAlgo = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict("Interface.ui.drill.model.DrillExportModel"
                                ".RundexSettings.EXPORT_ALGO_CRITERIA",
                                self.EXPORT_ALGO_CRITERIA,
                                clear=True)
        self.mAlgoCriteria = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch.dict("Interface.ui.drill.model.DrillExportModel"
                                ".RundexSettings.EXPORT_ALGO_EXTENSION",
                                self.EXPORT_ALGO_EXTENSION,
                                clear=True)
        self.mAlgoExtension = patch.start()
        self.addCleanup(patch.stop)

        patch = mock.patch("Interface.ui.drill.model.DrillExportModel" ".DrillAlgorithmPool")
        self.mTasksPool = patch.start()
        self.mTasksPool = self.mTasksPool.return_value
        self.addCleanup(patch.stop)

        patch = mock.patch("Interface.ui.drill.model.DrillExportModel" ".DrillTask")
        self.mTask = patch.start()
        self.addCleanup(patch.stop)

        self.exportModel = DrillExportModel("a1")

    def test_init(self):
        self.assertDictEqual(self.exportModel._exportAlgorithms, self.EXPORT_ALGORITHMS["a1"])
        self.mTasksPool.signals.taskError.connect.assert_called_once()
        self.mTasksPool.signals.taskSuccess.connect.assert_called_once()
        self.assertEqual(self.exportModel._exports, {})
        self.assertEqual(self.exportModel._successExports, {})

    def test_getAlgorithms(self):
        algs = self.exportModel.getAlgorithms()
        self.assertEqual(algs, [a for a in self.EXPORT_ALGORITHMS["a1"]])

    def test_isAlgorithmActivated(self):
        self.assertEqual(self.exportModel._exportAlgorithms["ea1"], True)
        self.assertEqual(self.exportModel.isAlgorithmActivated("ea1"), True)
        self.exportModel._exportAlgorithms["ea1"] = False
        self.assertEqual(self.exportModel.isAlgorithmActivated("ea1"), False)
        self.assertEqual(self.exportModel._exportAlgorithms["ea2"], False)
        self.assertEqual(self.exportModel.isAlgorithmActivated("ea2"), False)

    def test_activateAlgorithm(self):
        self.assertEqual(self.exportModel._exportAlgorithms["ea2"], False)
        self.exportModel.activateAlgorithm("ea2")
        self.assertEqual(self.exportModel._exportAlgorithms["ea2"], True)
        self.exportModel.activateAlgorithm("ea2")
        self.assertEqual(self.exportModel._exportAlgorithms["ea2"], True)
        self.exportModel.activateAlgorithm("ea3")

    def test_inactivateAlgorithm(self):
        self.assertEqual(self.exportModel._exportAlgorithms["ea1"], True)
        self.exportModel.inactivateAlgorithm("ea1")
        self.assertEqual(self.exportModel._exportAlgorithms["ea1"], False)
        self.exportModel.inactivateAlgorithm("ea1")
        self.assertEqual(self.exportModel._exportAlgorithms["ea1"], False)
        self.exportModel.inactivateAlgorithm("ea3")

    def test_valid_Criteria(self):
        mHist = self.mMtd.__getitem__.return_value.getHistory.return_value
        mAlgo = mHist.lastAlgorithm.return_value
        mAlgo.getPropertyValue.return_value = "test"
        self.assertTrue(self.exportModel._validCriteria(mock.Mock(), "ea3"))
        self.assertTrue(self.exportModel._validCriteria(mock.Mock(), "ea1"))
        self.assertFalse(self.exportModel._validCriteria(mock.Mock(), "ea2"))

    def test_onTaskSuccess(self):
        self.exportModel._logSuccessExport = mock.Mock()
        self.exportModel._exports = {"workspace1": {"filename1", "filename2"}, "workspace2": {"filename3"}}
        self.exportModel._onTaskSuccess("workspace1:filename1")
        self.assertDictEqual(self.exportModel._successExports, {"workspace1": {"filename1"}})
        self.assertDictEqual(self.exportModel._exports, {"workspace1": {"filename2"}, "workspace2": {"filename3"}})
        self.exportModel._onTaskSuccess("workspace1:filename2")
        self.assertDictEqual(self.exportModel._successExports, {"workspace1": {"filename1", "filename2"}})
        self.assertDictEqual(self.exportModel._exports, {"workspace2": {"filename3"}})
        self.exportModel._logSuccessExport.assert_called_once()

    def test_onTaskError(self):
        self.exportModel._logSuccessExport = mock.Mock()
        self.exportModel._exports = {"workspace1": {"filename1", "filename2"}, "workspace2": {"filename3"}}
        self.exportModel._onTaskError("workspace2:filename3", "error message")
        self.mLogger.error.assert_called()
        self.assertDictEqual(self.exportModel._exports, {"workspace1": {"filename1", "filename2"}})
        self.exportModel._logSuccessExport.assert_called_once()

    def test_logSuccessExport(self):
        self.exportModel._successExports = {"workspace1": {"filename1", "filename2"}}
        self.exportModel._logSuccessExport("workspace1")
        self.mLogger.notice.assert_called()
        self.assertDictEqual(self.exportModel._successExports, {})

    def test_run(self):
        self.mConfig.getString.return_value = "/default/save/directory/"
        mSample = mock.Mock()
        mSample.getOutputName.return_value = "workspace"
        mGroup = mock.Mock()
        mGroup.getNames.return_value = ["workspace"]
        self.mMtd.__getitem__.return_value = mGroup
        self.mMtd.getObjectNames.return_value = ["workspace_1", "workspace_2"]
        self.exportModel._validCriteria = mock.Mock()
        self.exportModel._validCriteria.return_value = True
        self.exportModel.run(mSample)
        self.assertDictEqual(
            self.exportModel._exports, {
                "workspace_1": {"/default/save/directory/workspace_1.txt"},
                "workspace_2": {"/default/save/directory/workspace_2.txt"}
            })
        self.mTask.assert_called()


if __name__ == "__main__":
    unittest.main()
