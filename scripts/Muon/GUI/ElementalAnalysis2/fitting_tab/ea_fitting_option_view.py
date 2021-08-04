# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_options_view import GeneralFittingOptionsView

from qtpy.QtWidgets import QWidget

EA_FIT_BY_OPTIONS = ["Run", "Detector"]


class EAFittingOptionsView(GeneralFittingOptionsView):
    """
    The GeneralFittingOptionsView includes the Simultaneous fitting options, and the cyclic dataset display combobox.
    """

    def __init__(self, parent: QWidget = None):
        """Initializes the GeneralFittingOptionsView. By default the simultaneous options are disabled."""
        super(EAFittingOptionsView, self).__init__(parent)
        self.simul_fit_by_combo.clear()
        self._setup_simultaneous_fit_by_combo_box(EA_FIT_BY_OPTIONS)
