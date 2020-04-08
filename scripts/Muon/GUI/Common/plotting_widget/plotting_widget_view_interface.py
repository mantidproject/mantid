from abc import abstractmethod


# THe purpose of this abstraction is to allow for changes in the plotting (view)
# These are somewhat planned, as a future goal the standard workbench plotting should
# be used in the GUI, as it provides far more flexibility.
class PlottingWidgetViewInterface(object):

    # define some methods that the view must have
    def __init__(self):
        pass

    @abstractmethod
    def setup_plot_type_options(self, options): pass

    @abstractmethod
    def setup_tiled_by_options(self, options): pass

    @abstractmethod
    def get_plot_type(self): pass

    @abstractmethod
    def is_data_rebinned(self): pass

    @abstractmethod
    def is_tiled_plot(self): pass

    @abstractmethod
    def tiled_by(self): pass

    @abstractmethod
    def on_rebin_options_changed(self, slot): pass

    @abstractmethod
    def on_plot_type_changed(self, slot): pass

    @abstractmethod
    def on_tiled_by_type_changed(self, slot): pass

    @abstractmethod
    def on_plot_tiled_changed(self, slot): pass

    @abstractmethod
    def on_external_plot_pressed(self, slot): pass


