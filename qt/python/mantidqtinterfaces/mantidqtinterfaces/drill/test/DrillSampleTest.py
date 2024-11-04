# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.drill.model.DrillSample import DrillSample


class DrillSampleTest(unittest.TestCase):
    INDEX = 99

    def setUp(self):
        self.sample = DrillSample(self.INDEX)

    def test_init(self):
        self.assertDictEqual(self.sample._parameters, {})
        self.assertEqual(self.sample._index, self.INDEX)
        self.assertIsNone(self.sample._group)

    def test_setController(self):
        controller = mock.Mock()
        self.sample.setController(controller)
        self.assertEqual(self.sample._controller, controller)

    def test_setExporter(self):
        exporter = mock.Mock()
        self.sample.setExporter(exporter)
        self.assertEqual(self.sample._exporter, exporter)

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
        group = mock.Mock()
        self.sample.setGroup(group)
        self.assertEqual(self.sample._group, group)
        self.sample.groupChanged.emit.assert_called_once()

    def test_getGroup(self):
        group = mock.Mock()
        self.sample._group = group
        self.assertEqual(self.sample.getGroup(), group)

    @mock.patch("mantidqtinterfaces.drill.model.DrillSample.DrillParameter")
    def test_addParameter(self, mParam):
        self.assertDictEqual(self.sample._parameters, {})
        self.sample.newParameter = mock.Mock()
        self.sample._controller = mock.Mock()
        self.sample.addParameter("test")
        mParam.assert_called_once_with("test")
        mParam.return_value.setController.assert_called_once_with(self.sample._controller)
        self.assertDictEqual(self.sample._parameters, {"test": mParam.return_value})
        self.sample.newParameter.emit.assert_called_once_with(mParam.return_value)

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
        self.assertDictEqual(self.sample.getParameterValues(), {"p1": "v1", "p2": "v2"})

    @mock.patch("mantidqtinterfaces.drill.model.DrillSample.logger")
    def test_onProcessStarted(self, mLogger):
        self.sample.statusChanged = mock.Mock()
        self.sample.onProcessStarted()
        mLogger.information.assert_called_once()
        self.sample.statusChanged.emit.assert_called_once()

    @mock.patch("mantidqtinterfaces.drill.model.DrillSample.logger")
    def test_onProcessSuccess(self, mLogger):
        self.sample._exporter = mock.Mock()
        self.sample.statusChanged = mock.Mock()
        self.sample.onProcessSuccess()
        mLogger.information.assert_called_once()
        self.sample.statusChanged.emit.assert_called_once_with()
        self.sample._exporter.run.assert_called_once_with(self.sample)

    @mock.patch("mantidqtinterfaces.drill.model.DrillSample.logger")
    def test_onProcessError(self, mLogger):
        self.sample.statusChanged = mock.Mock()
        self.sample.onProcessError("test")
        mLogger.error.assert_called_once()
        self.sample.statusChanged.emit.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
