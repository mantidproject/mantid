# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import MagicMock, patch
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.presenter import EngineeringDiffractionPresenter

presenter_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.presenter"


class EngineeringDiffractionPresenterTest(unittest.TestCase):
    def setUp(self):
        self.Presenter = EngineeringDiffractionPresenter
        self.presenter = EngineeringDiffractionPresenter()

        # Simple view stub used by setup_* methods
        self.view = MagicMock()
        self.view.tabs = MagicMock()
        self.view.savedir_observer = MagicMock()

    def test_init_sets_expected_defaults(self):
        self.assertIsNone(self.presenter.calibration_presenter)
        self.assertIsNone(self.presenter.focus_presenter)
        self.assertIsNone(self.presenter.correction_presenter)
        self.assertIsNone(self.presenter.fitting_presenter)
        self.assertIsNone(self.presenter.texture_presenter)
        self.assertIsNone(self.presenter.settings_presenter)
        self.assertIsNone(self.presenter.gsas2_presenter)

        # Observable attributes exist
        self.assertTrue(hasattr(self.presenter, "statusbar_observable"))
        self.assertTrue(hasattr(self.presenter, "savedir_observable"))

    @patch(presenter_path + ".CalibrationPresenter")
    @patch(presenter_path + ".FocusPresenter")
    @patch(presenter_path + ".CalibrationView")
    @patch(presenter_path + ".FocusModel")
    @patch(presenter_path + ".CalibrationModel")
    def test_setup_calibration_creates_presenters_and_adds_tab(self, CalModel, FocusModel, CalView, FocusPresenter, CalPresenter):
        cal_model = MagicMock()
        focus_model = MagicMock()
        cal_view = MagicMock()

        CalModel.return_value = cal_model
        FocusModel.return_value = focus_model
        CalView.return_value = cal_view

        cal_presenter = MagicMock()
        focus_presenter = MagicMock()
        CalPresenter.return_value = cal_presenter
        FocusPresenter.return_value = focus_presenter

        self.presenter.setup_calibration(self.view)

        self.assertIs(self.presenter.calibration_presenter, cal_presenter)
        self.assertIs(self.presenter.focus_presenter, focus_presenter)

        # View gets a new tab
        self.view.tabs.addTab.assert_called_once_with(cal_view, "Run Processing")

        # Presenter constructors called correctly
        CalPresenter.assert_called_once_with(cal_model, cal_view)
        FocusPresenter.assert_called_once_with(focus_model, cal_view)

    def test_setup_calibration_notifier_subscribes_focus_and_main_observer(self):
        self.presenter.calibration_presenter = MagicMock()
        self.presenter.focus_presenter = MagicMock()

        notifier = MagicMock()
        self.presenter.calibration_presenter.calibration_notifier = notifier
        self.presenter.focus_presenter.calibration_observer = MagicMock()

        self.presenter.setup_calibration_notifier()

        # Should add both observers as subscribers
        notifier.add_subscriber.assert_any_call(self.presenter.focus_presenter.calibration_observer)
        notifier.add_subscriber.assert_any_call(self.presenter.calibration_observer)
        self.assertEqual(notifier.add_subscriber.call_count, 2)

    @patch(presenter_path + ".TextureCorrectionPresenter")
    @patch(presenter_path + ".TextureCorrectionView")
    @patch(presenter_path + ".CorrectionModel")
    def test_setup_correction_creates_presenter_adds_tab_and_subscribes(
        self, CorrectionModel, TextureCorrectionView, TextureCorrectionPresenter
    ):
        # mock correction_observer
        self.presenter.focus_presenter = MagicMock()
        self.presenter.focus_presenter.correction_observer = MagicMock()

        correction_model = MagicMock()
        correction_view = MagicMock()
        correction_presenter = MagicMock()

        CorrectionModel.return_value = correction_model
        TextureCorrectionView.return_value = correction_view
        TextureCorrectionPresenter.return_value = correction_presenter

        self.presenter.setup_correction(self.view)

        self.assertIs(self.presenter.correction_presenter, correction_presenter)
        self.view.tabs.addTab.assert_called_once_with(correction_view, "Absorption Correction")
        correction_presenter.add_correction_subscriber.assert_called_once_with(self.presenter.focus_presenter.correction_observer)

    @patch(presenter_path + ".FittingPresenter")
    @patch(presenter_path + ".FittingView")
    def test_setup_fitting_creates_presenter_adds_tab_and_registers_focus_observers(self, FittingView, FittingPresenter):
        self.presenter.focus_presenter = MagicMock()

        fitting_view = MagicMock()
        fitting_presenter = MagicMock()
        fitting_presenter.data_widget.presenter.focus_run_observer = MagicMock()
        fitting_presenter.data_widget.presenter.focus_combined_observer = MagicMock()

        FittingView.return_value = fitting_view
        FittingPresenter.return_value = fitting_presenter

        self.presenter.setup_fitting(self.view)

        self.assertIs(self.presenter.fitting_presenter, fitting_presenter)
        self.view.tabs.addTab.assert_called_once_with(fitting_view, "Fitting")
        self.presenter.focus_presenter.add_focus_subscriber.assert_called_once_with(
            fitting_presenter.data_widget.presenter.focus_run_observer
        )
        self.presenter.focus_presenter.add_focus_texture_subscriber.assert_called_once_with(
            fitting_presenter.data_widget.presenter.focus_combined_observer
        )

    @patch(presenter_path + ".TexturePresenter")
    @patch(presenter_path + ".TextureView")
    @patch(presenter_path + ".ProjectionModel")
    def test_setup_texture_creates_presenter_adds_tab_and_subscribes_focus(self, ProjectionModel, TextureView, TexturePresenter):
        self.presenter.focus_presenter = MagicMock()
        texture_model = MagicMock()
        texture_view = MagicMock()
        texture_presenter = MagicMock()
        texture_presenter.focus_run_observer = MagicMock()

        ProjectionModel.return_value = texture_model
        TextureView.return_value = texture_view
        TexturePresenter.return_value = texture_presenter

        self.presenter.setup_texture(self.view)

        self.assertIs(self.presenter.texture_presenter, texture_presenter)
        self.presenter.focus_presenter.add_focus_texture_subscriber.assert_called_once_with(texture_presenter.focus_run_observer)
        self.view.tabs.addTab.assert_called_once_with(texture_view, "Texture")

    @patch(presenter_path + ".GSAS2Presenter")
    @patch(presenter_path + ".GSAS2View")
    @patch(presenter_path + ".GSAS2Model")
    def test_setup_gsas2_creates_presenter_adds_tab_and_subscribes(self, GSAS2Model, GSAS2View, GSAS2Presenter):
        self.presenter.focus_presenter = MagicMock()
        self.presenter.calibration_presenter = MagicMock()

        model = MagicMock()
        view = MagicMock()
        presenter = MagicMock()
        presenter.focus_run_observer_gsas2 = MagicMock()
        presenter.prm_filepath_observer_gsas2 = MagicMock()

        GSAS2Model.return_value = model
        GSAS2View.return_value = view
        GSAS2Presenter.return_value = presenter

        self.presenter.setup_gsas2(self.view)

        self.assertIs(self.presenter.gsas2_presenter, presenter)
        self.presenter.focus_presenter.add_focus_gsas2_subscriber.assert_called_once_with(presenter.focus_run_observer_gsas2)
        self.presenter.calibration_presenter.add_prm_gsas2_subscriber.assert_called_once_with(presenter.prm_filepath_observer_gsas2)
        self.view.tabs.addTab.assert_called_once_with(view, "GSAS II")

    @patch(presenter_path + ".SettingsPresenter")
    @patch(presenter_path + ".SettingsView")
    @patch(presenter_path + ".SettingsModel")
    def test_setup_settings_creates_presenter_loads_settings_and_setup_savedir_observer(
        self, SettingsModel, SettingsView, SettingsPresenter
    ):
        model = MagicMock()
        view = MagicMock()
        presenter = MagicMock()
        presenter.savedir_notifier = MagicMock()

        SettingsModel.return_value = model
        SettingsView.return_value = view
        SettingsPresenter.return_value = presenter

        self.presenter.setup_settings(self.view)

        self.assertIs(self.presenter.settings_presenter, presenter)
        presenter.load_settings_from_file_or_default.assert_called_once_with()
        presenter.savedir_notifier.add_subscriber.assert_called_once_with(self.view.savedir_observer)

    def test_setup_savedir_notifier(self):
        self.presenter.settings_presenter = MagicMock()
        notifier = MagicMock()
        self.presenter.settings_presenter.savedir_notifier = notifier

        self.presenter.setup_savedir_notifier(self.view)

        notifier.add_subscriber.assert_called_once_with(self.view.savedir_observer)

    def test_handle_close_calls_expected_cleanup(self):
        self.presenter.fitting_presenter = MagicMock()
        data_widget = self.presenter.fitting_presenter.data_widget
        plot_widget = self.presenter.fitting_presenter.plot_widget

        data_widget.ads_observer = MagicMock()
        data_widget.view = MagicMock()
        plot_widget.view = MagicMock()

        self.presenter.handle_close()

        data_widget.ads_observer.unsubscribe.assert_called_once_with()
        data_widget.view.saveSettings.assert_called_once_with()
        plot_widget.view.ensure_fit_dock_closed.assert_called_once_with()

    @patch(presenter_path + ".InterfaceManager")
    def test_open_help_window_calls_interface_manager(self, InterfaceManager):
        mgr = MagicMock()
        InterfaceManager.return_value = mgr

        self.presenter.open_help_window()

        mgr.showCustomInterfaceHelp.assert_called_once_with(self.presenter.doc, self.presenter.doc_folder)

    def test_open_settings_shows_settings_presenter(self):
        self.presenter.settings_presenter = MagicMock()
        self.presenter.open_settings()
        self.presenter.settings_presenter.show.assert_called_once_with()

    def test_update_calibration_with_calibration_notifies_statusbar(self):
        calibration = MagicMock()
        calibration.get_instrument.return_value = "ENGINX"
        calibration.get_ceria_runno.return_value = "123"
        calibration.get_vanadium_runno.return_value = "456"

        with patch.object(self.presenter.statusbar_observable, "notify_subscribers") as notify:
            self.presenter.update_calibration(calibration)

        notify.assert_called_once_with("CeO2: 123, V: 456, Instrument: ENGINX")

    def test_update_calibration_with_none_notifies_no_calibration(self):
        with patch.object(self.presenter.statusbar_observable, "notify_subscribers") as notify:
            self.presenter.update_calibration(None)

        notify.assert_called_once_with("No Calibration Loaded")

    @patch(presenter_path + ".get_setting")
    @patch(presenter_path + ".output_settings")
    def test_get_saved_rb_number_calls_get_setting(self, output_settings, get_setting):
        output_settings.INTERFACES_SETTINGS_GROUP = "G"
        output_settings.ENGINEERING_PREFIX = "P/"
        get_setting.return_value = "RB123"

        rb = self.presenter.get_saved_rb_number()

        self.assertEqual(rb, "RB123")
        get_setting.assert_called_once_with("G", "P/", "rb_number")

    @patch(presenter_path + ".set_setting")
    @patch(presenter_path + ".output_settings")
    def test_set_saved_rb_number_calls_set_setting(self, output_settings, set_setting):
        output_settings.INTERFACES_SETTINGS_GROUP = "G"
        output_settings.ENGINEERING_PREFIX = "P/"

        self.presenter.set_saved_rb_number("RB999")

        set_setting.assert_called_once_with("G", "P/", "rb_number", "RB999")


if __name__ == "__main__":
    unittest.main()
