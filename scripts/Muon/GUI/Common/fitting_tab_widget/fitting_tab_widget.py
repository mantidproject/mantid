# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_view import FittingTabView
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_presenter import FittingTabPresenter


class FittingTabWidget(object):
    def __init__(self, context, parent):
        self.fitting_tab_view = FittingTabView(parent)

        self.fitting_tab_presenter = FittingTabPresenter(self.fitting_tab_view, context)

        self.fitting_tab_view.set_slot_for_select_workspaces_to_fit(self.fitting_tab_presenter.handle_select_fit_data_clicked)
        self.fitting_tab_view.set_slot_for_display_workspace_changed(self.fitting_tab_presenter.handle_display_workspace_changed)
        self.fitting_tab_view.set_slot_for_use_raw_changed(self.fitting_tab_presenter.handle_use_rebin_changed)
