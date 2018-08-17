class PlotPresenter(object):
    def __init__(self, view):
        self.view = view

    def get_subplot(self, name):
        return self.view.get_subplot(name)

    def get_subplots(self):
        return self.view.get_subplots()

    def add_subplot(self, name):
        return self.view.add_subplot(name)

<<<<<<< ae7573c727e5735ae06fb9409e70f3bca0028ba1
    def plot(self, name, workspace):
        self.view.plot(name, workspace)

||||||| merged common ancestors
=======
    def plot_workspace(self, name, workspace):
        self.view.plot_workspace(name, workspace)

>>>>>>> refs #23260 add workspace plotting
    def remove_subplot(self, name):
        self.view.remove_subplot(name)

    def call_plot_method(self, name, func, *args, **kwargs):
        return self.view.call_plot_method(name, func, *args, **kwargs)

    def add_vline(self, plot_name, x_value, y_min, y_max, **kwargs):
        return self.view.add_vline(plot_name, x_value, y_min, y_max, **kwargs)

    def add_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        return self.view.add_hline(plot_name, y_value, x_min, x_max, **kwargs)

    def add_moveable_vline(self, plot_name, x_value, y_minx, y_max, **kwargs):
        pass

    def add_moveable_hline(self, plot_name, y_value, x_min, x_max, **kwargs):
        pass
