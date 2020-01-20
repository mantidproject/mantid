# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

import unittest

from mantid.py3compat.mock import Mock, patch
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.settings.fitting.presenter import FittingSettings


class MockSettingsView(object):
    def __init__(self):
        self.auto_bkg=StrictMock()
        self.background_args = StrictMock()
        self.default_peak = StrictMock()
        self.findpeaks_fwhm = StrictMock()
        self.findpeaks_tol = StrictMock()


class MockConfigService(object):
    def __init__(self):
        self.getString = StrictMock(return_value="1")
        self.setString = StrictMock()


@start_qapplication
class FittingSettingsTest(unittest.TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.settings.fitting.presenter.ConfigService"

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_signals(self, _):
        presenter = FittingSettings(None)

        self.assert_connected_once(presenter.view.auto_bkg,
                                   presenter.view.auto_bkg.stateChanged)

        self.assert_connected_once(presenter.view.default_peak,
                                   presenter.view.default_peak.stateChanged)

        self.assert_connected_once(presenter.view.background_args,
                                   presenter.view.background_args.stateChanged)

        self.assert_connected_once(presenter.view.findpeaks_fwhm,
                                   presenter.view.findpeaks_fwhm.stateChanged)

        self.assert_connected_once(presenter.view.findpeaks_tol,
                                   presenter.view.findpeaks_tol.stateChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_auto_background_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("None")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "None")

        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("Gaussian")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "Gaussian")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_background_args_changed(self, mock_ConfigService):
        mock_view = MockSettingsView()
        mock_view.auto_bkg.currentText = Mock(return_value="")
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("None")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "None")

        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("Gaussian")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "Gaussian")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_default_peak_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("None")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "None")

        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("Gaussian")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "Gaussian")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_find_peaks_fwhm_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("None")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "None")

        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("Gaussian")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "Gaussian")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_find_peaks_tolerance_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("None")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "None")

        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("Gaussian")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "Gaussian")