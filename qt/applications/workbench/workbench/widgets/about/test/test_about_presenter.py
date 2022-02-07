# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
import unittest
from unittest import TestCase
from unittest.mock import call, Mock, patch

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.about.presenter import AboutPresenter


class MockInstrument(object):
    def __init__(self, idx):
        self.name = StrictMock(return_value="instr{}".format(idx))


class MockFacility(object):
    def __init__(self, name):
        self.name = StrictMock(return_value=name)
        self.all_instruments = [MockInstrument(0), MockInstrument(1)]
        self.instruments = StrictMock(return_value=self.all_instruments)


class MockConfigService(object):
    all_facilities = ["facility1", "facility2"]

    def __init__(self):
        self.mock_facility = MockFacility(self.all_facilities[0])
        self.mock_instrument = self.mock_facility.all_instruments[0]
        self.getFacilityNames = StrictMock(return_value=self.all_facilities)
        self.getFacility = StrictMock(return_value=self.mock_facility)
        self.getInstrument = StrictMock(return_value=self.mock_instrument)
        self.getString = StrictMock(return_value="FACILITY1")
        self.setFacility = StrictMock()
        self.setString = StrictMock()


class FakeVersionInfo(object):
    def __init__(self):
        self.major = "the same"
        self.minor = "every time"


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
        elif p_str == AboutPresenter.PREVIOUS_VERSION:
            return self.string_value
        else:
            return "unknown p_str"


@start_qapplication
class AboutPresenterTest(TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.about.presenter.ConfigService"
    QSETTINGS_CLASSPATH = "workbench.widgets.about.presenter.QSettings"
    RELEASE_NOTES_URL_CLASSPATH = "workbench.widgets.about.presenter.release_notes_url"
    VERSION_INFO_CLASSPATH = "workbench.widgets.about.presenter.version"

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_should_show_on_startup_no_facility(self, MockConfigService):
        MockConfigService.getString.return_value = ""
        self.assertTrue(AboutPresenter.should_show_on_startup(),
                        "If the facilty is not set then should_show_on_startup should always be true")
        MockConfigService.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                      call(AboutPresenter.INSTRUMENT)])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_should_show_on_startup_invalid_facility(self, MockConfigService):
        MockConfigService.getFacility.side_effect = RuntimeError("Invalid Facility name")
        self.assertTrue(AboutPresenter.should_show_on_startup(),
                        "If the facilty is invalid then should_show_on_startup should always be true")
        MockConfigService.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                      call(AboutPresenter.INSTRUMENT)])
        MockConfigService.getFacility.assert_has_calls([call("FACILITY1")])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_should_show_on_startup_invalid_instrument(self, MockConfigService):
        MockConfigService.getInstrument.side_effect = RuntimeError("Invalid Instrument name")
        self.assertTrue(AboutPresenter.should_show_on_startup(),
                        "If the instrument is invalid then should_show_on_startup should always be true")
        MockConfigService.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                      call(AboutPresenter.INSTRUMENT)])
        MockConfigService.getFacility.assert_has_calls([call("FACILITY1")])
        MockConfigService.getInstrument.assert_has_calls([call("FACILITY1")])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_should_show_on_startup_do_not_show_same_version(self, MockConfigService):
        version_str = "the same.every time"
        with patch(self.QSETTINGS_CLASSPATH, return_value=FakeQSettings(version_str)):
            with patch(self.VERSION_INFO_CLASSPATH, return_value=FakeVersionInfo()):
                self.assertFalse(AboutPresenter.should_show_on_startup(),
                                 "If do not show is in Qsettings then should_show_on_startup should always be False"
                                 + "for the same version")
        MockConfigService.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                      call(AboutPresenter.INSTRUMENT)])
        MockConfigService.getFacility.assert_has_calls([call("FACILITY1")])
        MockConfigService.getInstrument.assert_has_calls([call("FACILITY1")])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_should_show_on_startup_do_not_show_different_versions(self, MockConfigService):
        version_str = "the same every time"
        with patch(self.QSETTINGS_CLASSPATH, return_value=FakeQSettings(version_str)):
            with patch(self.RELEASE_NOTES_URL_CLASSPATH, return_value="not the " + version_str):
                self.assertTrue(AboutPresenter.should_show_on_startup(),
                                "If do not show is in Qsettings then should_show_on_startup should always be True"
                                + " for different versions")
        MockConfigService.getString.assert_has_calls([call(AboutPresenter.FACILITY),
                                                      call(AboutPresenter.INSTRUMENT)])
        MockConfigService.getFacility.assert_has_calls([call("FACILITY1")])
        MockConfigService.getInstrument.assert_has_calls([call("FACILITY1")])

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_facilities_with_valid_combination(self, mock_ConfigService):
        self.assertEqual(0, mock_ConfigService.mock_instrument.name.call_count)
        presenter = AboutPresenter(None)
        self.assertEqual(0, mock_ConfigService.setFacility.call_count)
        self.assertEqual(3, mock_ConfigService.getFacility.call_count)
        self.assertEqual(3, mock_ConfigService.mock_facility.name.call_count)
        self.assert_connected_once(presenter.view.about_widget.cb_facility,
                                   presenter.view.about_widget.cb_facility.currentTextChanged)

    def test_setup_checkbox_signals(self):
        presenter = AboutPresenter(None)
        about_widget = presenter.view.about_widget

        self.assert_connected_once(about_widget.chk_do_not_show_until_next_release,
                                   presenter.view.about_widget.chk_do_not_show_until_next_release.stateChanged)

        self.assert_connected_once(about_widget.chk_allow_usage_data,
                                   about_widget.chk_allow_usage_data.stateChanged)

    def test_setup_button_signals(self):
        presenter = AboutPresenter(None)
        about_widget = presenter.view.about_widget

        self.assert_connected_once(about_widget.clb_release_notes,
                                   about_widget.clb_release_notes.clicked)
        self.assert_connected_once(about_widget.clb_sample_datasets,
                                   about_widget.clb_sample_datasets.clicked)
        self.assert_connected_once(about_widget.clb_mantid_introduction,
                                   about_widget.clb_mantid_introduction.clicked)
        self.assert_connected_once(about_widget.clb_python_introduction,
                                   about_widget.clb_python_introduction.clicked)
        self.assert_connected_once(about_widget.clb_python_in_mantid,
                                   about_widget.clb_python_in_mantid.clicked)
        self.assert_connected_once(about_widget.clb_extending_mantid,
                                   about_widget.clb_extending_mantid.clicked)
        self.assert_connected_once(about_widget.pb_manage_user_directories,
                                   about_widget.pb_manage_user_directories.clicked)
        self.assert_connected_once(about_widget.lbl_privacy_policy,
                                   about_widget.lbl_privacy_policy.linkActivated)

    def test_setup_link_signals(self):
        presenter = AboutPresenter(None)
        about_widget = presenter.view.about_widget

        self.assert_connected_once(about_widget.clb_release_notes,
                                   about_widget.clb_release_notes.clicked)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_that_about_presenter_is_instantiated_without_error_when_getFacility_causes_exception(self,
                                                                                                  MockConfigService):
        MockConfigService.getFacility.side_effect = RuntimeError(Mock(status=101), "No facility")
        presenter = AboutPresenter(None)

        self.assertEqual(3, MockConfigService.getFacility.call_count)
        self.assertEqual(presenter._get_current_facility(), None)


if __name__ == "__main__":
    unittest.main()
