# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget import MuonAnalysisPlotWidget, FIT, RAW
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_view import MainPlotWidgetView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_model import PlotFreqFitPaneModel
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_presenter import PlotFreqFitPanePresenter

from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.dual_plot_maxent_pane.\
    dual_plot_maxent_pane_presenter import DualPlotMaxentPanePresenter
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.dual_plot_maxent_pane.\
     dual_plot_maxent_pane_model import DualPlotMaxentPaneModel
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.dual_plot_maxent_pane.\
    dual_plot_maxent_pane_view import DualPlotMaxentPaneView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.raw_pane.raw_pane_model import RawPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.quick_edit.quick_edit_widget import DualQuickEditWidget


# These are just used internally for ID purposes
# the rest of the code uses the names from the models
MAXENT = "maxent dual"


class FrequencyAnalysisPlotWidget(MuonAnalysisPlotWidget):
    def __init__(self, context=None, parent=None):
        super().__init__(context, parent)

    def _create_default_panes(self):
        self.view = MainPlotWidgetView(self._parent)
        # The plotting canvas widgets
        self.plotting_canvas_widgets = {}
        # The UI view
        self._views = {}
        # create default panes
        self.create_data_pane()
        self.create_fit_pane()
        self.create_maxent_pane()

    def handle_plot_mode_changed_by_user(self):
        old_plot_mode = self._current_plot_mode
        self._current_plot_mode = self.presenter.get_plot_mode
        if old_plot_mode == self._current_plot_mode:
            return
        if self._current_plot_mode==self.mode_name(RAW):
            # plot the raw data
            self.modes[RAW].handle_data_updated()

        self.presenter.hide(old_plot_mode)
        self.presenter.show(self._current_plot_mode)

    def update_freq_units_add_subscriber(self, subscriber):
        self.fit_mode.update_freq_units.add_subscriber(subscriber)

    """ Fit (and transform) pane """
    def create_fit_pane(self):
        self.fit_model = PlotFreqFitPaneModel(self._context)
        name = self.fit_model.name
        self.plotting_canvas_widgets[name] = PlottingCanvasWidget(self._parent, context=
                                                                        self._context.plot_panes_context[name],
                                                                        plot_model=self.fit_model)
        self._views[name] = BasePaneView(self._parent)
        self._views[name].add_canvas_widget(self.plotting_canvas_widgets[name].widget)

        self.modes[FIT] = PlotFreqFitPanePresenter(self._views[name], self.fit_model,
                                                   self._context,self._context.fitting_context,
                                                   self.plotting_canvas_widgets[name].presenter)
        self._panes.append(self.modes[FIT])

    @property
    def fit_index(self):
        return self.view.get_index(self.mode_name(FIT))

    @property
    def fit_mode(self):
        return self.modes[FIT]

    """ Maxent dual pane """
    def create_maxent_pane(self):
        self.dual_model = DualPlotMaxentPaneModel(self._context, self.data_model, RawPaneModel(self._context))
        name = self.dual_model.name
        dual_quick_edit = DualQuickEditWidget(self._context.plot_panes_context[name], self._parent)
        self.plotting_canvas_widgets[name] = PlottingCanvasWidget(self._parent, context=
                                                                        self._context.plot_panes_context[name],
                                                                        plot_model=self.dual_model, figure_options=dual_quick_edit)

        self._views[name] = DualPlotMaxentPaneView(self._parent)
        self._views[name].add_canvas_widget(self.plotting_canvas_widgets[name].widget)

        self.modes[MAXENT] = DualPlotMaxentPanePresenter(self._views[name], self.dual_model,
                                                         self._context,
                                                         self.plotting_canvas_widgets[name].presenter)
        self._panes.append(self.modes[MAXENT])
        self.maxent_mode.update_freq_units.add_subscriber(self.fit_mode.update_fit_pane_observer)
        self.fit_mode.update_maxent_plot.add_subscriber(self.maxent_mode.update_x_label_observer)

    @property
    def maxent_index(self):
        return self.view.get_index(self.mode_name(MAXENT))

    @property
    def maxent_mode(self):
        return self.modes[MAXENT]
