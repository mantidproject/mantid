# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from qtpy import QtCore, QtWidgets

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
from .settings.settings_model import SettingsModel
from .settings.settings_view import SettingsView
from .settings.settings_presenter import SettingsPresenter
from mantidqt.icons import get_icon

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt import load_ui

Ui_main_window, _ = load_ui(__file__, "main_window.ui")


class EngineeringDiffractionGui(QtWidgets.QMainWindow, Ui_main_window):
    """
    The engineering diffraction interface
    """
    def __init__(self, parent=None, window_flags=None):
        if window_flags is not None:
            super(EngineeringDiffractionGui, self).__init__(parent, window_flags)
        else:
            super(EngineeringDiffractionGui, self).__init__(parent)

    # Main Window
        self.setupUi(self)
        self.doc = "Engineering Diffraction"
        self.tabs = self.tab_main
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.calibration_presenter = None
        self.focus_presenter = None
        self.fitting_presenter = None
        self.settings_presenter = None
        self.calibration_observer = CalibrationObserver(self)
        self.set_on_help_clicked(self.open_help_window)

        self.set_on_settings_clicked(self.open_settings)
        self.btn_settings.setIcon(get_icon("mdi.settings", "black", 1.2))

        # Setup status bar
        self.status_label = QtWidgets.QLabel()
        self.setup_statusbar()

        # Setup Elements
        self.setup_settings()
        self.setup_calibration()
        self.setup_focus()
        self.setup_fitting()

        # Setup notifiers
        self.setup_calibration_notifier()

        # Usage Reporting
        try:
            import mantid

            # register startup
            mantid.UsageService.registerFeatureUsage(mantid.kernel.FeatureType.Interface,
                                                     "Engineering Diffraction", False)
        except ImportError:
            pass

    def setup_settings(self):
        model = SettingsModel()
        view = SettingsView(self)
        self.settings_presenter = SettingsPresenter(model, view)
        self.settings_presenter.load_settings_from_file_or_default()

    def setup_calibration(self):
        cal_model = CalibrationModel()
        cal_view = CalibrationView(parent=self.tabs)
        self.calibration_presenter = CalibrationPresenter(cal_model, cal_view)
        self.set_on_instrument_changed(self.calibration_presenter.set_instrument_override)
        self.set_on_rb_num_changed(self.calibration_presenter.set_rb_num)
        self.tabs.addTab(cal_view, "Calibration")

    def setup_focus(self):
        focus_model = FocusModel()
        focus_view = FocusView()
        self.focus_presenter = FocusPresenter(focus_model, focus_view)
        self.set_on_instrument_changed(self.focus_presenter.set_instrument_override)
        self.set_on_rb_num_changed(self.focus_presenter.set_rb_num)
        self.tabs.addTab(focus_view, "Focus")

    def setup_fitting(self):
        fitting_view = FittingView()
        self.fitting_presenter = FittingPresenter(fitting_view)
        self.tabs.addTab(fitting_view, "Fitting")

    def setup_calibration_notifier(self):
        self.calibration_presenter.calibration_notifier.add_subscriber(
            self.focus_presenter.calibration_observer)
        self.calibration_presenter.calibration_notifier.add_subscriber(self.calibration_observer)

    def setup_statusbar(self):
        self.statusbar.addWidget(self.status_label)
        self.set_statusbar_text("No Calibration Loaded.")

    def set_on_help_clicked(self, slot):
        self.pushButton_help.clicked.connect(slot)

    def set_on_settings_clicked(self, slot):
        self.btn_settings.clicked.connect(slot)

    def set_on_rb_num_changed(self, slot):
        self.lineEdit_RBNumber.textChanged.connect(slot)

    def set_on_instrument_changed(self, slot):
        self.comboBox_instrument.currentIndexChanged.connect(slot)

    def open_help_window(self):
        InterfaceManager().showCustomInterfaceHelp(self.doc)

    def open_settings(self):
        self.settings_presenter.load_existing_settings()
        self.settings_presenter.show()

    def get_rb_no(self):
        return self.lineEdit_RBNumber.text()

    def update_calibration(self, calibration):
        instrument = calibration.get_instrument()
        van_no = get_run_number_from_path(calibration.get_vanadium(), instrument)
        sample_no = get_run_number_from_path(calibration.get_sample(), instrument)
        self.set_statusbar_text(f"V: {van_no}, CeO2: {sample_no}, Instrument: {instrument}")

    def set_statusbar_text(self, text):
        self.status_label.setText(text)
