# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.phase_table_widget.phase_table_view import PhaseTableView
from mantidqtinterfaces.Muon.GUI.Common.phase_table_widget.phase_table_model import PhaseTableModel
from mantidqtinterfaces.Muon.GUI.Common.phase_table_widget.phase_table_presenter import PhaseTablePresenter


class PhaseTabWidget(object):
    def __init__(self, context, parent):
        self.phase_table_view = PhaseTableView(parent)
        self.phase_table_model = PhaseTableModel(context)
        self.phase_table_presenter = PhaseTablePresenter(self.phase_table_view, self.phase_table_model)

        # Phase table actions
        self.phase_table_view.set_calculate_phase_table_action(self.phase_table_presenter.handle_calculate_phase_table_clicked)
        self.phase_table_view.set_cancel_calculate_phase_table_action(self.phase_table_presenter.cancel_current_alg)
        self.phase_table_view.set_phase_table_changed_action(self.phase_table_presenter.handle_phase_table_changed)

        # Phaseqaud table actions
        self.phase_table_view.set_add_phasequad_action(self.phase_table_presenter.handle_add_phasequad_button_clicked)
        self.phase_table_view.set_remove_phasequad_action(self.phase_table_presenter.handle_remove_phasequad_button_clicked)

        context.update_view_from_model_notifier.add_subscriber(self.phase_table_presenter.update_view_from_model_observer)
