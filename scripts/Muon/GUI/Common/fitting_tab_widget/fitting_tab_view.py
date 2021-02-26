# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_context import MuonContext
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing
from mantidqt.utils.qt import load_ui

from qtpy.QtWidgets import QWidget

ui_fitting_tab, _ = load_ui(__file__, "fitting_tab.ui")

NORMAL_FITTING_COMBO_INDEX = 0
TF_ASYMMETRY_FITTING_COMBO_INDEX = 1


class FittingTabView(QWidget, ui_fitting_tab):
    def __init__(self, parent: QWidget = None, context: MuonContext = None, fitting_view: QWidget = None,
                 is_frequency_domain: bool = False):
        super(FittingTabView, self).__init__(parent)
        self.setupUi(self)

        self.context = context

        self.fitting_view = fitting_view
        self.layout().addWidget(self.fitting_view)

        self.tf_asymmetry_mode_changed_notifier = GenericObservable()

        self.reset_tab_observer = GenericObserver(self.reset_tab)
        self.disable_tab_observer = GenericObserver(self.disable_view)
        self.enable_tab_observer = GenericObserver(self.enable_view)
        self.instrument_changed_observer = GenericObserver(self.handle_instrument_changed)

        self.double_pulse_observer = GenericObserverWithArgPassing(self.handle_pulse_type_changed)
        self.context.gui_context.add_non_calc_subscriber(self.double_pulse_observer)

        if not is_frequency_domain:
            self.tf_asymmetry_mode_changed_observer = GenericObserverWithArgPassing(
                self.handle_tf_asymmetry_mode_changed)
            self.fitting_type_combo_box.currentIndexChanged.connect(self.handle_fitting_type_changed)
        else:
            self.fitting_type_label.setHidden(True)
            self.fitting_type_combo_box.setHidden(True)

        self.disable_view()

    def handle_instrument_changed(self):
        self.fitting_type_combo_box.setCurrentIndex(NORMAL_FITTING_COMBO_INDEX)

    def handle_pulse_type_changed(self, updated_variables):
        if "DoublePulseEnabled" in updated_variables:
            self.fitting_type_combo_box.setCurrentIndex(NORMAL_FITTING_COMBO_INDEX)

    def handle_fitting_type_changed(self):
        current_index = self.fitting_type_combo_box.currentIndex()
        if current_index == NORMAL_FITTING_COMBO_INDEX:
            self.tf_asymmetry_mode_changed_notifier.notify_subscribers(False)
        elif current_index == TF_ASYMMETRY_FITTING_COMBO_INDEX:
            self.tf_asymmetry_mode_changed_notifier.notify_subscribers(True)

    def handle_tf_asymmetry_mode_changed(self, tf_asymmetry_on):
        self.fitting_type_combo_box.blockSignals(True)
        if not tf_asymmetry_on:
            self.fitting_type_combo_box.setCurrentIndex(NORMAL_FITTING_COMBO_INDEX)
        else:
            self.fitting_type_combo_box.setCurrentIndex(TF_ASYMMETRY_FITTING_COMBO_INDEX)
        self.fitting_type_combo_box.blockSignals(False)

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
