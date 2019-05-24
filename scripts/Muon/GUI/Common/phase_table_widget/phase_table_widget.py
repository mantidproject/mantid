# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.phase_table_widget.phase_table_view import PhaseTableView
from Muon.GUI.Common.phase_table_widget.phase_table_presenter import PhaseTablePresenter


class PhaseTabWidget(object):
    def __init__(self, context, parent):
        self.phase_table_view = PhaseTableView(parent)

        self.phase_table_presenter = PhaseTablePresenter(self.phase_table_view, context)

        self.phase_table_view.set_calculate_phase_table_action(self.phase_table_presenter.handle_calulate_phase_table_clicked)

        self.phase_table_view.set_calculate_phase_quad_action(self.phase_table_presenter.handle_calculate_phase_quad_button_clicked)

        self.phase_table_view.set_cancel_action(self.phase_table_presenter.cancel)
