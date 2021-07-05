# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_presenter import PlotFitPanePresenter


class PlotModelFitPanePresenter(PlotFitPanePresenter):

    def __init__(self, view, model, context, fitting_context, figure_presenter):
        super().__init__(view, model, context, fitting_context, figure_presenter)
        self._data_type = [""]
        self._sort_by = [""]
        self.update_view()

        self.update_override_tick_labels_observer = GenericObserverWithArgPassing(
            self.update_override_x_and_y_tick_labels)
        self.update_x_range_observer = GenericObserverWithArgPassing(self.update_x_plot_range)

        self._figure_presenter.set_errors(True)
        self._view.disable_plot_raw_option()
        self._view.hide_plot_type()
        self._view.hide_plot_raw()
        self._view.hide_tiled_by()

    def update_override_x_and_y_tick_labels(self, tick_labels: list) -> None:
        """Updates the override x and y tick labels to use when plotting data."""
        # This will be implemented in a separate PR when adjusting some of the plot options for model fitting.
        pass

    def update_x_plot_range(self, x_limits: list) -> None:
        """Updates the x range of a plot using the provided x limits of a workspace."""
        self._figure_presenter.set_plot_range(x_limits)
