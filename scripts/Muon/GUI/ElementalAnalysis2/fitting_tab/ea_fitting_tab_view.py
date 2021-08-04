from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_view import GeneralFittingView
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_option_view import EAFittingOptionsView
from Muon.GUI.Common.data_selectors.cyclic_data_selector_view import CyclicDataSelectorView
from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_model import SPECTRA_INDICES
from qtpy.QtWidgets import QWidget


class EAFittingTabView(GeneralFittingView):

    def __init__(self, parent: QWidget = None):
        super(GeneralFittingView, self).__init__(parent)
        self.general_fitting_options = EAFittingOptionsView(self)
        self.general_fitting_options_layout.addWidget(self.general_fitting_options)
        self.spectrum_selector = CyclicDataSelectorView(self)
        self.workspace_selector_layout.addWidget(self.spectrum_selector)
        self.set_workspace_combo_box_label("Select workspace")
        self.set_spectrum_combo_box_label("Select spectrum")
        self.spectrum_selector.update_dataset_name_combo_box(reversed(list(SPECTRA_INDICES)))

    def set_slot_for_spectrum_changed(self, slot) -> None:
        self.spectrum_selector.set_slot_for_dataset_changed(slot)

    def set_spectrum_combo_box_label(self, text: str) -> None:
        """Sets the label text next to the workspace selector combobox."""
        self.spectrum_selector.set_data_combo_box_label(text)

    @property
    def current_workspace_index(self) -> int:
        """Returns the selected dataset name."""
        return SPECTRA_INDICES[self.spectrum_selector.current_dataset_name]
