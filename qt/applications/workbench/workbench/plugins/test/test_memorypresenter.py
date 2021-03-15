# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from workbench.plugins.memoryview import MemoryView
from workbench.plugins.memoryview import fromNormalToCritical, fromCriticalToNormal
from workbench.plugins.memorypresenter import MemoryPresenter


import unittest
from unittest import mock

class MemoryPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(MemoryView)

        self.view.updateSignal = mock.Mock()
        self.view.critical = 90
        self.view.memory_bar = mock.Mock()
        self.view.setBarColor = mock.Mock()
        self.view.setValue = mock.Mock()
        self.view.onUpdateRequest = mock.Mock()

        self.presenter = MemoryPresenter(self.view)

    def test_doSomething(self):
        self.assertTrue(fromNormalToCritical(self.presenter.view.critical,
                        75, 95))
        self.assertFalse(fromCriticalToNormal(self.presenter.view.critical, 
                         75, 95))

        self.assertFalse(fromNormalToCritical(self.presenter.view.critical,
                         95, 75))
        self.assertTrue(fromCriticalToNormal(self.presenter.view.critical,
                        95, 75))

        self.assertEqual(self.presenter.view.setValue.call_count, 1)

        self.view.memory_bar.value.retun_value = 95
        self.presenter.view.memory_bar.value.retun_value = 95

        self.presenter.updateMemoryUsage()
        self.assertEqual(self.presenter.view.setValue.call_count, 2)


if __name__ == "__main__":
    unittest.main()
