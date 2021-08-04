# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_presenter import GeneralFittingPresenter
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_view import EAFittingTabView
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_model import EAFittingTabModel


class EAFittingTabPresenter(GeneralFittingPresenter):

    def __init__(self, view: EAFittingTabView, model: EAFittingTabModel):
        super(EAFittingTabPresenter, self).__init__(view, model)
        self.view.set_slot_for_spectrum_changed(self.handle_spectrum_changed)
        self.handle_spectrum_changed()

    def handle_spectrum_changed(self):
        self.model.current_spectrum = self.view.current_workspace_index

    def handle_fit_clicked(self) -> None:
        """Handle when the fit button is clicked."""
        if self.model.number_of_datasets < 1:
            self.view.warning_popup("No data selected for fitting.")
            return
        if not self.view.fit_object:
            return
        self.model.fitting_context.dataset_names = self.model._get_equivalent_binned_or_unbinned_workspaces()
        self.model.save_current_fit_function_to_undo_data()
        self._perform_fit()
