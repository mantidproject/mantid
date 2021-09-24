# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "fit_controls.ui")


class FitControlsView(ui_form, base_widget):
    """
    The FitControlsView includes the Fit, Undo Fit and Fit Script Generator buttons. It also has the Plot Guess
    checkbox, and the global fit status label.
    """

    def __init__(self, parent: QWidget = None):
        """Initialize the FitControlsView. The Undo Fit button is disabled as no fits exist yet."""
        super(FitControlsView, self).__init__(parent)
        self.setupUi(self)

        # Comment out this line to show the 'Fit Generator' button
        self.fit_generator_button.hide()

    def set_slot_for_fit_generator_clicked(self, slot) -> None:
        """Connect the slot for the Fit Generator button."""
        self.fit_generator_button.clicked.connect(slot)

    def set_slot_for_fit_button_clicked(self, slot) -> None:
        """Connect the slot for the Fit button."""
        self.fit_button.clicked.connect(slot)

    def set_slot_for_undo_fit_clicked(self, slot) -> None:
        """Connect the slot for the Undo Fit button."""
        self.undo_fit_button.clicked.connect(slot)

    def set_slot_for_plot_guess_changed(self, slot) -> None:
        """Connect the slot for the Plot Guess checkbox."""
        self.plot_guess_checkbox.stateChanged.connect(slot)

    def set_number_of_undos(self, number_of_undos: int) -> None:
        """Sets the allowed number of 'Undo Fit' events."""
        self.undo_fit_button.setText(f"Undo Fit ({number_of_undos})")
        self.undo_fit_button.setEnabled(number_of_undos != 0)

    @property
    def plot_guess(self) -> bool:
        """Returns true if plot guess is ticked."""
        return self.plot_guess_checkbox.isChecked()

    @plot_guess.setter
    def plot_guess(self, check: bool) -> None:
        """Sets whether or not plot guess is ticked."""
        self.plot_guess_checkbox.setChecked(check)

    def update_global_fit_status_label(self, fit_success: list) -> None:
        """Updates the global fit status label."""
        number_of_fits = len(fit_success)
        if number_of_fits == 0:
            self.global_fit_status_label.setText("No Fit")
            self.global_fit_status_label.setStyleSheet("color: black")
        elif all(fit_success):
            self.global_fit_status_label.setText("Fit Successful")
            self.global_fit_status_label.setStyleSheet("color: green")
        else:
            self.global_fit_status_label.setText(f"{number_of_fits - sum(fit_success)} of {number_of_fits} fits failed")
            self.global_fit_status_label.setStyleSheet("color: red")
