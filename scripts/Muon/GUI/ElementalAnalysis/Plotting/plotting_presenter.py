class PlotPresenter(object):
    def __init__(self, view):
        self.view = view

    def add_vline(self, plot_name, x_value, y_min, y_max, **kwargs):
        pass

    def add_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass

    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass

    def add_errors(self, *plots):
        pass
