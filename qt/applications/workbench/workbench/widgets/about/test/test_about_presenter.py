# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from unittest import TestCase

from unittest.mock import call, Mock, patch
from mantidqt.utils.testing.mocks.mock_qt import MockQButton, MockQWidget
from mantidqt.utils.testing.strict_mock import StrictPropertyMock, StrictMock
from workbench.widgets.about.presenter import AboutPresenter

class MockConfigServiceNoFacility(object):
    def __init__(self):
        self.getString = StrictMock(return_value=None)

class MockConfigServiceInvalidFacility(object):
    def __init__(self):
        self.getString = StrictMock(return_value="FACILITY1")
        self.getFacility = StrictMock(return_value=None)
        self.getFacility.side_effect = self.side_effect_runtime_error
    def side_effect_runtime_error(self, string):
        raise RuntimeError("Not Found")

class MockConfigServiceInvalidInstrument(object):
    def __init__(self):
        self.getString = StrictMock(return_value="FACILITY1")
        self.getFacility = StrictMock(return_value=None)
        self.getInstrument = StrictMock(return_value=None)
        self.getInstrument.side_effect = self.side_effect_runtime_error
    def side_effect_runtime_error(self, string):
        raise RuntimeError("Not Found")

class MockConfigService(object):
    def __init__(self):
        self.getString = StrictMock(return_value="FACILITY1")
        self.getFacility = StrictMock(return_value=None)
        self.getInstrument = StrictMock(return_value=None)
        self.getInstrument.side_effect = self.side_effect_runtime_error
    def side_effect_runtime_error(self, string):
        raise RuntimeError("Not Found")

class FakeInfo(object):
    def __init__(self, name_value):
        self.name_value = name_value
    def name(self):
        return self.name_value

class MockConfigService(object):
    def __init__(self):
        self.getString = StrictMock(return_value="FACILITY1")
        self.getFacility = StrictMock(return_value=FakeInfo("FACILITY1"))
        self.getInstrument = StrictMock(return_value=FakeInfo("INST1"))

class FakeQSettings(object):
    def __init__(self, string_value):
        self.string_value = string_value
        self.beginGroup = StrictMock()
        self.value = StrictMock()
        self.value.side_effect = self.value_depending_on_str
        self.endGroup = StrictMock()

    def value_depending_on_str(self, p_str, defaultValue=None, type=None):
        if p_str == AboutPresenter.DO_NOT_SHOW:
            return "2"
        elif p_str == AboutPresenter.LAST_VERSION:
            return self.string_value
        else:
            return "unknown p_str"

def MockQSettings():
    return FakeQSettings

class FakeMVP(object):
    def __init__(self):
        self.view = MockQWidget()


class FakeSectionsListWidget:
    def __init__(self):
        self.fake_items = []

    def addItems(self, item):
        self.fake_items.extend(item)

    def item(self, index):
        return self.fake_items[index]


class MockAboutView(object):
    def __init__(self):
        self.mock_container = MockQWidget()
        self.mock_current = MockQWidget()
        self.container = StrictPropertyMock(return_value=self.mock_container)
        self.current = StrictPropertyMock(return_value=self.mock_current)
        self.sections = FakeSectionsListWidget()
        self.general_settings = FakeMVP()
        self.categories_settings = FakeMVP()
        self.plot_settings = FakeMVP()
        self.fitting_settings = FakeMVP()
        self.save_settings_button = MockQButton()
        self.help_button = MockQButton()


class AboutPresenterTest(TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.about.presenter.ConfigService"
    QSETTINGS_CLASSPATH = "workbench.widgets.about.presenter.QSettings"
    RELEASE_NOTES_URL_CLASSPATH = "workbench.widgets.about.presenter.release_notes_url"

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigServiceNoFacility)
    def test_should_show_on_startup_no_facility(self, MockConfigServiceNoFacility):
        self.assertTrue(AboutPresenter.should_show_on_startup(),
                        "If the facilty is not set then should_show_on_startup should always be true")
        MockConfigServiceNoFacility.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                                call(AboutPresenter.INSTRUMENT)])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigServiceInvalidFacility)
    def test_should_show_on_startup_invalid_facility(self, MockConfigServiceInvalidFacility):
        self.assertTrue(AboutPresenter.should_show_on_startup(),
                        "If the facilty is invalid then should_show_on_startup should always be true")
        MockConfigServiceInvalidFacility.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                                call(AboutPresenter.INSTRUMENT)])
        MockConfigServiceInvalidFacility.getFacility.assert_has_calls([call("FACILITY1")])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigServiceInvalidInstrument)
    def test_should_show_on_startup_invalid_instrument(self, MockConfigServiceInvalidInstrument):
        self.assertTrue(AboutPresenter.should_show_on_startup(),
                        "If the instrument is invalid then should_show_on_startup should always be true")
        MockConfigServiceInvalidInstrument.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                                call(AboutPresenter.INSTRUMENT)])
        MockConfigServiceInvalidInstrument.getFacility.assert_has_calls([call("FACILITY1")])
        MockConfigServiceInvalidInstrument.getInstrument.assert_has_calls([call("FACILITY1")])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_should_show_on_startup_do_not_show_same_version(self, MockConfigService):
        version_str = "the same every time"
        with patch(self.QSETTINGS_CLASSPATH, return_value = FakeQSettings(version_str)):
            with patch(self.RELEASE_NOTES_URL_CLASSPATH, return_value = version_str):
                self.assertFalse(AboutPresenter.should_show_on_startup(),
                                 "If do not show is in Qsettings then should_show_on_startup should always be False for the same version")
        MockConfigService.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                                call(AboutPresenter.INSTRUMENT)])
        MockConfigService.getFacility.assert_has_calls([call("FACILITY1")])
        MockConfigService.getInstrument.assert_has_calls([call("FACILITY1")])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_should_show_on_startup_do_not_show_different_versions(self, MockConfigService):
        version_str = "the same every time"
        with patch(self.QSETTINGS_CLASSPATH, return_value = FakeQSettings(version_str)):
            with patch(self.RELEASE_NOTES_URL_CLASSPATH, return_value = "not the " + version_str):
                self.assertTrue(AboutPresenter.should_show_on_startup(),
                                 "If do not show is in Qsettings then should_show_on_startup should always be True for  different versions")
        MockConfigService.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                                call(AboutPresenter.INSTRUMENT)])
        MockConfigService.getFacility.assert_has_calls([call("FACILITY1")])
        MockConfigService.getInstrument.assert_has_calls([call("FACILITY1")])
