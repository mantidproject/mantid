# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder TOF plot presenter.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver


class DNSTofPowderPlotPresenter(DNSObserver):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        self._plotted_script_number = 0

    def set_view_from_param(self):
        pass

    def _plot(self):
        workspace = self.model.get_plot_workspace()
        if workspace:
            self.view.set_plot(workspace)
            self._plotted_script_number = self.param_dict["tof_powder_script_generator"]["script_number"]
        else:
            self.raise_error("No processed data found, generate script first.")

    def tab_got_focus(self):
        if self.param_dict["tof_powder_script_generator"]["script_number"] != self._plotted_script_number:
            self._plot()

    def process_auto_reduction_request(self):
        pass
