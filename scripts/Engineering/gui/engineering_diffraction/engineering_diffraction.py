# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from qtpy import QtCore, QtWidgets

from .tabs.calibration.model import CalibrationModel
from .tabs.calibration.view import CalibrationView
from .tabs.calibration.presenter import CalibrationPresenter
from .tabs.focus.model import FocusModel
from .tabs.focus.view import FocusView
from .tabs.focus.presenter import FocusPresenter

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt import load_ui

Ui_main_window, _ = load_ui(__file__, "main_window.ui")


class EngineeringDiffractionGui(QtWidgets.QMainWindow, Ui_main_window):
    """
    The engineering diffraction interface v2.0
    """
    def __init__(self, parent=None):
        super(EngineeringDiffractionGui, self).__init__(parent)

        # Main Window
        self.setupUi(self)
        self.doc = "Engineering Diffraction 2"
        self.tabs = self.tab_main
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.calibration_presenter = None
        self.focus_presenter = None
        self.set_on_help_clicked(self.open_help_window)

        # Setup Tabs
        self.setup_calibration()
        self.setup_focus()

        # Setup notifiers
        self.setup_calibration_notifier()

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

    def setup_calibration_notifier(self):
        self.calibration_presenter.calibration_notifier.add_subscriber(
            self.focus_presenter.calibration_observer)

    def set_on_help_clicked(self, slot):
        self.pushButton_help.clicked.connect(slot)

    def set_on_rb_num_changed(self, slot):
        self.lineEdit_RBNumber.textChanged.connect(slot)

    def set_on_instrument_changed(self, slot):
        self.comboBox_instrument.currentIndexChanged.connect(slot)

    def open_help_window(self):
        InterfaceManager().showCustomInterfaceHelp(self.doc)

    def get_rb_no(self):
        return self.lineEdit_RBNumber.text()
