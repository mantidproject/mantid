# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_presenter import PlotFitPanePresenter

# A 10% margin when updating the x range based on the data x limits
X_MARGIN = 0.1


class PlotModelFitPanePresenter(PlotFitPanePresenter):

    def __init__(self, view, model, context, fitting_context, figure_presenter):
        super().__init__(view, model, context, fitting_context, figure_presenter)
        self._data_type = [""]
        self._sort_by = [""]
        self.update_view()

        self.update_x_range_observer = GenericObserverWithArgPassing(self.update_x_plot_range)

        self._figure_presenter.set_autoscale(True)
        self._figure_presenter.set_errors(True)
        self._figure_presenter.set_plot_as_point_data(True)
        self._view.disable_plot_raw_option()
        self._view.hide_plot_type()
        self._view.hide_plot_raw()
        self._view.hide_tiled_by()

    def update_x_plot_range(self, x_limits: list) -> list:
        """Updates the x range of a plot using the provided x limits of a workspace."""
        self._figure_presenter.set_plot_range(self._calculate_new_x_range(x_limits))

    @staticmethod
    def _calculate_new_x_range(x_limits: list) -> tuple:
        """Calculates the new X range based off the x limits of the plotted data."""
        x_lower, x_upper = x_limits[0], x_limits[1]
        x_offset = abs(x_upper - x_lower) * X_MARGIN
        return x_lower - x_offset, x_upper + x_offset
