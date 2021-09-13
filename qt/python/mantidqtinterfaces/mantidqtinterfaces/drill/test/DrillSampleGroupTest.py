# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.drill.model.DrillSampleGroup import DrillSampleGroup


class DrillSampleGroupTest(unittest.TestCase):

    def setUp(self):
        self.sampleGroup = DrillSampleGroup()

    def test_init(self):
        self.assertEqual(self.sampleGroup._name, "")
        self.assertEqual(self.sampleGroup._samples, [])

    def test_setName(self):
        name = "test"
        self.sampleGroup.setName(name)
        self.assertEqual(self.sampleGroup._name, name)

    def test_getName(self):
        name = "test"
        self.sampleGroup._name = name
        self.assertEqual(self.sampleGroup.getName(), name)

    def test_addSample(self):
        s0 = mock.Mock()
        s0.getIndex.return_value = 0
        s1 = mock.Mock()
        s1.getIndex.return_value = 1
        self.assertEqual(self.sampleGroup._samples, [])
        self.sampleGroup.addSample(s1)
        self.assertEqual(self.sampleGroup._samples, [s1])
        s1.groupChanged.emit.assert_called_once()
        self.sampleGroup.addSample(s0)
        self.assertEqual(self.sampleGroup._samples, [s0, s1])
        s0.groupChanged.emit.assert_called_once()

    def test_delSample(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        self.sampleGroup._samples = [s0, s1]
        self.sampleGroup.delSample(s0)
        self.assertEqual(self.sampleGroup._samples, [s1])
        s1.groupChanged.emit.assert_called_once()

    def test_getSampleIndex(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        self.sampleGroup._samples = [s0, s1]
        self.assertEqual(self.sampleGroup.getSampleIndex(s0), 0)
        self.assertEqual(self.sampleGroup.getSampleIndex(s1), 1)

    def test_isEmpty(self):
        self.assertEqual(self.sampleGroup._samples, [])
        self.assertTrue(self.sampleGroup.isEmpty())
        self.sampleGroup._samples = [mock.Mock()]
        self.assertFalse(self.sampleGroup.isEmpty())

    def test_setMaster(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        self.sampleGroup._samples = [s0, s1]
        self.assertIsNone(self.sampleGroup._master)
        self.sampleGroup.setMaster(s0)
        self.assertEqual(self.sampleGroup._master, s0)
        self.sampleGroup.setMaster(s1)
        self.assertEqual(self.sampleGroup._master, s1)

    def test_unsetMaster(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        self.sampleGroup._samples = [s0, s1]
        self.sampleGroup._master = s0
        self.sampleGroup.unsetMaster()
        self.assertIsNone(self.sampleGroup._master)

    def test_getMaster(self):
        s0 = mock.Mock()
        s1 = mock.Mock()
        self.sampleGroup._samples = [s0, s1]
        self.sampleGroup._master = s0
        self.assertEqual(self.sampleGroup.getMaster(), s0)


if __name__ == "__main__":
    unittest.main()
