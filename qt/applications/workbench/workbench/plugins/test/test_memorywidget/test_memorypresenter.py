# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from workbench.plugins.memorywidget.memoryview import MemoryView, \
    from_normal_to_critical, from_critical_to_normal
from workbench.plugins.memorywidget.memorypresenter import MemoryPresenter

import unittest
from unittest import mock


class MemoryPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(MemoryView)

        self.view.critical = 90
        self.view.memory_bar = mock.Mock()
        self.view.set_bar_color = mock.Mock()
        self.view.set_value = mock.Mock()

        self.presenter = MemoryPresenter(self.view)

    def test_presenter(self):
        self.assertTrue(from_normal_to_critical(self.presenter.view.critical,
                        75, 95))
        self.assertFalse(from_critical_to_normal(self.presenter.view.critical, 
                         75, 95))

        self.assertFalse(from_normal_to_critical(self.presenter.view.critical,
                         95, 75))
        self.assertTrue(from_critical_to_normal(self.presenter.view.critical,
                        95, 75))

        self.assertEqual(self.presenter.view.set_value.call_count, 1)

        self.presenter.update_memory_usage()
        self.assertEqual(self.presenter.view.set_value.call_count, 2)

if __name__ == "__main__":
    unittest.main()
