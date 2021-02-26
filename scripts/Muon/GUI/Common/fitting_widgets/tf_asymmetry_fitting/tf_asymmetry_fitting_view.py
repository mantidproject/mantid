# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_view import GeneralFittingView
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_options_view \
    import TFAsymmetryFittingOptionsView
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_mode_switcher_view \
    import TFAsymmetryModeSwitcherView

from qtpy.QtWidgets import QWidget


class TFAsymmetryFittingView(GeneralFittingView):
    """
    The TFAsymmetryFittingView derives from the GeneralFittingView. It adds the TFAsymmetryFittingOptionsView to the
    widget.
    """

    def __init__(self, parent: QWidget = None, is_frequency_domain: bool = False):
        """Initializes the TFAsymmetryFittingView, and adds the TFAsymmetryFittingOptionsView widget."""
        super(TFAsymmetryFittingView, self).__init__(parent, is_frequency_domain)

        self.tf_asymmetry_mode_switcher = TFAsymmetryModeSwitcherView(self)
        self.tf_asymmetry_mode_switcher_layout.addWidget(self.tf_asymmetry_mode_switcher)

        self.tf_asymmetry_fitting_options = TFAsymmetryFittingOptionsView(self)
        self.tf_asymmetry_fitting_options_layout.addWidget(self.tf_asymmetry_fitting_options)

        self.tf_asymmetry_mode = False

    def set_slot_for_fitting_type_changed(self, slot) -> None:
        """Sets the slot for handling when a the fitting type is switched."""
        self.tf_asymmetry_mode_switcher.set_slot_for_fitting_type_changed(slot)

    def set_slot_for_normalisation_changed(self, slot) -> None:
        """Sets the slot for handling when a normalisation value is changed by the user."""
        self.tf_asymmetry_fitting_options.set_slot_for_normalisation_changed(slot)

    @property
    def tf_asymmetry_mode(self) -> bool:
        """Returns true if TF Asymmetry fitting mode is currently active."""
        return self.tf_asymmetry_mode_switcher.tf_asymmetry_mode

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_on: bool) -> None:
        """Hides or shows the normalisation options depending on if TF Asymmetry fitting mode is on or off."""
        self.tf_asymmetry_mode_switcher.tf_asymmetry_mode = tf_asymmetry_on
        self.tf_asymmetry_fitting_options.set_tf_asymmetry_mode(tf_asymmetry_on)

    @property
    def normalisation(self) -> float:
        """Returns the normalisation value currently displayed in the normalisation line edit."""
        return self.tf_asymmetry_fitting_options.normalisation

    @normalisation.setter
    def normalisation(self, value: float) -> None:
        """Sets the normalisation value currently displayed in the normalisation line edit."""
        self.tf_asymmetry_fitting_options.normalisation = value
