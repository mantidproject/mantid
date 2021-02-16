# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from mantidqt.utils.qt import load_ui

from qtpy.QtWidgets import QStackedWidget, QWidget

ui_fitting_tab, _ = load_ui(__file__, "fitting_tab.ui")

NORMAL_FITTING_COMBO_INDEX = 0
TF_ASYMMETRY_FITTING_COMBO_INDEX = 1


class FittingTabView(QWidget, ui_fitting_tab):
    def __init__(self, parent=None, context=None, general_fitting_view=None):
        super(FittingTabView, self).__init__(parent)
        self.setupUi(self)

        self.context = context

        self.fit_type_stacked_widget = QStackedWidget()
        self.fit_type_stacked_widget.addWidget(general_fitting_view)

        self.layout().addWidget(self.fit_type_stacked_widget)

        self.reset_tab_observer = GenericObserver(self.reset_tab)
        self.disable_tab_observer = GenericObserver(self.disable_view)
        self.enable_tab_observer = GenericObserver(self.enable_view)
        self.instrument_changed_observer = GenericObserver(self.handle_instrument_changed)

        self.double_pulse_observer = GenericObserverWithArgPassing(self.handle_pulse_type_changed)
        self.context.gui_context.add_non_calc_subscriber(self.double_pulse_observer)

        self.disable_view()

    def handle_instrument_changed(self):
        self.fitting_type_combo_box.setCurrentIndex(NORMAL_FITTING_COMBO_INDEX)

    def handle_pulse_type_changed(self, updated_variables):
        if "DoublePulseEnabled" in updated_variables:
            self.fitting_type_combo_box.setCurrentIndex(NORMAL_FITTING_COMBO_INDEX)

    def reset_tab(self):
        """Disable all widgets in the fitting tab."""
        self.fitting_type_combo_box.setCurrentIndex(NORMAL_FITTING_COMBO_INDEX)
        self.disable_view()

    def disable_view(self):
        """Disable all widgets in the fitting tab."""
        self.setEnabled(False)

    def enable_view(self):
        """Enable all widgets in the fitting tab."""
        self.setEnabled(True)
