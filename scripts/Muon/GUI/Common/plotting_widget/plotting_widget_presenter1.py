# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from Muon.GUI.Common.plotting_widget.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plotting_widget.external_plotting_view import ExternalPlottingView
from Muon.GUI.Common.plotting_widget.plotting_widget_view1 import PlotWidgetView1
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQUENCY_EXTENSIONS
from Muon.GUI.Common.plotting_widget.workspace_finder import WorkspaceFinder
from Muon.GUI.Common.ADSHandler.workspace_naming import TF_ASYMMETRY_PREFIX


class PlotWidgetPresenter1(HomeTabSubWidget):

    def __init__(self, view: PlotWidgetView1, model=None, context=None):
        """
        :param view: A reference to the QWidget object for plotting
        :param model: A reference to a model which contains the plotting logic
        :param context: A reference to the Muon context object
        """
        self._view = view
        self._model = model
        self.context = context
        self.workspace_finder = WorkspaceFinder(self.context)

        # observers
        self.input_workspace_observer = GenericObserver(self.handle_data_updated)

    def handle_data_updated(self):
        """
        Handles the group, pair calculation finishing. Checks whether the list of workspaces has changed before doing
        anything as workspaces being modified in place is handled by the ADS handler observer.
        """
        print("CALLING HANDLE DATA UPDATED DATA UPATED ")

        if self._view.is_tiled_plot():
            num_axes = self.update_model_tile_plot_positions()
            self.new_plot_figure(num_axes)

        workspace_list = self.workspace_finder.get_workspace_list_to_plot(True, 'Asymmetry')

        self._view.plot_widget().plot_workspace_data(workspace_list, 0, False, False)

    def handle_plot_type_changed(self):
        pass

    def handle_rebin_options_changed(self):
        pass

    def handle_ads_observer_stuff(self):
        pass
