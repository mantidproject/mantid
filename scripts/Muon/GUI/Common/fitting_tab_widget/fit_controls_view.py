# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy import QtWidgets

ui_fit_controls, _ = load_ui(__file__, "fit_controls.ui")


class FitControlsView(QtWidgets.QWidget, ui_fit_controls):

    def __init__(self, parent=None):
        super(FitControlsView, self).__init__(parent)
        self.setupUi(self)

        self.enable_undo_fit(False)

        # Comment out this line to show the 'Fit Generator' button
        self.fit_generator_button.hide()

    def set_slot_for_fit_generator_clicked(self, slot):
        """Connect the slot for the Fit Generator button."""
        self.fit_generator_button.clicked.connect(slot)

    def set_slot_for_fit_button_clicked(self, slot):
        """Connect the slot for the Fit button."""
        self.fit_button.clicked.connect(slot)

    def set_slot_for_undo_fit_clicked(self, slot):
        """Connect the slot for the Undo Fit button."""
        self.undo_fit_button.clicked.connect(slot)

    def set_slot_for_plot_guess_changed(self, slot):
        """Connect the slot for the Plot Guess checkbox."""
        self.plot_guess_checkbox.stateChanged.connect(slot)

    @property
    def plot_guess(self):
        """Returns true if plot guess is ticked."""
        return self.plot_guess_checkbox.isChecked()

    @plot_guess.setter
    def plot_guess(self, check):
        """Sets whether or not plot guess is ticked."""
        self.plot_guess_checkbox.setChecked(check)

    def enable_undo_fit(self, enable):
        """Sets whether or not undo fit is enabled."""
        self.undo_fit_button.setEnabled(enable)

    def update_global_fit_status_label(self, success_list):
        """Updates the global fit status label."""
        if len(success_list) == 0:
            self.global_fit_status_label.setText("No Fit")
            self.global_fit_status_label.setStyleSheet("color: black")
        elif all(success_list):
            self.global_fit_status_label.setText("Fit Successful")
            self.global_fit_status_label.setStyleSheet("color: green")
        else:
            self.global_fit_status_label.setText(
                f"{len(success_list) - sum(success_list)} of {len(success_list)} fits failed")
            self.global_fit_status_label.setStyleSheet("color: red")
