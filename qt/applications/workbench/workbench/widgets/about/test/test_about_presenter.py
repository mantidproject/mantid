# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from unittest import TestCase

from unittest.mock import call, patch
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


@start_qapplication
class AboutPresenterTest(TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.about.presenter.ConfigService"
    QSETTINGS_CLASSPATH = "workbench.widgets.about.presenter.QSettings"
    RELEASE_NOTES_URL_CLASSPATH = "workbench.widgets.about.presenter.release_notes_url"

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
        version_str = "the same every time"
        with patch(self.QSETTINGS_CLASSPATH, return_value = FakeQSettings(version_str)):
            with patch(self.RELEASE_NOTES_URL_CLASSPATH, return_value = version_str):
                self.assertFalse(AboutPresenter.should_show_on_startup(),
                                 "If do not show is in Qsettings then should_show_on_startup should always be False" +
                                 "for the same version")
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
                                "If do not show is in Qsettings then should_show_on_startup should always be True" +
                                " for different versions")
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
        self.assertEqual(2, mock_ConfigService.mock_facility.name.call_count)
        self.assert_connected_once(presenter.view.cbFacility, presenter.view.cbFacility.currentTextChanged)

        mock_ConfigService.getInstrument.assert_called_once_with()
        self.assertEqual(2, mock_ConfigService.mock_instrument.name.call_count)
        self.assert_connected_once(presenter.view.cbInstrument, presenter.view.cbInstrument.currentTextChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_facilities_with_invalid_default_facility_chooses_first(self, mock_ConfigService):
        mock_ConfigService.getFacility.side_effect = [RuntimeError("Invalid facility name"),
                                                      mock_ConfigService.mock_facility,
                                                      mock_ConfigService.mock_facility]
        presenter = AboutPresenter(None)

        self.assertEqual(mock_ConfigService.mock_facility.name(),
                         presenter.view.cbFacility.currentText())
        self.assertEqual(mock_ConfigService.mock_instrument.name(),
                         presenter.view.cbInstrument.currentText())
        self.assert_connected_once(presenter.view.cbFacility, presenter.view.cbFacility.currentTextChanged)
        self.assert_connected_once(presenter.view.cbInstrument, presenter.view.cbInstrument.currentTextChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_facilities_with_invalid_default_instrument_chooses_first(self, mock_ConfigService):
        mock_ConfigService.getInstrument.side_effect = [RuntimeError("Invalid instrument name"),
                                                        mock_ConfigService.mock_instrument]
        presenter = AboutPresenter(None)

        self.assertEqual(mock_ConfigService.mock_instrument.name(),
                         presenter.view.cbInstrument.currentText())
        self.assert_connected_once(presenter.view.cbFacility, presenter.view.cbFacility.currentTextChanged)
        self.assert_connected_once(presenter.view.cbInstrument, presenter.view.cbInstrument.currentTextChanged)

    def test_setup_checkbox_signals(self):
        presenter = AboutPresenter(None)

        self.assert_connected_once(presenter.view.chkDoNotShowUntilNextRelease,
                                   presenter.view.chkDoNotShowUntilNextRelease.stateChanged)

        self.assert_connected_once(presenter.view.chkAllowUsageData,
                                   presenter.view.chkAllowUsageData.stateChanged)

    def test_setup_button_signals(self):
        presenter = AboutPresenter(None)

        self.assert_connected_once(presenter.view.clbReleaseNotes,
                                   presenter.view.clbReleaseNotes.clicked)
        self.assert_connected_once(presenter.view.clbSampleDatasets,
                                   presenter.view.clbSampleDatasets.clicked)
        self.assert_connected_once(presenter.view.clbMantidIntroduction,
                                   presenter.view.clbMantidIntroduction.clicked)
        self.assert_connected_once(presenter.view.clbPythonIntroduction,
                                   presenter.view.clbPythonIntroduction.clicked)
        self.assert_connected_once(presenter.view.clbPythonInMantid,
                                   presenter.view.clbPythonInMantid.clicked)
        self.assert_connected_once(presenter.view.clbExtendingMantid,
                                   presenter.view.clbExtendingMantid.clicked)
        self.assert_connected_once(presenter.view.pbMUD,
                                   presenter.view.pbMUD.clicked)
        self.assert_connected_once(presenter.view.lblPrivacyPolicy,
                                   presenter.view.lblPrivacyPolicy.linkActivated)

    def test_setup_link_signals(self):
        presenter = AboutPresenter(None)

        self.assert_connected_once(presenter.view.clbReleaseNotes,
                                   presenter.view.clbReleaseNotes.clicked)
