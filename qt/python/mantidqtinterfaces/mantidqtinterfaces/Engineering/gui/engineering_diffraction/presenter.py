# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from .tabs.common import CalibrationObserver, output_settings
from .tabs.calibration.model import CalibrationModel
from .tabs.calibration.view import CalibrationView
from .tabs.calibration.presenter import CalibrationPresenter
from .tabs.focus.model import FocusModel
from .tabs.focus.view import FocusView
from .tabs.focus.presenter import FocusPresenter
from .tabs.correction.model import CorrectionModel
from .tabs.correction.view import TextureCorrectionView
from .tabs.correction.presenter import TextureCorrectionPresenter
from .tabs.fitting.view import FittingView
from .tabs.fitting.presenter import FittingPresenter
from .tabs.gsas2.model import GSAS2Model
from .tabs.gsas2.presenter import GSAS2Presenter
from .tabs.gsas2.view import GSAS2View
from .tabs.texture.model import ProjectionModel
from .tabs.texture.view import TextureView
from .tabs.texture.presenter import TexturePresenter
from .settings.settings_model import SettingsModel
from .settings.settings_view import SettingsView
from .settings.settings_presenter import SettingsPresenter
from .settings.settings_helper import get_setting, set_setting

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.observer_pattern import GenericObservable


class EngineeringDiffractionPresenter(object):
    def __init__(self):
        self.calibration_presenter = None
        self.correction_presenter = None
        self.focus_presenter = None
        self.fitting_presenter = None
        self.texture_presenter = None
        self.settings_presenter = None
        self.gsas2_presenter = None

        self.doc_folder = "diffraction"
        self.doc = "Engineering Diffraction"

        # Setup observers
        self.calibration_observer = CalibrationObserver(self)

        # Setup observables
        self.statusbar_observable = GenericObservable()
        self.savedir_observable = GenericObservable()

    # the following setup functions should only be called from the view, this ensures both that the presenter object
    # itself doesn't own the view (vice versa in this instance) and that the 'child' tabs of the presenter can be mocked
    # /subbed in for other purposes i.e. testing, agnostic of the view

    def setup_calibration(self, view):
        cal_model = CalibrationModel()
        cal_view = CalibrationView(parent=view.tabs)
        self.calibration_presenter = CalibrationPresenter(cal_model, cal_view)
        view.tabs.addTab(cal_view, "Calibration")

    def setup_calibration_notifier(self):
        self.calibration_presenter.calibration_notifier.add_subscriber(self.focus_presenter.calibration_observer)
        self.calibration_presenter.calibration_notifier.add_subscriber(self.calibration_observer)

    def setup_correction(self, view):
        correction_model = CorrectionModel()
        correction_view = TextureCorrectionView()
        self.correction_presenter = TextureCorrectionPresenter(correction_model, correction_view)
        view.tabs.addTab(correction_view, "Absorption Correction")

    def setup_focus(self, view):
        focus_model = FocusModel()
        focus_view = FocusView()
        self.focus_presenter = FocusPresenter(focus_model, focus_view)
        self.correction_presenter.add_correction_subscriber(self.focus_presenter.correction_observer)
        view.tabs.addTab(focus_view, "Focus")

    def setup_fitting(self, view):
        fitting_view = FittingView()
        self.fitting_presenter = FittingPresenter(fitting_view)
        self.focus_presenter.add_focus_subscriber(self.fitting_presenter.data_widget.presenter.focus_run_observer)
        self.focus_presenter.add_focus_texture_subscriber(self.fitting_presenter.data_widget.presenter.focus_combined_observer)
        view.tabs.addTab(fitting_view, "Fitting")

    def setup_gsas2(self, view):
        gsas2_model = GSAS2Model()
        gsas2_view = GSAS2View(parent=view.tabs)
        self.gsas2_presenter = GSAS2Presenter(gsas2_model, gsas2_view)
        self.focus_presenter.add_focus_gsas2_subscriber(self.gsas2_presenter.focus_run_observer_gsas2)
        self.calibration_presenter.add_prm_gsas2_subscriber(self.gsas2_presenter.prm_filepath_observer_gsas2)
        view.tabs.addTab(gsas2_view, "GSAS II")

    def setup_texture(self, view):
        texture_model = ProjectionModel()
        texture_view = TextureView()
        self.texture_presenter = TexturePresenter(texture_model, texture_view)
        self.focus_presenter.add_focus_texture_subscriber(self.texture_presenter.focus_run_observer)
        view.tabs.addTab(texture_view, "Texture")

    def setup_settings(self, view):
        settings_model = SettingsModel()
        settings_view = SettingsView(view)
        settings_presenter = SettingsPresenter(settings_model, settings_view)
        settings_presenter.load_settings_from_file_or_default()
        self.settings_presenter = settings_presenter
        self.setup_savedir_notifier(view)

    def setup_savedir_notifier(self, view):
        self.settings_presenter.savedir_notifier.add_subscriber(view.savedir_observer)

    def handle_close(self):
        self.fitting_presenter.data_widget.ads_observer.unsubscribe()
        self.fitting_presenter.data_widget.view.saveSettings()
        self.fitting_presenter.plot_widget.view.ensure_fit_dock_closed()

    def open_help_window(self):
        InterfaceManager().showCustomInterfaceHelp(self.doc, self.doc_folder)

    def open_settings(self):
        self.settings_presenter.show()

    def update_calibration(self, calibration):
        instrument = calibration.get_instrument()
        ceria_no = calibration.get_ceria_runno()
        self.statusbar_observable.notify_subscribers(f"CeO2: {ceria_no}, Instrument: {instrument}")

    @staticmethod
    def get_saved_rb_number() -> str:
        rb_number = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "rb_number")
        return rb_number

    @staticmethod
    def set_saved_rb_number(rb_number) -> None:
        set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "rb_number", rb_number)
