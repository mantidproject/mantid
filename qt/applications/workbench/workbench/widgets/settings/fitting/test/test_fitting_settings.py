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
        self.auto_bkg = Mock()
        self.background_args = Mock()
        self.default_peak = Mock()
        self.findpeaks_fwhm = Mock()
        self.findpeaks_tol = Mock()


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
                                   presenter.view.auto_bkg.currentTextChanged)

        self.assert_connected_once(presenter.view.default_peak,
                                   presenter.view.default_peak.currentTextChanged)

        self.assert_connected_once(presenter.view.background_args,
                                   presenter.view.background_args.editingFinished)

        self.assert_connected_once(presenter.view.findpeaks_fwhm,
                                   presenter.view.findpeaks_fwhm.valueChanged)

        self.assert_connected_once(presenter.view.findpeaks_tol,
                                   presenter.view.findpeaks_tol.valueChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_auto_background_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("None")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "")

        mock_ConfigService.setString.reset_mock()

        presenter.action_auto_background_changed("Polynomial")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "Polynomial ")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_background_args_changed(self, mock_ConfigService):
        mock_view = MockSettingsView()
        mock_view.auto_bkg.currentText = Mock(return_value="Polynomial")
        presenter = FittingSettings(None, mock_view)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        mock_view.background_args.text = Mock(return_value="n=3")
        presenter.action_background_args_changed()
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "Polynomial n=3")

        mock_ConfigService.setString.reset_mock()

        mock_view.background_args.text = Mock(return_value="n=5")
        presenter.action_background_args_changed()
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND,
                                                             "Polynomial n=5")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_background_args_changed_with_auto_background_none(self, mock_ConfigService):
        mock_view = MockSettingsView()
        mock_view.auto_bkg.currentText = Mock(return_value="None")
        presenter = FittingSettings(None, mock_view)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        mock_view.background_args.text = Mock(return_value="n=3")
        presenter.action_background_args_changed()
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.AUTO_BACKGROUND, "")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_default_peak_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_default_peak_changed("None")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.DEFAULT_PEAK, "None")

        mock_ConfigService.setString.reset_mock()

        presenter.action_default_peak_changed("Gaussian")
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.DEFAULT_PEAK, "Gaussian")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_find_peaks_fwhm_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_find_peaks_fwhm_changed(5)
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.FWHM, "5")

        mock_ConfigService.setString.reset_mock()

        presenter.action_find_peaks_fwhm_changed(9)
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.FWHM, "9")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_find_peaks_tolerance_changed(self, mock_ConfigService):
        presenter = FittingSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_find_peaks_tolerance_changed(3)
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.TOLERANCE, "3")

        mock_ConfigService.setString.reset_mock()

        presenter.action_find_peaks_tolerance_changed(8)
        mock_ConfigService.setString.assert_called_once_with(FittingSettings.TOLERANCE, "8")
