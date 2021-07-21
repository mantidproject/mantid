# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from Interface.ui.drill.model.DrillSample import DrillSample


class DrillSampleTest(unittest.TestCase):

    INDEX = 99

    def setUp(self):
        self.sample = DrillSample(self.INDEX)

    def test_init(self):
        self.assertDictEqual(self.sample._parameters, {})
        self.assertEqual(self.sample._index, self.INDEX)
        self.assertIsNone(self.sample._group)
        self.assertFalse(self.sample._master)

    def test_setController(self):
        controller = mock.Mock()
        self.sample.setController(controller)
        self.assertEqual(self.sample._controller, controller)

    def test_setIndex(self):
        index = 120
        self.sample.setIndex(index)
        self.assertEqual(self.sample._index, index)

    def test_getIndex(self):
        index = self.sample.getIndex()
        self.assertEqual(index, self.INDEX)

    def test_setName(self):
        self.assertIsNone(self.sample._name)
        self.sample.setName("test")
        self.assertEqual(self.sample._name, "test")

    def test_getName(self):
        self.sample._name = "test"
        name = self.sample.getName()
        self.assertEqual(name, "test")

    def test_setOutputName(self):
        self.assertIsNone(self.sample._outputName)
        self.sample.setOutputName("test")
        self.assertEqual(self.sample._outputName, "test")

    def test_getOutputName(self):
        self.assertIsNone(self.sample._outputName)
        self.sample._outputName = "test"
        name = self.sample.getOutputName()
        self.assertEqual(name, "test")

    def test_setGroup(self):
        self.sample.groupChanged = mock.Mock()
        self.assertIsNone(self.sample._group)
        self.assertIsNone(self.sample._groupIndex)
        self.assertFalse(self.sample._master)
        self.sample.setGroup("test")
        self.assertEqual(self.sample._group, "test")
        self.assertIsNone(self.sample._groupIndex)
        self.sample.groupChanged.emit.assert_called_once()
        self.sample.groupChanged.reset_mock()
        self.sample.setGroup("test2", 10)
        self.assertEqual(self.sample._group, "test2")
        self.assertEqual(self.sample._groupIndex, 10)
        self.assertFalse(self.sample._master)
        self.sample._master = True
        self.sample.groupChanged.emit.assert_called_once()
        self.sample.groupChanged.reset_mock()
        self.sample.setGroup("test3", 20)
        self.assertEqual(self.sample._group, "test3")
        self.assertEqual(self.sample._groupIndex, 20)
        self.assertFalse(self.sample._master)
        self.sample.groupChanged.emit.assert_called_once()
        self.sample.groupChanged.reset_mock()
        self.sample._master = True
        self.sample.setGroup(None)
        self.assertIsNone(self.sample._group)
        self.assertIsNone(self.sample._groupIndex)
        self.assertFalse(self.sample._master)
        self.sample.groupChanged.emit.assert_called_once()
        self.sample.groupChanged.reset_mock()

    def test_getGroupName(self):
        self.assertIsNone(self.sample._group)
        self.sample._group = "test"
        group = self.sample.getGroupName()
        self.assertEqual(group, "test")

    def test_getGroupIndex(self):
        self.assertIsNone(self.sample._groupIndex)
        self.sample._groupIndex = 22
        index = self.sample.getGroupIndex()
        self.assertEqual(index, 22)

    def test_setmaster(self):
        self.sample.groupChanged = mock.Mock()
        self.assertFalse(self.sample._master)
        self.assertIsNone(self.sample._group)
        self.sample.setMaster(True)
        self.assertFalse(self.sample._master)
        self.sample.groupChanged.emit.assert_not_called()
        self.sample._group = "test"
        self.sample.setMaster(True)
        self.assertTrue(self.sample._master)
        self.sample.groupChanged.emit.assert_called_once()
        self.sample.groupChanged.reset_mock()
        self.sample.setMaster(False)
        self.assertFalse(self.sample._master)
        self.sample.groupChanged.emit.assert_called_once()

    def test_isMaster(self):
        self.assertFalse(self.sample._master)
        master = self.sample.isMaster()
        self.assertFalse(master)
        self.sample._master = True
        master = self.sample.isMaster()
        self.assertTrue(master)

    @mock.patch("Interface.ui.drill.model.DrillSample.DrillParameter")
    def test_addParameter(self, mParam):
        self.assertDictEqual(self.sample._parameters, {})
        self.sample.newParameter = mock.Mock()
        self.sample._controller = mock.Mock()
        self.sample.addParameter("test")
        mParam.assert_called_once_with("test")
        mParam.return_value.setController.assert_called_once_with(
                self.sample._controller)
        self.assertDictEqual(self.sample._parameters,
                             {"test": mParam.return_value})
        self.sample.newParameter.emit.assert_called_once_with(
                mParam.return_value)

    def test_getParameter(self):
        self.assertDictEqual(self.sample._parameters, {})
        p1 = mock.Mock()
        p2 = mock.Mock()
        self.sample._parameters = {"p1": p1, "p2": p2}
        self.assertEqual(self.sample.getParameter("p1"), p1)
        self.assertEqual(self.sample.getParameter("p2"), p2)
        self.assertIsNone(self.sample.getParameter("p3"))

    def test_getParameterValues(self):
        self.assertDictEqual(self.sample._parameters, {})
        self.assertDictEqual(self.sample.getParameterValues(), {})
        p1 = mock.Mock()
        p1.getValue.return_value = "v1"
        p2 = mock.Mock()
        p2.getValue.return_value = "v2"
        self.sample._parameters = {"p1": p1, "p2": p2}
        self.assertDictEqual(self.sample.getParameterValues(),
                             {"p1": "v1", "p2": "v2"})

    @mock.patch("Interface.ui.drill.model.DrillSample.logger")
    def test_onProcessStarted(self, mLogger):
        self.sample.statusChanged = mock.Mock()
        self.sample.onProcessStarted()
        mLogger.information.assert_called_once()
        self.sample.statusChanged.emit.assert_called_once()

    @mock.patch("Interface.ui.drill.model.DrillSample.logger")
    def test_onProcessSuccess(self, mLogger):
        self.sample.statusChanged = mock.Mock()
        self.sample.onProcessSuccess()
        mLogger.information.assert_called_once()
        self.sample.statusChanged.emit.assert_called_once_with()

    @mock.patch("Interface.ui.drill.model.DrillSample.logger")
    def test_onProcessError(self, mLogger):
        self.sample.statusChanged = mock.Mock()
        self.sample.onProcessError("test")
        mLogger.error.assert_called_once()
        self.sample.statusChanged.emit.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
