# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
import time

from mantidqt.widgets.memorywidget.memoryview import MemoryView, from_normal_to_critical, from_critical_to_normal
from mantidqt.widgets.memorywidget.memorypresenter import MemoryPresenter, TIME_INTERVAL_MEMORY_USAGE_UPDATE

import unittest
from unittest import mock


class MemoryPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(MemoryView, instance=True)
        self.mock_view_internals()
        self.presenter = MemoryPresenter(self.view)

    def tearDown(self):
        self.presenter.cancel_memory_update()

    def mock_view_internals(self):
        self.view.critical = 90
        self.view.memory_bar = mock.Mock()
        self.view.memory_bar.value.return_value = 0
        self.view.set_bar_color = mock.Mock()
        self.view.invoke_set_value = mock.Mock()
        self.view.mantid_memory_bar = mock.Mock()
        self.view.mantid_memory_bar.value.return_value = 0
        self.view.set_mantid_bar_color = mock.Mock()
        self.view.invoke_mantid_set_value = mock.Mock()

    def test_presenter(self):
        self.presenter.cancel_memory_update()
        self.assertTrue(from_normal_to_critical(self.presenter.view.critical, 75, 95))
        self.assertFalse(from_critical_to_normal(self.presenter.view.critical, 75, 95))
        self.assertFalse(from_normal_to_critical(self.presenter.view.critical, 95, 75))
        self.assertTrue(from_critical_to_normal(self.presenter.view.critical, 95, 75))

        self.assertEqual(self.presenter.view.set_bar_color.call_count, 1)

    def test_memory_usage_is_updated_based_on_a_constant(self):
        # Sleep for just longer than the default so the test can run
        time.sleep(TIME_INTERVAL_MEMORY_USAGE_UPDATE + 0.5)
        self.assertGreater(self.view.invoke_set_value.call_count, 1)
        self.assertGreater(self.view.invoke_mantid_set_value.call_count, 1)


if __name__ == "__main__":
    unittest.main()
