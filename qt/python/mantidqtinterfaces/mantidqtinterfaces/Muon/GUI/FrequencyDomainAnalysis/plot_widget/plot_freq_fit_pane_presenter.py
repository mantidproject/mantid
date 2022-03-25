# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_presenter import PlotFitPanePresenter
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQ, FIELD, GAUSS


class PlotFreqFitPanePresenter(PlotFitPanePresenter):

    def __init__(self, view, model, context, fitting_context, figure_presenter):
        super().__init__(view, model, context, fitting_context, figure_presenter)
        self._data_type = [FREQ, FIELD]
        self.context.frequency_context.x_label = FREQ
        self._sort_by = [""]
        self.update_view()
        self._view.hide_plot_raw()
        self._view.hide_tiled_by()
        self.update_freq_units = GenericObservable()
        self.update_maxent_plot = GenericObservable()
        self.update_fit_pane_observer = GenericObserver(self._update_fit_pane)

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        self.context.frequency_context.x_label = self._view.get_plot_type()
        self._figure_presenter.set_plot_range(self.context.frequency_context.range())
        # need to add observable for switching units cannot reuse it as it causes a loop
        self.update_maxent_plot.notify_subscribers()

        # need to send signal out to update stuff => dont undate here
        # the slot will update the plot when the fit list updates
        self.update_freq_units.notify_subscribers()

    def handle_rebin_options_changed(self):
        # there is no way to rebin the data -> do nothing
        return

    def _update_fit_pane(self):
        if self.context.frequency_context.unit() == GAUSS:
            self._view.set_plot_type(FIELD)
        else:
            self._view.set_plot_type(FREQ)
        self._figure_presenter.set_plot_range(self.context.frequency_context.range())
        self.update_freq_units.notify_subscribers()
