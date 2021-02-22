# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver

from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_view import GeneralFittingView
from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_options_view \
    import TFAsymmetryFittingOptionsView

from qtpy.QtWidgets import QWidget


class TFAsymmetryFittingView(GeneralFittingView):
    """
    The TFAsymmetryFittingView derives from the GeneralFittingView. It adds the TFAsymmetryFittingOptionsView to the
    widget.
    """

    def __init__(self, parent: QWidget = None, is_frequency_domain: bool = False):
        """Initializes the TFAsymmetryFittingView, and adds the TFAsymmetryFittingOptionsView widget."""
        super(TFAsymmetryFittingView, self).__init__(parent, is_frequency_domain)

        self.tf_asymmetry_fitting_options = TFAsymmetryFittingOptionsView(self)
        self.tf_asymmetry_fitting_options_layout.addWidget(self.tf_asymmetry_fitting_options)

        self.switch_to_normal_fitting_observer = GenericObserver(self.switch_to_normal_fitting)
        self.switch_to_tf_asymmetry_fitting_observer = GenericObserver(self.switch_to_tf_asymmetry_fitting)

    def switch_to_normal_fitting(self):
        self.tf_asymmetry_fitting_options.hide_normalisation_options()

    def switch_to_tf_asymmetry_fitting(self):
        self.tf_asymmetry_fitting_options.show_normalisation_options()
