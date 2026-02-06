# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from qtpy import QtCore, QtWidgets
from mantidqt.icons import get_icon
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing
from mantidqt.utils.qt import load_ui
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.presenter import EngineeringDiffractionPresenter
from .tabs.common import SavedirObserver


from qtpy.QtCore import QRegExp
from qtpy.QtGui import QRegExpValidator

Ui_main_window, _ = load_ui(__file__, "main_window.ui")


class EngineeringDiffractionGui(QtWidgets.QMainWindow, Ui_main_window):
    """
    The engineering diffraction interface
    """

    status_savdirMaxwidth = 300

    def __init__(self, parent=None, window_flags=None):
        if window_flags is not None:
            super(EngineeringDiffractionGui, self).__init__(parent, window_flags)
        else:
            super(EngineeringDiffractionGui, self).__init__(parent)

        # Main Window
        self.setupUi(self)
        self.tabs = self.tab_main
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)

        self.btn_settings.setIcon(get_icon("mdi.settings", "black", 1.2))

        # Create status bar widgets
        self.status_label = QtWidgets.QLabel()
        self.savedir_label = QtWidgets.QLabel()
        self.savedir_label.setMaximumWidth(self.status_savdirMaxwidth)

        # observers
        self.update_statusbar_text_observable = GenericObserverWithArgPassing(self.set_statusbar_text)
        self.update_savedir_observable = GenericObserverWithArgPassing(self.update_savedir)
        self.savedir_observer = SavedirObserver(self)

        # this presenter needs to be accessible to this view so that it can be accessed by project save
        self.presenter = self.setup_presenter()

        # RB number validator
        q_reg_exp = QRegExp(r"(?!.* /|.*/ )(?!.* \\|.*\\ )(?!^ )[a-zA-Z0-9-_ ]+(?!.*[/\\]{3,})[a-zA-Z0-9-_/\\ ]+")
        validator = QRegExpValidator(q_reg_exp, self.lineEdit_RBNumber)
        self.lineEdit_RBNumber.setValidator(validator)

        # setup that can only happen with presenter created
        self.setup_statusbar()
        # Calibration Tab
        self.set_on_instrument_changed(self.presenter.calibration_presenter.set_instrument_override)
        self.set_on_rb_num_changed(self.presenter.calibration_presenter.set_rb_num)
        # Cropping Widget
        self.set_on_instrument_changed(self.presenter.calibration_presenter.cropping_widget.set_instrument_override)
        # Absorption Correction Tab
        self.set_on_instrument_changed(self.presenter.correction_presenter.set_instrument_override)
        self.set_on_rb_num_changed(self.presenter.correction_presenter.set_rb_num)
        # Focus Tab
        self.set_on_instrument_changed(self.presenter.focus_presenter.set_instrument_override)
        self.set_on_rb_num_changed(self.presenter.focus_presenter.set_rb_num)
        # Fitting Tab
        self.set_on_rb_num_changed(self.presenter.fitting_presenter.set_rb_num)
        # Texture Tab
        self.set_on_instrument_changed(self.presenter.texture_presenter.set_instrument_override)
        self.set_on_rb_num_changed(self.presenter.texture_presenter.set_rb_num)
        # GSAS Tab
        # self.set_on_instrument_changed(self.presenter.gsas2_presenter.set_instrument_override)
        self.set_on_rb_num_changed(self.presenter.gsas2_presenter.set_rb_num)
        # Settings Tab
        self.set_on_instrument_changed(self.presenter.settings_presenter.set_instrument_override)

        # load most recent calibration, if one saved
        self.presenter.calibration_presenter.load_last_calibration()

        # load previous rb number if saved, create mechanism to save
        self.set_rb_no(self.presenter.get_saved_rb_number())
        self.set_on_rb_num_changed(self.presenter.set_saved_rb_number)

        # Usage Reporting
        try:
            import mantid

            # register startup
            mantid.UsageService.registerFeatureUsage(mantid.kernel.FeatureType.Interface, "Engineering Diffraction", False)
        except ImportError:
            pass

    def setup_presenter(self):
        presenter = EngineeringDiffractionPresenter()
        presenter.setup_settings(self)  # init before fitting otherwise default peak not populated on first time opened
        presenter.setup_calibration(self)
        presenter.setup_correction(self)
        presenter.setup_fitting(self)
        presenter.setup_texture(self)
        presenter.setup_gsas2(self)
        presenter.setup_calibration_notifier()
        presenter.statusbar_observable.add_subscriber(self.update_statusbar_text_observable)
        presenter.savedir_observable.add_subscriber(self.update_savedir_observable)
        self.set_on_settings_clicked(presenter.open_settings)
        self.set_on_help_clicked(presenter.open_help_window)
        return presenter

    def setup_statusbar(self):
        self.statusbar.addWidget(self.status_label)
        self.set_statusbar_text("No Calibration Loaded.")
        self.statusbar.addWidget(self.savedir_label)
        self.update_savedir(self.presenter.settings_presenter.settings["save_location"])

    def set_statusbar_text(self, text):
        self.status_label.setText(text)

    def update_savedir(self, savedir):
        savedir_text = "SaveDir: " + savedir
        self.savedir_label.setToolTip(savedir_text)
        self.savedir_label.setText(savedir_text)

    def closeEvent(self, event):
        self.presenter.handle_close()
        self.setParent(None)
        event.accept()

    def get_rb_no(self):
        return self.lineEdit_RBNumber.text()

    def set_rb_no(self, text) -> None:
        self.lineEdit_RBNumber.setText(text)

    def set_on_help_clicked(self, slot):
        self.pushButton_help.clicked.connect(slot)

    def set_on_settings_clicked(self, slot):
        self.btn_settings.clicked.connect(slot)

    def set_on_rb_num_changed(self, slot):
        self.lineEdit_RBNumber.textChanged.connect(slot)

    def set_on_instrument_changed(self, slot):
        self.comboBox_instrument.currentIndexChanged.connect(slot)
