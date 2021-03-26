# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from Interface.ui.drill.model.DrillRundexIO import DrillRundexIO


class DrillRundexIOTest(unittest.TestCase):
    def setUp(self):
        # mock open
        patch = mock.patch('Interface.ui.drill.model.DrillRundexIO.open')
        self.mOpen = patch.start()
        self.addCleanup(patch.stop)

        # mock json
        patch = mock.patch('Interface.ui.drill.model.DrillRundexIO.json')
        self.mJson = patch.start()
        self.addCleanup(patch.stop)

        self.filename = "test"
        self.mDrillModel = mock.Mock()
        self.mExportModel = mock.Mock()
        self.mDrillModel.getExportModel.return_value = self.mExportModel
        self.model = DrillRundexIO(self.filename, self.mDrillModel)

    def test_getFilename(self):
        self.assertEqual(self.model.getFilename(), self.filename)

    @mock.patch("Interface.ui.drill.model.DrillRundexIO.DrillSample")
    def test_loadRundexV1(self, mSample):
        self.mJson.load.return_value = {
            "Instrument": "i1",
            "AcquisitionMode": "a1",
            "CycleNumber": "cycle",
            "ExperimentID": "exp",
            "VisualSettings": {
                "FoldedColumns": ["c1", "c2", "c3"],
                "HiddenColumns": ["c4", "c5"],
                "ColumnsOrder": ["c1", "c2", "c3", "c4", "c5"]
            },
            "GlobalSettings": {
                "param1": True,
                "param2": "value1",
                "param3": 0.1,
                "param4": [0.1, 0.2, 0.3],
                "param5": [],
            },
            "Samples": [{
                "param6": "value1",
                "param7": "value2",
                "CustomOptions": {
                    "param1": False
                }
            }]
        }
        self.model.load()
        self.mOpen.assert_called_once_with("test")
        self.mJson.load.assert_called_once()
        mD = self.mDrillModel
        mD.setCycleAndExperiment.assert_called_once_with("cycle", "exp")
        mD.setVisualSettings.assert_called_once()
        sample = mSample.return_value
        sample.setParameters.assert_called_once_with({"param6": "value1", "param7": "value2", "param1": False})
        mD.addSample.assert_called_once_with(-1, sample)

    @mock.patch("Interface.ui.drill.model.DrillRundexIO.DrillSample")
    def test_loadRundexV2(self, mSample):
        self.mJson.load.return_value = {
            "Instrument": "i1",
            "AcquisitionMode": "a1",
            "CycleNumber": "cycle",
            "ExperimentID": "exp",
            "VisualSettings": {
                "FoldedColumns": ["c1", "c2", "c3"],
                "HiddenColumns": ["c4", "c5"],
                "ColumnsOrder": ["c1", "c2", "c3", "c4", "c5"]
            },
            "GlobalSettings": {
                "param1": True,
                "param2": "value1",
                "param3": 0.1,
                "param4": [0.1, 0.2, 0.3],
                "param5": [],
            },
            "Samples": [{
                "param6": "value1",
                "param7": "value2",
                "param1": False
            }]
        }
        self.model.load()
        self.mOpen.assert_called_once_with("test")
        self.mJson.load.assert_called_once()
        mD = self.mDrillModel
        mD.setCycleAndExperiment.assert_called_once_with("cycle", "exp")
        mD.setVisualSettings.assert_called_once()
        sample = mSample.return_value
        sample.setParameters.assert_called_once_with({"param6": "value1", "param7": "value2", "param1": False})
        mD.addSample.assert_called_once_with(-1, sample)

    def test_save(self):
        mD = self.mDrillModel
        mD.getInstrument.return_value = "i1"
        mD.getAcquisitionMode.return_value = "a1"
        mD.getCycleAndExperiment.return_value = "cycle", "exp"
        mD.getVisualSettings.return_value = {"key": "value"}
        mD.getSettings.return_value = {"setting1": "value1"}
        self.mExportModel.getAlgorithms.return_value = ["ex1", "ex2"]
        self.mExportModel.isAlgoritmActivated.return_value = True
        s0 = mock.Mock()
        s0.getParameters.return_value = {"param1": "value1"}
        mD.getSamples.return_value = [s0]
        mD.getSamplesGroups.return_value = {"A": [0]}
        mD.getMasterSamples.return_value = {"A": 0}

        json = {
            "Instrument": "i1",
            "AcquisitionMode": "a1",
            "CycleNumber": "cycle",
            "ExperimentID": "exp",
            "VisualSettings": {
                "key": "value"
            },
            "GlobalSettings": {
                "setting1": "value1"
            },
            "ExportAlgorithms": ["ex1", "ex2"],
            "Samples": [{
                "param1": "value1"
            }],
            "SamplesGroups": {
                "A": [0]
            },
            "MasterSamples": {
                "A": 0
            }
        }
        self.model.save()
        self.mOpen.assert_called_once_with("test", 'w')
        self.mJson.dump.assert_called_once()
        name, args, kwargs = self.mJson.dump.mock_calls[0]
        self.assertDictEqual(args[0], json)


if __name__ == "__main__":
    unittest.main()
