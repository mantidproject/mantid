import unittest
import sys
from mantid.kernel import ConfigService, ConfigServiceObserver

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

    
class ConfigServiceObserverTest(unittest.TestCase):
    def setUp(self):
        self._search_directories = ConfigService.Instance().getString("datasearch.directories")
        self._default_save_directory = ConfigService.Instance().getString("defaultsave.directory")

    def tearDown(self):
        ConfigService.Instance().setString("datasearch.directories", self._search_directories)
        ConfigService.Instance().setString("defaultsave.directory", self._default_save_directory)

    def test_calls_for_default_save_change(self):
        onValueChangedMock = mock.Mock()

        class FakeConfigServiceObserver(ConfigServiceObserver):
            def onValueChanged(self, name, new, old):
                onValueChangedMock(name, new, old)

        observer = FakeConfigServiceObserver()
        ConfigService.Instance().setString("defaultsave.directory", "/dev/null")
        onValueChangedMock.assert_any_call("defaultsave.directory", mock.ANY, mock.ANY)
        onValueChangedMock.assert_any_call("datasearch.directories", mock.ANY, mock.ANY)


if __name__ == '__main__':
    unittest.main()
