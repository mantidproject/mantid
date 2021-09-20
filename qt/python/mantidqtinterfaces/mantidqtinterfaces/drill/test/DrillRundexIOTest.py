# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.drill.model.DrillRundexIO import DrillRundexIO


class DrillRundexIOTest(unittest.TestCase):

    def setUp(self):
        # mock open
        patch = mock.patch('mantidqtinterfaces.drill.model.DrillRundexIO.open')
        self.mOpen = patch.start()
        self.addCleanup(patch.stop)

        # mock json
        patch = mock.patch('mantidqtinterfaces.drill.model.DrillRundexIO.json')
        self.mJson = patch.start()
        self.addCleanup(patch.stop)

        self.filename = "test"
        self.mDrillModel = mock.Mock()
        self.mExportModel = mock.Mock()
        self.mDrillModel.getExportModel.return_value = self.mExportModel
        self.model = DrillRundexIO(self.filename, self.mDrillModel)

    def test_getFilename(self):
        self.assertEqual(self.model.getFilename(), self.filename)

    def test_loadRundexV1(self):
        self.mJson.load.return_value = {
                "Instrument": "i1",
                "AcquisitionMode": "a1",
                "CycleNumber": "cycle",
                "ExperimentID": "exp",
                "VisualSettings": {
                    "FoldedColumns": [
                        "c1",
                        "c2",
                        "c3"
                        ],
                    "HiddenColumns": [
                        "c4",
                        "c5"
                        ],
                    "ColumnsOrder": [
                        "c1",
                        "c2",
                        "c3",
                        "c4",
                        "c5"
                        ]
                    },
                "GlobalSettings": {
                    "param1": True,
                    "param2": "value1",
                    "param3": 0.1,
                    "param4": [0.1, 0.2, 0.3],
                    "param5": [],
                    },
                "Samples": [
                        {
                            "param6": "value1",
                            "param7": "value2",
                            "CustomOptions": {
                                "param1": False
                                }
                            }
                        ]
                }
        mD = self.mDrillModel
        p1 = mock.Mock()
        p1.getName.return_value = "param1"
        p2 = mock.Mock()
        p2.getName.return_value = "param2"
        p3 = mock.Mock()
        p3.getName.return_value = "param3"
        p6 = mock.Mock()
        p6.getName.return_value = "param6"
        mD.getParameters.return_value = [p1, p2, p3, p6]
        self.model.load()
        self.mOpen.assert_called_once_with("test")
        self.mJson.load.assert_called_once()
        mD.setCycleAndExperiment.assert_called_once_with("cycle", "exp")
        mD.setVisualSettings.assert_called_once()
        p1.setValue.assert_called_once()
        p2.setValue.assert_called_once()
        p3.setValue.assert_called_once()
        p6.setValue.assert_not_called()
        mD.addSample.assert_called_once_with(0)
        mSample = mD.addSample.return_value
        calls = [mock.call("param6"), mock.call().setValue('value1'),
                 mock.call("param7"), mock.call().setValue('value2'),
                 mock.call("param1"), mock.call().setValue(False)]
        mSample.addParameter.assert_has_calls(calls)

    def test_loadRundexV2(self):
        self.mJson.load.return_value = {
                "Instrument": "i1",
                "AcquisitionMode": "a1",
                "CycleNumber": "cycle",
                "ExperimentID": "exp",
                "VisualSettings": {
                    "FoldedColumns": [
                        "c1",
                        "c2",
                        "c3"
                        ],
                    "HiddenColumns": [
                        "c4",
                        "c5"
                        ],
                    "ColumnsOrder": [
                        "c1",
                        "c2",
                        "c3",
                        "c4",
                        "c5"
                        ]
                    },
                "GlobalSettings": {
                    "param1": True,
                    "param2": "value1",
                    "param3": 0.1,
                    "param4": [0.1, 0.2, 0.3],
                    "param5": [],
                    },
                "Samples": [
                        {
                            "param6": "value1",
                            "param7": "value2",
                            "param1": False
                            }
                        ]
                }
        mD = self.mDrillModel
        p1 = mock.Mock()
        p1.getName.return_value = "param1"
        p2 = mock.Mock()
        p2.getName.return_value = "param2"
        p3 = mock.Mock()
        p3.getName.return_value = "param3"
        p6 = mock.Mock()
        p6.getName.return_value = "param6"
        mD.getParameters.return_value = [p1, p2, p3, p6]
        self.model.load()
        self.mOpen.assert_called_once_with("test")
        self.mJson.load.assert_called_once()
        mD.setCycleAndExperiment.assert_called_once_with("cycle", "exp")
        mD.setVisualSettings.assert_called_once()
        p1.setValue.assert_called_once()
        p2.setValue.assert_called_once()
        p3.setValue.assert_called_once()
        p6.setValue.assert_not_called()
        mD.addSample.assert_called_once_with(0)
        mSample = mD.addSample.return_value
        calls = [mock.call("param6"), mock.call().setValue('value1'),
                 mock.call("param7"), mock.call().setValue('value2'),
                 mock.call("param1"), mock.call().setValue(False)]
        mSample.addParameter.assert_has_calls(calls)

    def test_save(self):
        mD = self.mDrillModel
        mD.getInstrument.return_value = "i1"
        mD.getAcquisitionMode.return_value = "a1"
        mD.getCycleAndExperiment.return_value = "cycle", "exp"
        mD.getVisualSettings.return_value = {"key": "value"}
        p1 = mock.Mock()
        p1.getName.return_value = "setting1"
        p1.getValue.return_value = "value1"
        mD.getParameters.return_value = [p1]
        self.mExportModel.getAlgorithms.return_value = ["ex1", "ex2"]
        self.mExportModel.isAlgoritmActivated.return_value = True
        s0 = mock.Mock()
        g0 = mock.Mock()
        s0.getIndex.return_value = 0
        s0.getParameterValues.return_value = {"param1": "value1"}
        s0.getGroup.return_value = g0
        g0.getName.return_value = "A"
        g0.getMaster.return_value = s0
        g0.getSamples.return_value = [s0]
        mD.getSamples.return_value = [s0]
        mD.getSampleGroups.return_value = {"A": g0}

        json = {
                "Instrument": "i1",
                "AcquisitionMode": "a1",
                "CycleNumber": "cycle",
                "ExperimentID": "exp",
                "VisualSettings": {"key": "value"},
                "GlobalSettings": {"setting1": "value1"},
                "ExportAlgorithms": ["ex1", "ex2"],
                "Samples": [{"param1": "value1"}],
                "SamplesGroups": {"A": [0]},
                "MasterSamples": {"A": 0}
                }
        self.model.save()
        self.mOpen.assert_called_once_with("test", 'w')
        self.mJson.dump.assert_called_once()
        name, args, kwargs = self.mJson.dump.mock_calls[0]
        self.assertDictEqual(args[0], json)


if __name__ == "__main__":
    unittest.main()
