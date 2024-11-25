# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import ConfigService, ConfigObserver
from unittest import mock


class ConfigObserverTest(unittest.TestCase):
    def setUp(self):
        self._search_directories = ConfigService.Instance().getString("datasearch.directories")
        self._default_save_directory = ConfigService.Instance().getString("defaultsave.directory")

    def tearDown(self):
        ConfigService.Instance().setString("datasearch.directories", self._search_directories)
        ConfigService.Instance().setString("defaultsave.directory", self._default_save_directory)

    def test_calls_for_default_save_change(self):
        onValueChangedMock = mock.Mock()

        class FakeConfigObserver(ConfigObserver):
            def onValueChanged(self, name, new, old):
                onValueChangedMock(name, new, old)

        self.observer = FakeConfigObserver()
        ConfigService.Instance().setString("defaultsave.directory", "/dev/null")
        onValueChangedMock.assert_any_call("defaultsave.directory", mock.ANY, mock.ANY)
        onValueChangedMock.assert_any_call("datasearch.directories", mock.ANY, mock.ANY)


if __name__ == "__main__":
    unittest.main()
