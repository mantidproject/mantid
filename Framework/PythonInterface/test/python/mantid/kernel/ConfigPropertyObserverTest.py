# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import ConfigService, ConfigPropertyObserver
from unittest import mock


class ConfigObserverTest(unittest.TestCase):
    def setUp(self):
        self.config = ConfigService.Instance()
        self._search_directories = self.config.getString("datasearch.directories")
        self._default_save_directory = self.config.getString("defaultsave.directory")
        self._retained_algorithms = self.config.getString("algorithms.retained")

    def tearDown(self):
        self.config.setString("datasearch.directories", self._search_directories)
        self.config.setString("defaultsave.directory", self._default_save_directory)
        self.config.setString("algorithms.retained", self._retained_algorithms)

    def test_calls_for_default_save_change(self):
        onPropertyValueChangedMock = mock.Mock()

        class FakeConfigPropertyObserver(ConfigPropertyObserver):
            def __init__(self, property):
                super(FakeConfigPropertyObserver, self).__init__("defaultsave.directory")

            def onPropertyValueChanged(self, new, old):
                onPropertyValueChangedMock(new, old)

        new_save_directory_value = "/dev/null"
        self.observer = FakeConfigPropertyObserver("defaultsave.directory")
        self.config.setString("defaultsave.directory", new_save_directory_value)
        self.config.setString("algorithms.retained", "60")
        onPropertyValueChangedMock.assert_called_once_with(new_save_directory_value, mock.ANY)


if __name__ == "__main__":
    unittest.main()
