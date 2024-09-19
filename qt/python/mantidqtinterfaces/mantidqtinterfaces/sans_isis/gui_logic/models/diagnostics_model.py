# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from SANS.sans.common.RowEntries import RowEntries
from mantidqtinterfaces.sans_isis.gui_logic.presenter.gui_state_director import GuiStateDirector


class DiagnosticsModel:
    @staticmethod
    def create_state(state_model_with_view_update, file, period, facility):
        table_row = RowEntries(sample_scatter=file, sample_scatter_period=period)
        gui_state_director = GuiStateDirector(state_model_with_view_update, facility)
        state = gui_state_director.create_state(table_row).all_states
        return state
