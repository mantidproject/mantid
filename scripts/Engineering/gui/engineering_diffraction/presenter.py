# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from .tabs.calibration.model import CalibrationModel
from .tabs.calibration.view import CalibrationView
from .tabs.calibration.presenter import CalibrationPresenter
from .tabs.common import CalibrationObserver
from .tabs.common.path_handling import get_run_number_from_path
from .tabs.focus.model import FocusModel
from .tabs.focus.view import FocusView
from .tabs.focus.presenter import FocusPresenter
from .tabs.fitting.view import FittingView
from .tabs.fitting.presenter import FittingPresenter
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.observer_pattern import GenericObserver, GenericObservable


class EngineeringDiffractionPresenter(object):
    def __init__(self, view):
        self.view_tabs = view.tabs
        self.calibration_presenter = None
        self.focus_presenter = None
        self.fitting_presenter = None
        self.settings_presenter = None

        self.doc = "Engineering Diffraction"

        # Setup Elements
        self.setup_calibration()
        self.setup_focus()
        self.setup_fitting()

        # Setup observers
        self.calibration_observer = CalibrationObserver(self)
        self.close_event_observer = GenericObserver(self.handle_close)

        # Setup observables
        self.statusbar_observable = GenericObservable()
        self.savedir_observable = GenericObservable()

        # Setup notifiers
        self.setup_calibration_notifier()

    def handle_close(self):
        self.fitting_presenter.data_widget.ads_observer.unsubscribe()
        self.fitting_presenter.plot_widget.view.ensure_fit_dock_closed()

    def setup_calibration(self):
        cal_model = CalibrationModel()
        cal_view = CalibrationView(parent=self.view_tabs)
        self.calibration_presenter = CalibrationPresenter(cal_model, cal_view)
        self.view_tabs.addTab(cal_view, "Calibration")

    def setup_focus(self):
        focus_model = FocusModel()
        focus_view = FocusView()
        self.focus_presenter = FocusPresenter(focus_model, focus_view)
        self.view_tabs.addTab(focus_view, "Focus")

    def setup_fitting(self):
        fitting_view = FittingView()
        self.fitting_presenter = FittingPresenter(fitting_view)
        self.focus_presenter.add_focus_subscriber(self.fitting_presenter.data_widget.presenter.focus_run_observer)
        self.view_tabs.addTab(fitting_view, "Fitting")

    def setup_calibration_notifier(self):
        self.calibration_presenter.calibration_notifier.add_subscriber(
            self.focus_presenter.calibration_observer)
        self.calibration_presenter.calibration_notifier.add_subscriber(self.calibration_observer)

    def open_help_window(self):
        InterfaceManager().showCustomInterfaceHelp(self.doc)

    def open_settings(self):
        self.settings_presenter.show()

    def update_calibration(self, calibration):
        instrument = calibration.get_instrument()
        van_no = get_run_number_from_path(calibration.get_vanadium(), instrument)
        sample_no = get_run_number_from_path(calibration.get_sample(), instrument)
        self.statusbar_observable.notify_subscribers(f"V: {van_no}, CeO2: {sample_no}, Instrument: {instrument}")
