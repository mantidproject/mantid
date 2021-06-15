# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
#from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing#, GenericObservable


class PlotTimeFitPanePresenter(BasePanePresenter):

    def __init__(self, view, model, context,figure_presenter):
        super().__init__(view, model, context,figure_presenter)
        self._name = "Fit Data"
        self._data_type = ["Asymmetry"]
        self._sort_by = ["Group/Pair", "Run"]
        self.update_view()
        self._view.enable_tile_plotting_options()
