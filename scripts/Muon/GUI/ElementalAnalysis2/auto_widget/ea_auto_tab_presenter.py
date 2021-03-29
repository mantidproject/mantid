# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_view import EAAutoTabView
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import EAAutoTabModel
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter import EAMatchTablePresenter
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_table import EAAutoPopupTable
from mantidqt.utils.observer_pattern import GenericObserver


class EAAutoTabPresenter(object):

    def __init__(self , context, view, model, match_table):
        self.view = view
        self.model = model
        self.context = context
        self.match_table_presenter = match_table

        self.popup_match_table = None
        self.popup_peak_table = None

        self.find_peaks_observer = GenericObserver(self.run_find_peak_algorithms)
        self.show_peaks_table_observer = GenericObserver(self.show_peaks_table)
        self.show_match_table_observer = GenericObserver(self.show_match_table)
        self.update_match_table_observer = GenericObserver(self.update_match_table)
        self.update_view_observer = GenericObserver(self.update_view)

        self.enable_tab_observer = GenericObserver(self.enable_tab)
        self.disable_tab_observer = GenericObserver(self.disable_tab)

        self.setup_notifier()

    def setup_notifier(self):
        self.view.find_peaks_notifier.add_subscriber(self.find_peaks_observer)
        self.view.show_peaks_table_notifier.add_subscriber(self.show_peaks_table_observer)
        self.view.show_match_table_notifier.add_subscriber(self.show_match_table_observer)
        self.model.update_match_table_notifier.add_subscriber(self.update_match_table_observer)
        self.model.update_view_notifier.add_subscriber(self.update_view_observer)

    def run_find_peak_algorithms(self):
        parameters = self.view.get_parameters_for_find_peaks()
        if parameters == None:
            return
        self.model.handle_peak_algorithms(parameters)

    def show_peaks_table(self):
        parameters = self.view.show_peaks_table_combobox.currentText()
        self.table = EAAutoPopupTable()
        self.table.show()
        """
        if self.popup_peak_table != None:
            if parameter != self.popup_peak_table.name:
                del self.popup_peak_table
                self.popup_peak_table = EAAutoPopupTable(parameter,True)
                self.popup_peak_table.show()
        """

    def show_match_table(self):
        parameters = self.view.show_match_table_combobox.currentText()
        self.table = EAAutoPopupTable()
        self.table.show()
        """
        if self.popup_match_table != None:
            if parameter != self.popup_match_table.name:
                del self.popup_match_table
                self.popup_match_table = EAAutoPopupTable(parameter,False)
                self.popup_match_table.show()
        """

    def update_match_table(self):
        if self.model.table_entries != None:
            self.match_table_presenter.clear_table()
            self.match_table_presenter.update_table(self.model.table_entries)
            self.model.table_entries = None

    def update_view(self):
        #Check context for loaded workspaces and add to values find peak combobox
        #check all tables in ads or in context and add to show peaks and show matches combobox
        print("update view")
        group_names = self.context.group_context.group_names
        self.view.add_options_to_find_peak_combobox(group_names)

    def enable_tab(self):
        self.view.enable()

    def disable_tab(self):
        self.view.disable()