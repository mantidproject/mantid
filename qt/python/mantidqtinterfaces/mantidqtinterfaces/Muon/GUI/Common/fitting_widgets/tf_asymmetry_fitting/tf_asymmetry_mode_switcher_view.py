# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.qt import load_ui

from qtpy.QtWidgets import QWidget

ui_form, base_widget = load_ui(__file__, "tf_asymmetry_mode_switcher.ui")

NORMAL_FITTING_COMBO_INDEX = 0
TF_ASYMMETRY_FITTING_COMBO_INDEX = 1


class TFAsymmetryModeSwitcherView(ui_form, base_widget):
    """
    The TFAsymmetryModeSwitcherView has a combo box for switching between normal and TF Asymmetry fitting.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the TFAsymmetryModeSwitcherView."""
        super(TFAsymmetryModeSwitcherView, self).__init__(parent)
        self.setupUi(self)

    def set_slot_for_fitting_type_changed(self, slot) -> None:
        """Sets the slot for handling when a the fitting type is switched."""
        self.fitting_type_combo_box.currentIndexChanged.connect(slot)

    @property
    def tf_asymmetry_mode(self) -> bool:
        """Returns true if TF Asymmetry fitting mode is currently active."""
        return True if self.fitting_type_combo_box.currentIndex() == TF_ASYMMETRY_FITTING_COMBO_INDEX else False

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_on: bool) -> None:
        """Sets the TF Asymmetry fitting mode to be on or off."""
        self.fitting_type_combo_box.blockSignals(True)
        if not tf_asymmetry_on:
            self.fitting_type_combo_box.setCurrentIndex(NORMAL_FITTING_COMBO_INDEX)
        else:
            self.fitting_type_combo_box.setCurrentIndex(TF_ASYMMETRY_FITTING_COMBO_INDEX)
        self.fitting_type_combo_box.blockSignals(False)

    def hide_fitting_type_combobox(self) -> None:
        """Hides the fitting type combobox."""
        self.fitting_type_label.setHidden(True)
        self.fitting_type_combo_box.setHidden(True)
