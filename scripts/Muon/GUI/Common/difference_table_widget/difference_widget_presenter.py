# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.difference_table_widget.difference_table_widget_presenter import DifferenceTablePresenter
from Muon.GUI.Common.difference_table_widget.difference_table_widget_view import DifferenceTableView
from Muon.GUI.Common.difference_table_widget.difference_widget_view import DifferenceView

diff_columns = ['diff_name', 'to_analyse', 'group_1', 'group_2']


class DifferencePresenter(object):
    def __init__(self, model):
        self.model = model
        self.group_view = DifferenceTableView()
        self.group_widget = DifferenceTablePresenter(self.group_view, self.model, "group")

        self.pair_view = DifferenceTableView()
        self.pair_widget = DifferenceTablePresenter(self.pair_view, self.model, "pair")

        self.view = DifferenceView(self.pair_view, self.group_view)

    def update_view_from_model(self):
        self.group_widget.update_view_from_model()
        self.pair_widget.update_view_from_model()

    def disable_editing(self):
        self.group_widget.disable_editing()
        self.pair_widget.disable_editing()

    def enable_editing(self):
        self.group_widget.enable_editing()
        self.pair_widget.enable_editing()

    def add_subscribers(self, observer_list):
        for observer in observer_list:
            self.group_widget.selected_diff_changed_notifier.add_subscriber(observer)
            self.pair_widget.selected_diff_changed_notifier.add_subscriber(observer)

    def on_data_changed(self, observer):
        self.group_widget.on_data_changed(observer)
        self.pair_widget.on_data_changed(observer)
