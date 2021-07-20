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
        self._figure_presenter.set_x_ticks(tick_labels[0])
        self._figure_presenter.set_y_ticks(tick_labels[1])

    def update_x_plot_range(self, x_limits: list) -> None:
        """Updates the x range of a plot using the provided x limits of a workspace."""
        self._figure_presenter.set_plot_range(self._calculate_new_x_range(x_limits))

    def _calculate_new_x_range(self, x_limits: list) -> tuple:
        """Calculates the new X range based off the x limits of the plotted data."""
        x_lower, x_upper = x_limits[0], x_limits[1]
        margin = self.context.plot_panes_context[self.name].settings.x_axis_margin
        x_offset = abs(x_upper - x_lower) * margin
        return [x_lower - x_offset, x_upper + x_offset]
