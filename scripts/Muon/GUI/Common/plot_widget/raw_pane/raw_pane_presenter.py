# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
from mantidqt.utils.observer_pattern import GenericObserver


class RawPanePresenter(BasePanePresenter):

    def __init__(self, view, model, context,figure_presenter):
        super().__init__(view, model, context,figure_presenter)
        self._data_type = ["Counts"]
        self._sort_by = ["Detector"]
        self.update_view()
        self._view.enable_plot_type_combo()
        self._view.hide_plot_diff()
        self._view.enable_tile_plotting_options()
        self._view.hide_plot_raw()
        self._view.disable_tile_plotting_options()
        self._view.set_is_tiled_plot(True)
        self.new_data_observer = GenericObserver(self.handle_new_data)

        self._view.set_slot_for_detectors_changed(self._selector_changed)
        self._view.set_slot_for_runs_changed(self._selector_changed)

    def handle_new_data(self, autoscale=True, hold_on=False):
        self._model.check_num_detectors()
        self.update_selectors()
        self.handle_data_updated(autoscale,hold_on)

    def handle_data_updated(self, autoscale=True, hold_on=False):
        detectors = self._view.get_detectors
        run = self._view.get_run
        workspace_list, indicies = self._model.get_workspace_list_and_indices_to_plot(True,
                                                                                      self._view.get_plot_type(), detectors, run)
        self.add_list_to_plot(workspace_list, indicies, hold=hold_on, autoscale=autoscale)

    def handle_plot_tiled_state_changed(self):
        """
        Handles the tiled plots checkbox being changed in the view. This leads to two behaviors:
        If switching to tiled plot, create a new figure based on the number of tiles and replot the data
        If switching from a tiled plot, create a new single figure and replot the data
        """
        self.context.plot_panes_context[self.name].settings.set_tiled(self._view.is_tiled_plot())
        if self._view.is_tiled_plot():
            tiled_by = self._view.tiled_by()
            self.context.plot_panes_context[self.name].settings.set_tiled_by(tiled_by)
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.convert_plot_to_tiled_plot(keys)
        else:
            self._figure_presenter.convert_plot_to_single_plot()

    def update_selectors(self):
        det_list = self._model.gen_detector_options()
        self._view.update_detectors(det_list)

        run_list = self._model.gen_run_list()
        self._view.update_runs(run_list)

    def _selector_changed(self) -> None:
        self.handle_data_updated()
